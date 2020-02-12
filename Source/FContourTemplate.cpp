// ----------------------------------------------------------------------------------------------------
//  Title			FContourTemplate.cpp
//  Description		Implementation of class FContourTemplate
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-06 22:24:21 +0200 (Di, 06 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "Eigen/Dense"
#include "levmar.h"
#include "FlowGL.h"
#include "FContour.h"
#include "FContourTemplate.h"
#include "FLevmarTermReason.h"

#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FContourTemplate
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FContourTemplate::FContourTemplate()
: m_patchSize(0, 0),
  m_pDistanceMap(NULL),
  m_pGradientMap(NULL),
  m_pCurrentContour(NULL)
{
}

FContourTemplate::FContourTemplate(const QSize& patchSize)
: m_patchSize(patchSize),
  m_pCurrentContour(NULL)
{
	int numPixels = m_patchSize.width() * m_patchSize.height();
	m_pDistanceMap = new float[numPixels];
	m_pGradientMap = new FVector2f[numPixels];

	memset(m_pDistanceMap, 0, numPixels * sizeof(float));
	memset(m_pGradientMap, 0, numPixels * sizeof(FVector2f));
}

FContourTemplate::~FContourTemplate()
{
	F_SAFE_DELETE_ARRAY(m_pDistanceMap);
	F_SAFE_DELETE_ARRAY(m_pGradientMap);
}

// Public commands ------------------------------------------------------------------------------------

void FContourTemplate::createMaps(const FContour* pContour)
{
	int numPixels = m_patchSize.width() * m_patchSize.height();
	memset(m_pDistanceMap, 0, numPixels * sizeof(float));
	memset(m_pGradientMap, 0, numPixels * sizeof(FVector2f));

	int length = pContour->length;
	int nx = m_patchSize.width();
	int ny = m_patchSize.height();
	float fx = nx * 0.5f;
	float fy = ny * 0.5f;

	for (int i = 0; i < length; i++)
	{
		//int x = (int)((normalizedContour.pos[i].x() + 1.0f) * fx + 0.5f);
		//int y = (int)((normalizedContour.pos[i].y() + 1.0f) * fy + 0.5f);
		int x = (int)((pContour->pos[i].x() + 1.0f) * fx);
		int y = (int)((pContour->pos[i].y() + 1.0f) * fy);

		if (x >= 0 && x < nx && y >= 0 && y < ny)
			m_pDistanceMap[x + y * nx] = FLT_MAX;
	}

	_signedDistanceTransform();
}

void FContourTemplate::serialize(FArchive& ar)
{
	if (ar.isReading())
	{
		ar >> m_patchSize;
		size_t numPixels = m_patchSize.width() * m_patchSize.height();

		F_SAFE_DELETE_ARRAY(m_pDistanceMap);
		m_pDistanceMap = new float[numPixels];
		for (size_t i = 0; i < numPixels; i++)
			ar >> m_pDistanceMap[i];

		F_SAFE_DELETE_ARRAY(m_pGradientMap);
		m_pGradientMap = new FVector2f[numPixels];
		for (size_t i = 0; i < numPixels; i++)
			ar >> m_pGradientMap[i];
	}
	else
	{
		ar << m_patchSize;
		size_t numPixels = m_patchSize.width() * m_patchSize.height();
		for (size_t i = 0; i < numPixels; i++)
			ar << m_pDistanceMap[i];
		for (size_t i = 0; i < numPixels; i++)
			ar << m_pGradientMap[i];
	}
}

// Public queries -------------------------------------------------------------------------------------

void FContourTemplate::matchContour(const FContour* pContour,
									OUT FMatrix3f& homography,
									OUT float* pInfo) const
{
	F_ASSERT(pContour);
	m_pCurrentContour = pContour;

	// levmar setup
	int numData = pContour->length;
	float params[] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
	float lmInfo[LM_INFO_SZ];
	float opts[] = { 1e-3, 1e-6, 1e-6, 1e-6, 1e-6 }; // tau, eps1, eps2, eps3, delta
	Eigen::Matrix<float, 8, 8> matCovar;
	float* pCovar = matCovar.data();
	float* pBuffer = FGlobalConstants::pLevmarDetectionWorkspace;

	// run Levenberg-Marquardt optimization
	int iter = slevmar_der(sLevmarUpdate, sLevmarJacobian,
		params, NULL, 8, numData, 100, NULL /*opts*/, lmInfo, pBuffer, pCovar, (void*)this, 0, 0.0f);

	float mse = lmInfo[1] / (float)numData;

	// copy result
	homography(2, 2) = 1.0f;
	for (int i = 0; i < 8; i++)
	{
		int r = i / 3;
		int c = i % 3;
		homography(r, c) = params[i];
	}

	if (pInfo)
	{
		pInfo[0] = mse;
		pInfo[1] = 0.0f;
		pInfo[2] = homography.determinant();

		if (mse < 25.0f && (lmInfo[6] != 2 || pInfo[2] < 0.25f))
		{
			F_CONSOLE(QString("matchContour #%1: it: %2, mse: %3, h-det: %4, covar: %5, term: %6")
				.arg(pContour->index()).arg(iter).arg(pInfo[0]).arg(pInfo[2]).arg(pInfo[1])
				.arg(FLevmarTermReason(lmInfo[6]).toString()));
		}
	}
}

void FContourTemplate::drawToTexture(FGLTextureRect& texture,
									 const FContour* pContour /* = NULL */,
									 const FMatrix3f* pHomography /* = NULL */) const
{
	size_t numBytes = m_patchSize.width() * m_patchSize.height() * sizeof(float);
	float* pData = (float*)malloc(numBytes);
	memcpy(pData, m_pDistanceMap, numBytes);

	if (pContour)
	{
		int nx = m_patchSize.width();
		int ny = m_patchSize.height();
		float fx = nx * 0.5f;
		float fy = ny * 0.5f;

		float p1 = pHomography->at(0, 0); float p2 = pHomography->at(0, 1);
		float p3 = pHomography->at(0, 2); float p4 = pHomography->at(1, 0);
		float p5 = pHomography->at(1, 1); float p6 = pHomography->at(1, 2);
		float p7 = pHomography->at(2, 0); float p8 = pHomography->at(2, 1);

		for (int i = 0; i < pContour->length; i++)
		{
			// unwarped, normalized contour coordinates
			float cx = pContour->pos[i].x();
			float cy = pContour->pos[i].y();

			// warped, normalized contour coordinates
			float w = cx * p7 + cy * p8 + 1.0f;
			float x = (cx * p1 + cy * p2 + p3) / w;
			float y = (cx * p4 + cy * p5 + p6) / w;

			// denormalized, warped coordinates, rounded to int, clamped to patch size
			//int px = (int)((x + 1.0f) * fx + 0.5f);
			//int py = (int)((y + 1.0f) * fy + 0.5f);
			int px = (int)((x + 1.0f) * fx);
			int py = (int)((y + 1.0f) * fy);
			px = fMinMax(px, 0, nx - 1);
			py = fMinMax(py, 0, ny - 1);
			pData[py * nx + px] = -1.0f;
		}
	}

	texture.createInitialize(FGLPixelFormat::R32_Float, m_patchSize,
		FGLDataFormat::Red, FGLDataType::Float, numBytes, pData, false); // DO NOT FLIP
	free(pData);
}

// Internal functions ---------------------------------------------------------------------------------

void FContourTemplate::_signedDistanceTransform()
{
	int numPixels = m_patchSize.width() * m_patchSize.height();
	int nx = m_patchSize.width();
	int ny = m_patchSize.height();

	// initialize distance map (seed pixels = 0.0, other pixels = FLT_MAX)
	for (int i = 0; i < numPixels; i++)
		m_pDistanceMap[i] = (m_pDistanceMap[i] > 0) ? 0.0f : FLT_MAX;
	
	// reset gradient map
	for (int i = 0; i < numPixels; i++)
		m_pGradientMap[i].makeZero();

	// pass 1: top to bottom
	for (int y = 1; y < ny; y++)
	{
		int yy = y * nx;
		for (int x = 1; x < nx; x++)
		{
			int i = yy + x;
			float d = m_pDistanceMap[i];
			if (d > 0.0f)
			{
				float d1x = m_pGradientMap[i-1].x();
				float d2y = m_pGradientMap[i-nx].y();
				float d1 = m_pDistanceMap[i-1] - 2 * d1x + 1;
				float d2 = m_pDistanceMap[i-nx] - 2 * d2y + 1;
				if (d1 < d && d1 < d2)
				{
					m_pDistanceMap[i] = d1;
					m_pGradientMap[i].set(d1x - 1, m_pGradientMap[i-1].y());
				}
				else if (d2 < d)
				{
					m_pDistanceMap[i] = d2;
					m_pGradientMap[i].set(m_pGradientMap[i-nx].x(), d2y - 1);
				}
			}
		}

		for (int x = nx - 2; x >= 0; x--)
		{
			int i = yy + x;
			float d = m_pDistanceMap[i];
			if (d > 0.0f)
			{
				float d1x = m_pGradientMap[i+1].x();
				float d1 = m_pDistanceMap[i+1] + 2 * d1x + 1;
				if (d1 < d)
				{
					m_pDistanceMap[i] = d1;
					m_pGradientMap[i].set(d1x + 1, m_pGradientMap[i+1].y());
				}
			}
		}
	}

	// pass 2: bottom to top
	for (int y = ny - 2; y >= 0; y--)
	{
		int yy = y * nx;
		for (int x = nx - 2; x >= 0; x--)
		{
			int i = yy + x;
			float d = m_pDistanceMap[i];
			if (d > 0.0f)
			{
				float d1x = m_pGradientMap[i+1].x();
				float d2y = m_pGradientMap[i+nx].y();
				float d1 = m_pDistanceMap[i+1] + 2 * d1x + 1;
				float d2 = m_pDistanceMap[i+nx] + 2 * d2y + 1;
				if (d1 < d && d1 < d2)
				{
					m_pDistanceMap[i] = d1;
					m_pGradientMap[i].set(d1x + 1, m_pGradientMap[i+1].y());
				}
				else if (d2 < d)
				{
					m_pDistanceMap[i] = d2;
					m_pGradientMap[i].set(m_pGradientMap[i+nx].x(), d2y + 1);
				}
			}
		}

		for (int x = 1; x < nx; x++)
		{
			int i = yy + x;
			float d = m_pDistanceMap[i];
			if (d > 0.0f)
			{
				float d1x = m_pGradientMap[i-1].x();
				float d1 = m_pDistanceMap[i-1] - 2 * d1x + 1;
				if (d1 < d)
				{
					m_pDistanceMap[i] = d1;
					m_pGradientMap[i].set(d1x - 1, m_pGradientMap[i-1].y());
				}
			}
		}
	}

	// convert offset vectors to normalized gradient vectors
	for (int i = 0; i < numPixels; i++)
		m_pGradientMap[i] = -m_pGradientMap[i].normalized();
}

void FContourTemplate::_levmarUpdate(float* p, float* hx, int m, int n)
{
	F_ASSERT(m_pCurrentContour);
	F_ASSERT(m_pCurrentContour->length == n);
	F_ASSERT(m == 8);

	// denormalization of coordinates for lookup
	int nx = m_patchSize.width();
	int ny = m_patchSize.height();
	float fx = nx * 0.5f;
	float fy = ny * 0.5f;

	// coefficients of homography
	float p1 = p[0];
	float p2 = p[1];
	float p3 = p[2];
	float p4 = p[3];
	float p5 = p[4];
	float p6 = p[5];
	float p7 = p[6];
	float p8 = p[7];

	for (int i = 0; i < n; i++)
	{
		// unwarped, normalized contour coordinates
		float cx = m_pCurrentContour->pos[i].x();
		float cy = m_pCurrentContour->pos[i].y();

		// warped, normalized contour coordinates
		float w = cx * p7 + cy * p8 + 1.0f;
		float x = (cx * p1 + cy * p2 + p3) / w;
		float y = (cx * p4 + cy * p5 + p6) / w;

		// denormalized, warped coordinates, rounded to int, clamped to patch size
		//int px = (int)((x + 1.0f) * fx + 0.5f);
		//int py = (int)((y + 1.0f) * fy + 0.5f);
		int px = (int)((x + 1.0f) * fx);
		int py = (int)((y + 1.0f) * fy);
		px = fMinMax(px, 0, nx - 1);
		py = fMinMax(py, 0, ny - 1);

		// read distance map
		int pi = py * nx + px;
		hx[i] = m_pDistanceMap[pi];
	}
}

void FContourTemplate::_levmarJacobian(float* p, float* j, int m, int n)
{
	F_ASSERT(m_pCurrentContour);
	F_ASSERT(m_pCurrentContour->length == n);
	F_ASSERT(m == 8);

	// denormalization of coordinates for lookup
	int nx = m_patchSize.width();
	int ny = m_patchSize.height();
	float fx = nx * 0.5f;
	float fy = ny * 0.5f;

	// coefficients of homography
	float p1 = p[0];
	float p2 = p[1];
	float p3 = p[2];
	float p4 = p[3];
	float p5 = p[4];
	float p6 = p[5];
	float p7 = p[6];
	float p8 = p[7];

	int k = 0;
	for (int i = 0; i < n; i++)
	{
		// unwarped, normalized contour coordinates
		float cx = m_pCurrentContour->pos[i].x();
		float cy = m_pCurrentContour->pos[i].y();

		// warped, normalized contour coordinates
		float w = cx * p7 + cy * p8 + 1.0f;
		float x = (cx * p1 + cy * p2 + p3) / w;
		float y = (cx * p4 + cy * p5 + p6) / w;

		// denormalized, warped coordinates, rounded to int, clamped to patch size
		//int px = (int)((x + 1.0f) * fx + 0.5f);
		//int py = (int)((y + 1.0f) * fy + 0.5f);
		int px = (int)((x + 1.0f) * fx);
		int py = (int)((y + 1.0f) * fy);
		px = fMinMax(px, 0, nx - 1);
		py = fMinMax(py, 0, ny - 1);

		// gradient at warped contour coordinates
		int pi = py * nx + px;
		float gx = m_pGradientMap[pi].x();
		float gy = m_pGradientMap[pi].y();

		// jacobian at warped contour coordinates
		float d = x * p7 + y * p8 + 1.0f;
		float d2 = d * d;

		float v1 = x * p1 + y * p2 + p3;
		float v2 = x * p4 + y * p5 + p6;

		j[k++] = gx * (x / d);
		j[k++] = gx * (y / d);
		j[k++] = gx * (1.0f / d);
		
		j[k++] = gy * (x / d);
		j[k++] = gy * (y / d);
		j[k++] = gy * (1.0f / d);

		j[k++] = gx * (-x * v1 / d2) + gy * (-x * v2 / d2);
		j[k++] = gx * (-y * v1 / d2) + gy * (-y * v2 / d2);
	}
}

// ----------------------------------------------------------------------------------------------------