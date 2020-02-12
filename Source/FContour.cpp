// ----------------------------------------------------------------------------------------------------
//  Title			FContour.cpp
//  Description		Implementation of class FContour
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-09 13:00:10 +0200 (Fr, 09 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "Eigen/Dense"
#include "levmar.h"
#include "FLevmarTermReason.h"
#include "FlowGL.h"
#include "FContour.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FContour
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FContour::FContour()
: length(0),
  m_isValid(false),
  m_isNormalized(false),
  m_isClosed(true)
{
}

// Public commands ------------------------------------------------------------------------------------

void FContour::clear()
{
	m_isValid = false;
	m_isNormalized = false;
	m_isClosed = true;
	
	length = 0;
}

void FContour::process(int imWidth, int imHeight)
{
	F_ASSERT(!m_isValid);
	m_isNormalized = false;
	F_ASSERT(length <= MAX_CONTOUR_LENGTH);

	if (!m_isClosed)
		return;

	if (length < 75 || length == MAX_CONTOUR_LENGTH)
		return;

	// find mean and bounding box

	float minX = FLT_MAX;
	float minY = FLT_MAX;
	float maxX = 0.0f;
	float maxY = 0.0f;
	float meanX = 0.0f;
	float meanY = 0.0f;

	for (int i = 0; i < length; i++)
	{
		float x = pos[i].x();
		meanX += x;
		minX = fMin(minX, x);
		maxX = fMax(maxX, x);

		float y = pos[i].y();
		meanY += y;
		minY = fMin(minY, y);
		maxY = fMax(maxY, y);
	}

	// reject if contour touches image bounds
	if (minX < 2.0f || maxX >= imWidth - 2.0f || minY < 2.0f || maxY >= imHeight - 2.0f)
		return;

	// reject if contour width or height is > 0.75 * image width or height
	if (maxX - minX > imWidth * 0.75f || maxY - minY > imHeight * 0.75f)
		return;

	// reject if bounding box area is > 0.5 * image area
	if ((maxX - minX) * (maxY - minY) * 2.0f > imWidth * imHeight)
		return;

	// reject if bounding box is smaller than 16 * 16
	if (maxX - minX < 16.0f || maxY - minY < 16.0f)
		return;

	meanX /= (float)length;
	meanY /= (float)length;
	m_barycenter.set(meanX, meanY);
	m_boundingBox.set(minX, minY, maxX, maxY);

	_fitEllipse();
	if (m_ellipse.radii().x() > imWidth || m_ellipse.radii().y() > imWidth)
		return;

	if (m_ellipse.radii().x() < m_ellipse.radii().y())
	{
		m_ellipse.setRadii(m_ellipse.radii().y(), m_ellipse.radii().x());
		m_ellipse.setTiltAngle(m_ellipse.tiltAngle() - (float)FMath::piHalf);
	}

	m_ellipse.setRadii(m_ellipse.radii() * 1.85f);
	m_orientation.set(cosf(m_ellipse.tiltAngle()), sinf(m_ellipse.tiltAngle()));

	// ****** NEW: Adjust radii to make sure contour is inside the enclosing quad.

	float a = -m_ellipse.tiltAngle();
	float si = sinf(a);
	float co = cosf(a);
	float cx = m_ellipse.center().x();
	float cy = m_ellipse.center().y();
	minX = FLT_MAX;
	minY = FLT_MAX;
	maxX = -FLT_MAX;
	maxY = -FLT_MAX;

	for (int i = 0; i < length; i++)
	{
		float x = pos[i].x() - cx;
		float y = pos[i].y() - cy;
		float tx = x * co - y * si;
		float ty = x * si + y * co;
		
		minX = fMin(minX, tx);
		maxX = fMax(maxX, tx);
		minY = fMin(minY, ty);
		maxY = fMax(maxY, ty);
	}

	float dx = 0.5f * (minX + maxX);
	float dy = 0.5f * (minY + maxY);

	float tdx =  dx * co + dy * si;
	float tdy = -dx * si + dy * co;

	m_ellipse.setCenter(cx + tdx, cy + tdy);
	m_ellipse.setRadii(0.65f * (maxX - minX), 0.65f * (maxY - minY));
	m_quadCoords = m_ellipse.enclosingQuad();

	// ****** NEW: Adjust radii to make sure contour is inside the enclosing quad.

	//qDebug() << "\nELLIPSE FIT";
	//qDebug() << m_ellipse.toString();

	m_scale.set(1.0f / m_ellipse.radii().x(), 1.0f / m_ellipse.radii().y());
	m_translation.set(-m_ellipse.center());
	m_rotationAngle = -m_ellipse.tiltAngle();

	m_isValid = true;
}

void FContour::normalize()
{
	F_ASSERT(m_isValid && !m_isNormalized);
	m_isNormalized = true;

	float sin_a = sinf(m_rotationAngle);
	float cos_a = cosf(m_rotationAngle);

	for (int i = 0; i < length; i++)
	{
		float x = pos[i].x() + m_translation.x();
		float y = pos[i].y() + m_translation.y();

		pos[i].set((x * cos_a - y * sin_a) * m_scale.x(),
				   (x * sin_a + y * cos_a) * m_scale.y());
	}
}

// Public queries -------------------------------------------------------------------------------------

void FContour::getNormalizationMatrix(FMatrix3f& matNormalization) const
{
	FMatrix3f matRotation;
	matRotation.makeRotation2D(m_rotationAngle);

	FMatrix3f matTranslation;
	matTranslation.makeTranslation(m_translation);

	FMatrix3f matScale;
	matScale.makeScale(m_scale);
	
	matNormalization = matScale * matRotation * matTranslation;
}

void FContour::getInverseNormalizationMatrix(FMatrix3f& matInverseNormalization) const
{
	FMatrix3f matRotation;
	matRotation.makeRotation2D(-m_rotationAngle);

	FMatrix3f matTranslation;
	matTranslation.makeTranslation(-m_translation);

	FMatrix3f matScale;
	matScale.makeScale(1.0f / m_scale.x(), 1.0f / m_scale.y());

	matInverseNormalization = matTranslation * matRotation * matScale;
}

void FContour::drawToTexture(FGLTextureRect& texture, const QSize& textureSize)
{
	F_ASSERT(!"Is this function ever called?");

	int nx = textureSize.width();
	int ny = textureSize.height();
	float fx = nx * 0.5f;
	float fy = ny * 0.5f;

	quint8* pImage = new quint8[nx * ny];
	size_t numBytes = nx * ny * sizeof(quint8);
	memset(pImage, 0, numBytes);

	for (int i = 0; i < length; i++)
	{
		int x = (int)((pos[i].x() + 1.0f) * fx + 0.5f);
		int y = (int)((pos[i].y() + 1.0f) * fy + 0.5f);
		pImage[y * nx + x] = 255;
	}

	texture.createInitialize(FGLPixelFormat::R8_UNorm, textureSize,
		FGLDataFormat::Red, FGLDataType::UnsignedByte, numBytes, pImage, false);

	delete[] pImage;
}

// Internal functions ---------------------------------------------------------------------------------

void FContour::_fitEllipse2()
{
	// Method: See http://cococubed.asu.edu/papers/conics/fitzgibbon_1999.pdf

	Eigen::Matrix<float, Eigen::Dynamic, 6> matD(length, 6);
	for (int i = 0; i < length; ++i)
	{
		float x = pos[i].x();
		float y = pos[i].y();
		matD(i, 0) = x * x;
		matD(i, 1) = x * y;
		matD(i, 2) = y * y;
		matD(i, 3) = x;
		matD(i, 4) = y;
		matD(i, 5) = 1.0f;
	}

	Eigen::Matrix<float, 6, 6> matS = matD.transpose() * matD;

	Eigen::Matrix<float, 6, 6> matC;
	matC.setZero();
	matC(0, 2) = -2.0f;
	matC(1, 1) =  1.0f;
	matC(2, 0) = -2.0f;

	Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXf> solver(matS, matC);

	int i;
	for (i = 0; i < 6; i++)
		if (solver.eigenvalues()[i] < 0.0f)
			break;

	std::stringstream txt;
	txt << solver.eigenvalues();
	F_TRACE(QString("Fit Ellipse: EV - %1").arg(txt.str().c_str()));
	F_ASSERT(i < 6);
	Eigen::VectorXf params = solver.eigenvectors().col(i);

	// calculate radii (length of semi-axes), center and angle
	// source: http://mathworld.wolfram.com/Ellipse.html

	float a = params[0];
	float b = params[1];
	float c = params[2];
	float d = params[3];
	float e = params[4];
	float f = params[5];

	float r = b*b - 4.0f*a*c;
	float cx = c*d - b*e / r;
	float cy = a*e - b*d / r;

	float q = 2 * (a*e*e + c*d*d + f*b*b - 2.0f*b*d*e - a*c*f);
	float s = sqrtf((a-c)*(a-c) + 4.0f*b*b) - (a+c);
	float rx = sqrtf(q / (r * s));
	float ry = sqrtf(q / (-r * s));

	float phi = 0.0f;
	if (b == 0.0f)
	{
		if (a < c)
			phi = 0.0f;
		else
			phi = (float)FMath::piHalf;
	}
	else
	{
		if (a < c)
			phi = 0.5f * atanf((a - c) / (2.0f * b));
		else
			phi = (float)FMath::piHalf + 0.5f * atanf((a - c) / (2.0f * b));
	}

	m_ellipse.setCenter(FVector2f(cx, cy));
	m_ellipse.setRadii(FVector2f(rx, ry));
	m_ellipse.setTiltAngle(phi);
}

void FContour::_fitEllipse()
{
	m_ellipse.setCenter(m_barycenter);
	m_ellipse.setRadii(m_boundingBox.size());
	m_ellipse.setTiltAngle(0.0f);

	float deltaParams[] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	float opts[] = { 1e-2, 1e-17, 1e-17, 1e-17, 1e-2 }; // tau, eps1, eps2, eps3, delta
	float lmInfo[LM_INFO_SZ];
	float* pBuffer = FGlobalConstants::pLevmarDetectionWorkspace;

	int iter = slevmar_dif(FContour::sLevmarUpdate,
		deltaParams, NULL, 5, length, 80, opts, lmInfo, pBuffer, NULL, (void*)this, 0, 0.0);

	/*
	F_TRACE(QString("Ellipse fit, contour #%0, it: %1, %2").arg(m_identity)
		.arg(iter).arg(FLevmarTermReason((int)lmInfo[6]).toString()));
	*/

	m_ellipse.setCenter(FVector2f(m_ellipse.center().x() + deltaParams[0] * 100.0,
							    m_ellipse.center().y() + deltaParams[1] * 100.0));
	m_ellipse.setRadii(FVector2f(m_ellipse.radii().x() + deltaParams[2] * 100.0,
							   m_ellipse.radii().y() + deltaParams[3] * 100.0));
	m_ellipse.setTiltAngle(m_ellipse.tiltAngle() + deltaParams[4]);
}

void FContour::_levmarUpdate(float* p, float* hx, int m, int n)
{
	F_ASSERT(m == 5 && n == length);

	FEllipse2f de;
	de.setCenter(FVector2f(m_ellipse.center().x() + p[0] * 100.0, m_ellipse.center().y() + p[1] * 100.0));
	de.setRadii(FVector2f(m_ellipse.radii().x() + p[2] * 100.0, m_ellipse.radii().y() + p[3] * 100.0));
	de.setTiltAngle(m_ellipse.tiltAngle() + p[4]);

	for (int i = 0; i < length; i++)
	{
		float d = de.signedDistanceTo(pos[i]);
		hx[i] = d;
	}
}

void FContour::_dominantAngle(int nx, int ny)
{
	float meanX = m_barycenter.x();
	float meanY = m_barycenter.y();

	// find dominant angle
	float angle = 0.0f;
	float peak = 0.0f;
	float scale = 1.0f / (float)nx;

	for (float a = 0.0f; a < 360.0f; a++)
	{
		float sin_a = sinf(FMath::deg2rad(a));
		float cos_a = cosf(FMath::deg2rad(a));
		float sum = 0.0f;
		float count = 0.0f;

		for (int i = 0; i < length; i++)
		{
			float x = (pos[i].x() - meanX) * scale;
			float y = (pos[i].y() - meanY) * scale;

			float tx = x * cos_a - y * sin_a;
			float ty = x * sin_a + y * cos_a;

			if (tx > 0.0f && ty > 0.0f)
			{
				sum += tx * tx * tx;
				count++;
			}
		}

		sum /= count;
		if (sum > peak)
		{
			peak = sum;
			angle = a;
		}
	}

	angle = FMath::deg2rad(angle);
	m_orientation.set(cosf(angle) * 50.0f, sinf(angle) * 50.0f);

	// rotate by negative dominant angle (normalize rotation)
	float sin_inv_a = sinf(-angle);
	float cos_inv_a = cosf(-angle);

	for (int i = 0; i < length; i++)
	{
		float x = pos[i].x() - meanX;
		float y = pos[i].y() - meanY;
		float tx = x * cos_inv_a - y * sin_inv_a;
		float ty = x * sin_inv_a + y * cos_inv_a;
		pos[i].set(tx, ty);
	}

	// find boundaries in normalized orientation
	float minTX = FLT_MAX;
	float minTY = FLT_MAX;
	float maxTX = 0.0f;
	float maxTY = 0.0f;

	for (int i = 0; i < length; i++)
	{
		float x = pos[i].x();
		float y = pos[i].y();
		minTX = fMin(minTX, x);
		minTY = fMin(minTY, y);
		maxTX = fMax(maxTX, x);
		maxTY = fMax(maxTY, y);
	}

	// rotate back
	float sin_a = sinf(angle);
	float cos_a = cosf(angle);

	m_quadCoords[0].set((minTX * cos_a - minTY * sin_a) + meanX,
		(minTX * sin_a + minTY * cos_a) + meanY);
	m_quadCoords[1].set((maxTX * cos_a - minTY * sin_a) + meanX,
		(maxTX * sin_a + minTY * cos_a) + meanY);
	m_quadCoords[2].set((maxTX * cos_a - maxTY * sin_a) + meanX,
		(maxTX * sin_a + maxTY * cos_a) + meanY);
	m_quadCoords[3].set((minTX * cos_a - maxTY * sin_a) + meanX,
		(minTX * sin_a + maxTY * cos_a) + meanY);
}
