// ----------------------------------------------------------------------------------------------------
//  Title			FLineTrackerCPU.cpp
//  Description		Implementation of class FLineTrackerCPU
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 13 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FLineTrackerCPU.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLineTrackerCPU
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FLineTrackerCPU::FLineTrackerCPU()
: m_pImageBuffer(NULL),
  m_pDepthBuffer(NULL)
{
	m_frameWidth = m_frameSize.width();
	m_frameHeight = m_frameSize.height();
}

FLineTrackerCPU::~FLineTrackerCPU()
{
	F_SAFE_DELETE_ARRAY(m_pImageBuffer);
	F_SAFE_DELETE_ARRAY(m_pDepthBuffer);
}

// Overrides ------------------------------------------------------------------------------------------

void FLineTrackerCPU::onReset()
{
	_reset();
}

void FLineTrackerCPU::onProjectModel()
{
	// statistics
	m_sampleCount = 0;
	m_candidateCount = 0;
	m_pixelCount = 0;

	_projectModel();
}

const FGLTextureRect& FLineTrackerCPU::onProcessedFrame(size_t index) const
{
	if (index == 0)
		return m_fbTexAnnotationCanvas2; // hidden line model
	else if (index == 1)
		return m_texOverlayFittedPose; // result of edge search

	return m_sourceFrame;
}

// Internal functions ---------------------------------------------------------------------------------

void FLineTrackerCPU::_reset()
{
	m_frameWidth = m_frameSize.width();
	m_frameHeight = m_frameSize.height();

	// Create annotation canvas 2
	m_fbTexAnnotationCanvas2.createAllocate(FGLPixelFormat::R8G8B8A8_UNorm, m_frameSize);
	m_fbAnnotationCanvas2.create();
	m_fbAnnotationCanvas2.attachColorTexture(m_fbTexAnnotationCanvas2, 0);
	F_ASSERT(m_fbAnnotationCanvas2.checkStatus());
	m_annotationCanvas2.setCanvasRect(FRect2f(0.0f, 0.0f, m_frameSize.width(), m_frameSize.height()));

	// Host memory for depth and image buffer
	F_SAFE_DELETE_ARRAY(m_pDepthBuffer);
	F_SAFE_DELETE_ARRAY(m_pImageBuffer);
	size_t numPixels = m_frameSize.width() * m_frameSize.height();
	m_pDepthBuffer = new float[numPixels];
	m_pImageBuffer = new FPixelRGBA32f[numPixels];
}

void FLineTrackerCPU::_projectModel()
{
	// Retrieve source image and depth buffer
	m_sourceFrame.read(FGLDataFormat::RGBA, FGLDataType::Float, m_pImageBuffer);
	m_fbTexModelDepth.read(FGLDataFormat::Depth, FGLDataType::Float, m_pDepthBuffer);

	// Transform model according to current transformation
	FMatrix4f matMVP;
	m_pCamera->getModelViewCurrent(matMVP);
	m_pModel->transform(matMVP);

	// Search source image for edges
	FMatrix4f matMV_Extra, matPGL_Extra, matMVPGL_Extra;
	m_pCamera->getModelViewExtra(matMV_Extra);
	m_pCamera->getProjectionGLExtra(matPGL_Extra);
	matMVPGL_Extra = matPGL_Extra * matMV_Extra;

	FVector4f matImageScale(m_frameSize.width() / 2.0f, m_frameSize.height() / 2.0f, 0.5f, 1.0f);
	FVector4f matImageTranslation(1.0f, 1.0f, 1.0f, 0.0f);

	// For every line segment, create sample points and search in orthogonal direction
	m_annotationCanvas2.clear();
	m_pModel->beginAddCandidates();

	for (size_t l = 0, nl = m_pModel->lines().size(); l < nl; l++)
	{
		int edgeId = l;
		int sampleId = 0;

		const FLineModel::glLine_t& segment = m_pModel->lines().at(l);
		FVector4f p0 = (matMVPGL_Extra * segment.modelPoint0).homogenize();
		FVector4f p1 = (matMVPGL_Extra * segment.modelPoint1).homogenize();
		p0 += matImageTranslation;
		p1 += matImageTranslation;
		p0 *= matImageScale;
		p1 *= matImageScale;

		m_annotationCanvas2.addLine(p0.x(), p0.y(), p1.x(), p1.y(), FColor(0.0f, 0.5f, 1.0f));

		FVector4f line = p1 - p0;
		float length = sqrtf(line.x() * line.x() + line.y() * line.y());
		FVector4f unitLine = line / length;
		FVector2f lineNormal(-unitLine.y(), unitLine.x());

		float offset = length * 0.5f;
		float distance = length;
		float minSampleDistance = m_samplingDistance * m_frameSize.width() * 0.5f;
		
		while(distance >= minSampleDistance && sampleId < (int)FLineModel::MAX_SAMPLES_PER_EDGE)
		{
			for (float t = offset; t < length; t += distance)
			{
				FVector4f sp = p0 + unitLine * t;
				int spi = _pixIndex(sp.x(), sp.y());
				if (spi == -1)
					continue; // point is outside of image

				float spDepth = sp.z();
				float imDepth = m_pDepthBuffer[spi];
				if (spDepth - 0.0005f >= imDepth)
					continue; // point is hidden

				FVector2f searchP0(
					sp.x() - lineNormal.x() * m_searchRange,
					sp.y() - lineNormal.y() * m_searchRange);
				FVector2f searchP1(
					sp.x() + lineNormal.x() * m_searchRange,
					sp.y() + lineNormal.y() * m_searchRange);

				FVector2f searchDir = (searchP1 - searchP0).normalize();
				FVector2i sP0i(floorf(searchP0.x() + 0.5f), floorf(searchP0.y() + 0.5f));
				FVector2i sP1i(floorf(searchP1.x() + 0.5f), floorf(searchP1.y() + 0.5f));

				m_annotationCanvas2.addLine(sP0i.x(), sP0i.y(), sP1i.x(), sP1i.y(), FColor(0.0f, 0.7f, 0.3f));

				_searchLine(edgeId, sampleId, searchDir, sP0i, sP1i);
				sampleId++;

				m_sampleCount++; // statistics

				if (sampleId >= (int)FLineModel::MAX_SAMPLES_PER_EDGE)
					break;
			}
			
			distance = offset;
			offset *= 0.5f;
		}
	}

	m_pModel->endAddCandidates();

	m_fbAnnotationCanvas2.bind();
	glClear(GL_COLOR_BUFFER_BIT);
	m_annotationCanvas2.draw();

	FGLFramebuffer::bindDefault(); // bind default target
	F_GLERROR_ASSERT;
}

void FLineTrackerCPU::_searchLine(int edgeId, int sampleId, const FVector2f& direction,
								  const FVector2i& p0, const FVector2i& p1)
{
	// Mid-point line-drawing algorithm, see [Agoston, 2005], p. 43
	int fx = 1;
	int fy = 1;

	if (p0.x() > p1.x())
		fx = -1;
	if (p0.y() > p1.y())
		fy = -1;

	int x0 = fx * p0.x();
	int y0 = fy * p0.y();
	int x1 = fx * p1.x();
	int y1 = fy * p1.y();

	int dx = x1 - x0;
	int dy = y1 - y0;
	int x = x0;
	int y = y0;

	if (dx >= dy)
	{
		int d = 2 * dy - dx;
		int posInc = 2 * dy;
		int negInc = 2 * (dy - dx);
		_testPoint(edgeId, sampleId, direction, fx * x, fy * y);
		while (x < x1)
		{
			if (d <= 0)
				d += posInc;
			else {
				d += negInc;
				y++;
			}
			x++;
			_testPoint(edgeId, sampleId, direction, fx * x, fy * y);
		}
	}
	else
	{
		int d = 2 * dx - dy;
		int posInc = 2 * dx;
		int negInc = 2 * (dx - dy);
		_testPoint(edgeId, sampleId, direction, fx * x, fy * y);
		while (y < y1)
		{
			if (d <= 0)
				d += posInc;
			else {
				d += negInc;
				x++;
			}
			y++;
			_testPoint(edgeId, sampleId, direction, fx * x, fy * y);
		}
	}

}

void FLineTrackerCPU::_testPoint(int edgeId, int sampleId,
								 const FVector2f& direction, int x, int y)
{
	static int cx = -1;
	static int cy = -1;
	static int cd = 0.0f;

	int angle = FMath::rad2deg(atan2f(direction.y(), direction.x()));
	if (angle < 0) angle += 360;

	int x0 = floorf(x - direction.x() * 1.0f + 0.5f);
	int y0 = floorf(y - direction.y() * 1.0f + 0.5f);
	int x1 = floorf(x + direction.x() * 1.0f + 0.5f);
	int y1 = floorf(y + direction.y() * 1.0f + 0.5f);

	FVector3f c0 = _filterPixel(angle, x0, y0);
	FVector3f c1 = _filterPixel(angle, x1, y1);

	float dLuma = 0.3334f * (fabsf(c1.x() - c0.x()) + fabsf(c1.y() - c0.y()) + fabsf(c1.z() - c0.z()));
	float dChroma = c0.normalize() * c1.normalize();
	float d = dLuma * m_edgeLumaWeight * 10.0f + dChroma * m_edgeChromaWeight * 0.1f;

	if (d >= m_edgeThreshold)
	{
		if (cd < d)
		{
			cd = d;
			cx = x;
			cy = y;
		}
	}
	else
	{
		if (cd > 0.0f)
		{
			m_pModel->addCandidate(edgeId, sampleId, FVector2f(cx, cy), cd, 1.0f);
			m_annotationCanvas2.addMarker(cx, cy, 0.5f, FColor(1.0f, 1.0f, 0.3f));
			cd = 0.0f;

			m_candidateCount++; // statistics
		}
	}

	m_pixelCount++; // statistics
}

FVector3f FLineTrackerCPU::_filterPixel(int angle, int px, int py)
{
	F_ASSERT(angle >= 0 && angle < 360);
	int fi = angle * m_blurFiterWidth * m_blurFiterWidth;
	int fw2 = m_blurFiterWidth / 2;

	if (px < fw2 || px >= m_frameWidth - fw2 || py < fw2 || py >= m_frameHeight - fw2)
		return FVector3f(0.0f, 0.0f, 0.0f);

	FVector3f sum;
	sum.makeZero();

	for (int y = -fw2; y <= fw2; y++)
	{
		for (int x = -fw2; x <= fw2; x++)
		{
			float coeff = m_pBlurFilter[fi++];

			if (coeff > 0.0f)
			{
				const FPixelRGBA32f& p = m_pImageBuffer[_pixIndex(px + x, py + y)];
				sum += FVector3f(p.r, p.g, p.b) * coeff;
			}
		}
	}

	return sum;
}

// ----------------------------------------------------------------------------------------------------