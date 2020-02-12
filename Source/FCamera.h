// ----------------------------------------------------------------------------------------------------
//  Title			FCamera.h
//  Description		Header file for FCamera.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FCAMERA_H
#define FCAMERA_H

#include "FTrackMe.h"
#include "FlowMath.h"
#include "FCameraPose.h"

// ----------------------------------------------------------------------------------------------------
//  Class FCamera
// ----------------------------------------------------------------------------------------------------

class FCamera
{
	//  Static members ---------------------------------------------------------

private:
	static const double FD_MULTIPLIER;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FCamera();
	/// Virtual destructor.
	virtual ~FCamera();

	//  Public commands --------------------------------------------------------

public:
	/// Resets the camera parameters to the initial values.
	void resetPose();
	/// Resets the camera pose to the given values.
	void resetPose(const FCameraPose& pose);
	/// Updates the intermediate pose and matrices from the given motion parameters.
	void updatePose(double* pMotionParameters);
	/// Smoothing of the resulting pose. 0.0 uses the calculated pose, 1.0 uses the predicted pose.
	void smoothResult(double smoothFactor);
	/// Advances to the next frame.
	void advanceFrame(float motionPredictionFactor);

	/// Sets the initial calibration parameters of the camera.
	void setAperture(const FVector2f& apertureSize) {
		m_apertureSize = apertureSize;
	}
	void setFocalDistance(float focalDistance) {
		m_focalDistance = focalDistance;
	}
	void setTranslation(const FVector3f& translation) {
		m_translation = translation;
	}
	void setRotation(const FVector3f& rotation) {
		m_rotation = rotation;
	}

	/// Sets the size of the image rectangle in pixels.
	void setImageSize(const FVector2f& imageSize) {
		m_imageSize = imageSize;
	}
	void setImagePlaneOffset(const FVector2f& imagePlaneOffset) {
		m_imagePlaneOffset = imagePlaneOffset;
	}

	/// Enables or disables the dynamic adaptation of the focal distance.
	/// If set to false, the focal distance remains fixed at its initial value.
	void setFocalDistanceEnabled(bool state) {
		m_focalDistanceEnabled = state;
	}

	//  Public queries ---------------------------------------------------------

	void getModelViewStart(FMatrix4f& matMV) const;
	void getProjectionStart(FMatrix4f& matP) const;
	void getProjectionGLStart(FMatrix4f& matPGL) const;

	void getModelViewExtra(FMatrix4f& matMV) const;
	void getProjectionExtra(FMatrix4f& matP) const;
	void getProjectionGLExtra(FMatrix4f& matPGL) const;

	void getModelViewCurrent(FMatrix4f& matMV) const;
	void getProjectionCurrent(FMatrix4f& matP) const;
	void getProjectionGLCurrent(FMatrix4f& matPGL) const;

	void getModelViewSmooth(FMatrix4f& matMV) const;
	void getProjectionSmooth(FMatrix4f& matP) const;
	void getProjectionGLSmooth(FMatrix4f& matPGL) const;

	double poseCurrent(size_t index);
	double poseSmooth(size_t index);

	/// Returns the size of the image rectangle in pixels.
	inline const FVector2f& imageSize() const { return m_imageSize; }

	/// Returns true if the focal distance is included in estimation.
	bool focalDistanceEnabled() const { return m_focalDistanceEnabled; }
	
	//  Internal functions -----------------------------------------------------

private:
	void _generateModelViewMatrix(FMatrix4f& matMV, const double* pParam) const;
	void _generateProjectionMatrix(FMatrix4f& matProj, double fovY) const;
	void _generateProjectionMatrixGL(FMatrix4f& matProjGL, double fovY) const;

	//  Internal data members --------------------------------------------------

private:
	FVector2f m_imageSize;
	FVector2f m_imagePlaneOffset;
	FVector2f m_apertureSize;

	// Initial calibration and pose
	FVector3f m_translation;
	FVector3f m_rotation;
	float m_focalDistance;

	// Current pose and history
	static const size_t NUM_PARAM = 7;
	static const size_t HISTORY_SIZE = 12;
	size_t m_frameIndex;

	bool m_isValid[HISTORY_SIZE];
	double m_pose[HISTORY_SIZE][NUM_PARAM]; // history of final pose estimates
	double m_poseSmooth[HISTORY_SIZE][NUM_PARAM]; // history of smoothed pose

	double m_poseStart[NUM_PARAM]; // initial estimate for current frame
	double m_poseExtra[NUM_PARAM]; // extrapolated start pose
	double m_poseDelta[NUM_PARAM]; // pose delta calculated by solver

	bool m_focalDistanceEnabled;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FCAMERA_H