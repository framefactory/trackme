// ----------------------------------------------------------------------------------------------------
//  Title			FCamera.cpp
//  Description		Implementation of class FCamera
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "FGLMath.h"

#include "FCamera.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FCamera
// ----------------------------------------------------------------------------------------------------

const double FCamera::FD_MULTIPLIER = 1.0;

// Constructors and destructor ------------------------------------------------------------------------

FCamera::FCamera()
: m_frameIndex(0),
  m_focalDistanceEnabled(false)
{
	m_imageSize.set(768, 576);

	m_focalDistance = 35.0f;
	m_apertureSize.set(36.0f, 24.0f);
	m_imagePlaneOffset.makeZero();
	m_rotation.makeZero();
	m_translation.makeZero();

	for (size_t i = 0; i < HISTORY_SIZE; i++)
		m_isValid[i] = false;
}

FCamera::~FCamera()
{
}

// Public commands ------------------------------------------------------------------------------------

void FCamera::resetPose()
{
	for (size_t i = 0; i < HISTORY_SIZE; i++)
		m_isValid[i] = false;

	m_frameIndex = 0;

	m_pose[0][0] = m_translation.x();
	m_pose[0][1] = m_translation.y();
	m_pose[0][2] = m_translation.z();
	m_pose[0][3] = m_rotation.x();
	m_pose[0][4] = m_rotation.y();
	m_pose[0][5] = m_rotation.z();
	m_pose[0][6] = m_focalDistance * FD_MULTIPLIER;

	for (int i = 0; i < NUM_PARAM; i++)
	{
		m_poseStart[i] = m_poseExtra[i] = m_poseSmooth[0][i] = m_pose[0][i];
		m_poseDelta[i] = 0.0;
	}
}

void FCamera::resetPose(const FCameraPose& pose)
{
	for (size_t i = 0; i < HISTORY_SIZE; i++)
		m_isValid[i] = false;

	m_frameIndex = 0;

	m_pose[0][0] = pose.translation().x();
	m_pose[0][1] = pose.translation().y();
	m_pose[0][2] = pose.translation().z();
	m_pose[0][3] = pose.rotation().x() * FMath::r2d;
	m_pose[0][4] = pose.rotation().y() * FMath::r2d;
	m_pose[0][5] = pose.rotation().z() * FMath::r2d;
	m_pose[0][6] = m_focalDistance * FD_MULTIPLIER;

	for (int i = 0; i < NUM_PARAM; i++)
	{
		m_poseStart[i] = m_poseExtra[i] = m_poseSmooth[0][i] = m_pose[0][i];
		m_poseDelta[i] = 0.0;
	}
}

void FCamera::updatePose(double* pParam)
{
	size_t f = m_frameIndex;
	size_t np = m_focalDistanceEnabled ? NUM_PARAM : NUM_PARAM - 1;

	for (size_t i = 0; i < np; i++)
	{
		m_poseDelta[i] = pParam[i];
		m_pose[f][i] = m_poseExtra[i] + m_poseDelta[i];
	}

	m_isValid[f] = true;
}

void FCamera::smoothResult(double smoothFactor)
{
	size_t t0 = m_frameIndex;
	size_t t1 = (m_frameIndex + HISTORY_SIZE - 1) % HISTORY_SIZE;
	size_t t2 = (m_frameIndex + HISTORY_SIZE - 2) % HISTORY_SIZE;
	size_t t3 = (m_frameIndex + HISTORY_SIZE - 3) % HISTORY_SIZE;
	size_t t4 = (m_frameIndex + HISTORY_SIZE - 4) % HISTORY_SIZE;
	size_t t5 = (m_frameIndex + HISTORY_SIZE - 5) % HISTORY_SIZE;
	size_t t6 = (m_frameIndex + HISTORY_SIZE - 6) % HISTORY_SIZE;
	size_t t7 = (m_frameIndex + HISTORY_SIZE - 7) % HISTORY_SIZE;
	size_t t8 = (m_frameIndex + HISTORY_SIZE - 8) % HISTORY_SIZE;

	for (size_t i = 0; i < NUM_PARAM; i++)
		m_pose[t0][i] = m_poseExtra[i] + (1.0 - smoothFactor) * m_poseDelta[i];

	if (m_isValid[t1] && m_isValid[t2])
	{
		for (size_t i = 0; i < NUM_PARAM; i++)
			/*
			m_poseSmooth[t0][i] = (1.0 / 9.0)
				* (m_pose[t0][i] + m_pose[t1][i] + m_pose[t2][i] + m_pose[t3][i] + m_pose[t4][i]
				 + m_pose[t5][i] + m_pose[t6][i] + m_pose[t7][i] + m_pose[t8][i]);
			*/
			
			 m_poseSmooth[t0][i] = (1.0 / 5.0)
			 * (m_pose[t0][i] + m_pose[t1][i] + m_pose[t2][i] + m_pose[t3][i] + m_pose[t4][i]);
			
	}
	else
	{
		for (size_t i = 0; i < NUM_PARAM; i++)
			m_poseSmooth[t0][i] = m_pose[t0][i];
	}
}

void FCamera::advanceFrame(float motionPredictionFactor)
{
	size_t h1 = m_frameIndex;
	size_t h2 = (m_frameIndex - 2 + HISTORY_SIZE) % HISTORY_SIZE;

	m_frameIndex = (m_frameIndex + 1) % HISTORY_SIZE;
	size_t f = m_frameIndex;

	/*
	F_TRACE("Advance frame, optimization completed:");
	F_TRACE(QString("Pos x:%1, y:%2, z:%3, Rot: %4, %5, %6")
		.arg(m_pose[h1][0]).arg(m_pose[h1][1]).arg(m_pose[h1][2])
		.arg(m_pose[h1][3]).arg(m_pose[h1][4]).arg(m_pose[h1][5]));
	F_TRACE(QString("Focal distance: %1").arg(m_pose[h1][6] / FD_MULTIPLIER));
	F_TRACE("");
	*/

	for (int i = 0; i < NUM_PARAM; i++)
	{
		m_poseStart[i] = m_pose[f][i] = m_pose[h1][i];
		m_poseDelta[i] = 0.0;
		m_poseSmooth[f][i] = m_poseSmooth[h1][i];
	}

	if (motionPredictionFactor > 0.0f && m_isValid[h1] && m_isValid[h2])
	{
		for (int i = 0; i < NUM_PARAM; i++)
		{
			m_poseExtra[i] = m_poseStart[i]
				+ motionPredictionFactor * 0.5 * (m_pose[h1][i] - m_pose[h2][i]);
		}
	}
	else
	{
		for (int i = 0; i < NUM_PARAM; i++)
			m_poseExtra[i] = m_poseStart[i];
	}
}

// Public queries -------------------------------------------------------------------------------------

void FCamera::getModelViewStart(FMatrix4f& matMV) const
{
	_generateModelViewMatrix(matMV, m_poseStart);
}

void FCamera::getProjectionStart(FMatrix4f& matP) const
{
	_generateProjectionMatrix(matP, m_poseStart[6]);
}

void FCamera::getProjectionGLStart(FMatrix4f& matPGL) const
{
	_generateProjectionMatrixGL(matPGL, m_poseStart[6]);
}

void FCamera::getModelViewExtra(FMatrix4f& matMV) const
{
	_generateModelViewMatrix(matMV, m_poseExtra);
}

void FCamera::getProjectionExtra(FMatrix4f& matP) const
{
	_generateProjectionMatrix(matP, m_poseExtra[6]);
}

void FCamera::getProjectionGLExtra(FMatrix4f& matPGL) const
{
	_generateProjectionMatrixGL(matPGL, m_poseExtra[6]);
}

void FCamera::getModelViewCurrent(FMatrix4f& matMV) const
{
	_generateModelViewMatrix(matMV, m_pose[m_frameIndex]);
}

void FCamera::getProjectionCurrent(FMatrix4f& matP) const
{
	_generateProjectionMatrix(matP, m_pose[m_frameIndex][6]);
}

void FCamera::getProjectionGLCurrent(FMatrix4f& matPGL) const
{
	_generateProjectionMatrixGL(matPGL, m_pose[m_frameIndex][6]);
}

void FCamera::getModelViewSmooth(FMatrix4f& matMV) const
{
	_generateModelViewMatrix(matMV, m_poseSmooth[m_frameIndex]);
}

void FCamera::getProjectionSmooth(FMatrix4f& matP) const
{
	_generateProjectionMatrix(matP, m_poseSmooth[m_frameIndex][6]);
}

void FCamera::getProjectionGLSmooth(FMatrix4f& matPGL) const
{
	_generateProjectionMatrixGL(matPGL, m_poseSmooth[m_frameIndex][6]);
}

double FCamera::poseCurrent(size_t index)
{
	if (index == 6)
		return m_pose[m_frameIndex][6] / FD_MULTIPLIER;
	else
		return m_pose[m_frameIndex][index];
}

double FCamera::poseSmooth(size_t index)
{
	if (index == 6)
		return m_poseSmooth[m_frameIndex][6] / FD_MULTIPLIER;
	else
		return m_poseSmooth[m_frameIndex][index];

}

// Internal functions ---------------------------------------------------------------------------------

void FCamera::_generateModelViewMatrix(FMatrix4f& matMV, const double* pParam) const
{
	FVector3f translation(pParam[0], pParam[1], pParam[2]);
	FVector3f rotation(pParam[3], pParam[4], pParam[5]);
	rotation *= 0.017453293f;
	float angle = rotation.length();
	if (angle > 0.0f)
		rotation /= angle;

	matMV.makeRotation(rotation, angle);
	matMV.setTranslation(translation);
}

void FCamera::_generateProjectionMatrix(FMatrix4f& matProj, double focalDistance) const
{
	focalDistance /= FD_MULTIPLIER;
	float aspect = m_imageSize.x() / m_imageSize.y();

	matProj.makeZero();
	matProj[0][0] = (float)focalDistance * m_imageSize.x() / m_apertureSize.x();
	matProj[1][1] = (float)focalDistance * m_imageSize.y() / m_apertureSize.y();
	matProj[0][2] = -(m_imagePlaneOffset.x() + m_imageSize.x() * 0.5f);
	matProj[1][2] = -(m_imagePlaneOffset.y() + m_imageSize.y() * 0.5f);
	matProj[2][2] = -1.0f; // needed to track z value for clipping the line model
	matProj[3][2] = -1.0f;
}

void FCamera::_generateProjectionMatrixGL(FMatrix4f& matProjGL, double focalDistance) const
{
	focalDistance /= FD_MULTIPLIER;
	float invAspect = m_apertureSize.y() / m_apertureSize.x();
	float zNear = 1.0f;
	float width = zNear * m_apertureSize.x() / focalDistance;
	float height = width * invAspect;
	
	FGLMath::makeProjectionPerspectiveRH(matProjGL, width, height, zNear, 1000.0f);
}

// ----------------------------------------------------------------------------------------------------