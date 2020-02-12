// ----------------------------------------------------------------------------------------------------
//  Title			FCameraMetrics.cpp
//  Description		Implementation of class FCameraMetrics
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-17 00:16:08 +0200 (Mi, 17 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "FGLMath.h"

#include "FCameraMetrics.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FCameraMetrics
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FCameraMetrics::FCameraMetrics()
: m_apertureSize(36.0f, 27.0f),
  m_resolution(640.0f, 480.0f),
  m_pixelShift(320.0f, 240.0f),
  m_focalLength(35.0f)
{
}

// Public commands ------------------------------------------------------------------------------------

// Public queries -------------------------------------------------------------------------------------

void FCameraMetrics::getGLProjectionMatrix(FMatrix4f& matGLProjection) const
{
	float zNear = 1.0f;
	float width = zNear * m_apertureSize.x() / m_focalLength;
	float height = zNear * m_apertureSize.y() / m_focalLength;
	FGLMath::makeProjectionPerspectiveRH(matGLProjection, width, height, zNear, 1000.0f);
}

void FCameraMetrics::getProjectionMatrix(FMatrix4f& matProjection) const
{
	float a1 = m_focalLength * m_resolution.x() / m_apertureSize.x();
	float a2 = m_focalLength * m_resolution.y() / m_apertureSize.y();

	matProjection.makeZero();
	matProjection(0, 0) = a1;
	matProjection(1, 1) = a2;
	matProjection(0, 2) = m_pixelShift.x();
	matProjection(1, 2) = m_pixelShift.y();
	matProjection(2, 2) = 1.0f;
}

void FCameraMetrics::getProjectiveCameraMatrix(FMatrix3f& matCamera) const
{
	float a1 = m_focalLength * m_resolution.x() / m_apertureSize.x();
	float a2 = m_focalLength * m_resolution.y() / m_apertureSize.y();
	
	matCamera.makeIdentity();
	matCamera(0, 0) = a1;
	matCamera(1, 1) = a2;
	matCamera(0, 2) = m_pixelShift.x();
	matCamera(1, 2) = m_pixelShift.y();
}

void FCameraMetrics::getInverseProjectiveCameraMatrix(FMatrix3f& matInverseCamera) const
{
	float ia1 = m_apertureSize.x() / (m_focalLength * m_resolution.x());
	float ia2 = m_apertureSize.y() / (m_focalLength * m_resolution.y());

	matInverseCamera.makeIdentity();
	matInverseCamera(0, 0) = ia1;
	matInverseCamera(1, 1) = ia2;
	matInverseCamera(0, 2) = -m_pixelShift.x() * ia1;
	matInverseCamera(1, 2) = -m_pixelShift.y() * ia2;
}

void FCameraMetrics::dump(QDebug& debug) const
{
	debug << "\n\n----- FCameraMetrics -----";
	debug << "\n   Focal length:     " << m_focalLength;
	debug << "\n   Aperture size:    " << m_apertureSize.toString();
	debug << "\n   Image resolution: " << m_resolution.toString();
	debug << "\n   Image center:     " << m_pixelShift.toString();
}

// ----------------------------------------------------------------------------------------------------