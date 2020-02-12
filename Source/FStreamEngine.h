// ----------------------------------------------------------------------------------------------------
//  Title			FStreamEngine.h
//  Description		Header file for FStreamEngine.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-10 22:55:38 +0200 (Sa, 10 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FSTREAMENGINEBASE_H
#define FSTREAMENGINEBASE_H

#include <QObject>
#include "FTrackMe.h"
#include "FlowMath.h"
#include "FlowGL.h"

#include "FLineTracker.h"
#include "FPoseDetector.h"
#include "FCamera.h"
#include "FFrameStatistics.h"

class FFernTracker;
class FLineModel;
class FDetectorThread;

// ----------------------------------------------------------------------------------------------------
//  Class FStreamEngine
// ----------------------------------------------------------------------------------------------------

class FStreamEngine : public QObject
{
	Q_OBJECT;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FStreamEngine(QObject* pParent = NULL);
	/// Virtual destructor.
	virtual ~FStreamEngine();

	//  Public commands --------------------------------------------------------

public:
	/// Resets the engine and specifies the frame size.
	void reset(const QSize& frameSize);

	/// Processes the given frame.
	void processFrame(const FGLTextureRect& inputFrame, FFrameStatistics* pStats = NULL);
	/// Sets the tracker to its initial pose.
	void resetPose();

	/// Starts finding consistent keypoints on the model.
	void startKeypointDetection();
	/// Ends finding consistent keypoints.
	void endKeypointDetection();
	/// Learns keypoints from the given image.
	void learnKeypointDetection(const FGLTextureRect& inputFrame);

	/// Starts training of the model appearance.
	void startAppearanceTraining();
	/// Ends training of the model appearance.
	void endAppearanceTraining();
	/// Learns the model appearance.
	void learnAppearance(const FGLTextureRect& inputFrame);

	/// Draws the solid model used for tracking.
	void drawSolidModel();
	/// Draws and returns an overlay texture with the augmented image.
	const FGLTextureRect& drawAugmentedImage();

	/// Sets the line model to be used for tracking.
	void setLineModel(FLineModel* pModel);

	//  Public queries ---------------------------------------------------------

	/// Returns the camera containing the internal calibration and external pose.
	const FCamera& camera() const { return m_camera; }

	/// Returns true if the engine wants to redraw.
	inline bool wantRedraw() {
		bool result = m_wantRedraw; m_wantRedraw = false; return result; }

	/// Returns the current input frame.
	const FGLTextureRect& currentFrame() const {
		return m_texPreprocessed[m_frameIndex];
	}
	/// Returns the previous input frame.
	const FGLTextureRect& previousFrame() const;
	/// Returns the input frame at t - index.
	const FGLTextureRect& frameAt(size_t index) const;

	/// Returns the edge model used for tracking.
	FLineModel* lineModel() const { return m_pLineTracker->model(); }

	//  OUTPUT FROM LINE TRACKING

	/// Returns the depth texture from solid model rendering.
	const FGLTextureRect& trackerDepthPass() const {
		return m_pLineTracker->depthPass();
	}
	/// Creates and returns an overlay texture with the predicted line model,
	/// search lines and edge candidate points.
	const FGLTextureRect& trackerInitialModel() const {
		return m_pLineTracker->initialModel();
	}
	/// Creates and returns an overlay texture with the fitted line model.
	const FGLTextureRect& trackerFittedModel() const {
		return m_pLineTracker->fittedModel();
	}
	/// Creates and returns an overlay texture with the reference colors.
	const FGLTextureRect& trackerReferenceColors() const {
		return m_pLineTracker->referenceColors();
	}

	//  OUTPUT FROM POSE DETECTION

	/// Returns true if the detector is idle.
	bool isPoseDetectorIdle() const;

	/// Returns the result from canny edge detection.
	const FGLTextureRect& cannyEdges() const {
		return m_pPoseDetector->cannyEdges();
	}
	/// Returns the result from distance transform.
	const FGLTextureRect& distanceTransform() const {
		return m_pPoseDetector->distanceTransform();
	}
	/// Returns the result from contour detection.
	const FGLTextureRect& contourView() {
		return m_pPoseDetector->contourView();
	}
	/// Draws the contour statistics.
	void drawContourStatistics(FGLCanvas& canvas) {
		m_pPoseDetector->drawContourStatistics(canvas);
	}
	/// Draws the detected contour patches.
	void drawDetectionMap() {
		m_pPoseDetector->drawDetectionMap();
	}
	/// Draws the detected pose.
	void drawDetectedPose() {
		m_pPoseDetector->drawDetectedPose();
	}

	//  Parameters -------------------------------------------------------------

public slots:
	void loadAugmentationModel(QString modelFilePath);
	void setAugmentedPosition(FVector3d position);
	void setAugmentedRotation(FVector3d rotation);
	void setAugmentedScale(double scale);

	void loadLineModel(QString modelFilePath);

	void setCameraOverride(bool state);
	void setCameraRadialDistortion(FVector2d factor);
	void setCameraAperture(FVector2d aperture);
	void setCameraFocalDistance(double val);
	void setCameraTranslation(FVector3d translation);
	void setCameraRotation(FVector3d rotation);

	void setSearchRange(double val);
	void setSamplingDistance(double val);
	void setSamplingAdaptiveDensity(double val);
	void setMotionCompensation(double val);
	void setTrackerSurfaceAngleLimit(double val);
	void setTrackerContourAngleLimit(double val);
	
	void setEdgeLumaWeight(double val);
	void setEdgeChromaWeight(double val);
	void setEdgeThreshold(double val);
	
	void setColorToleranceEnabled(bool state);
	void setColorTolerance(double val);
	void setColorAdaptability(double val);
	void setColorPeekDistance(double val);
	void setColorFullEdgeThreshold(double val);
	void setColorHalfEdgeThreshold(double val);

	void setFocalDistanceEnabled(bool val);
	void setTrackerInitializationThreshold(double val);
	void setTrackerFailureThreshold(double val);

	void setMultipleHypothesesEnabled(bool val);
	void setTrackerPredictionFactor(double val);
	void setTrackerInterpolationRate(FVector2d val);
	void setTrackerEstimatorType(int val);
	void setTrackerEstimatorLimit(double val);
	void setTrackerRejectionFactorA(double val);
	void setTrackerRejectionFactorB(double val);

	void setDetectionEnabled(bool val);
	void setDetectionAlwaysOn(bool val);

	void setEdgeThresholdLow(double val);
	void setEdgeThresholdHigh(double val);

	void setDetectionMSEThreshold(double val);
	void setDetectionFixedTypeId(int val);
	void setDetectionContourPosition(FVector3d position);
	void setDetectionContourRotation(FVector3d rotation);
	void setDetectionContourScale(double scale);

	void loadClassifierDatabase(QString filePath);

	//  Internal functions -----------------------------------------------------

private:
	void _initGL();
	void _resetGL();
	void _radialUndistort();
	void _buildAugmentedMatrixMVP();

	//  Internal data members --------------------------------------------------

protected:
	QSize m_frameSize;
	FGLTextureRect m_inputFrame;

	FGLProgram m_prgUndistort;
	FGLSampler m_sampUndistort;
	FGLFramebuffer m_fbPreprocess;
	static const size_t FRAME_BUFFER_SIZE = 5;
	FGLTextureRect m_texPreprocessed[FRAME_BUFFER_SIZE];
	FGLOverlayRect m_overlay;
	size_t m_frameIndex;

	FVector2f m_undistFactor;
	int m_uUndistFactor;
	int m_suSourceFrame;

	FGLFramebuffer m_fbAugmentedModel;
	FGLTextureRect m_texAugmentedModelColor;
	FGLTextureRect m_texAugmentedModelDepth;

	FGLProgram m_prgModelDepth;
	FGLProgram m_prgModelColor;
	FGLProgram m_prgModelTextured;
	FGLBuffer m_bufModelTransform;
	int m_uTextureEnabled;

	FGLMesh m_augmentedModel;
	FMatrix4f m_augmentedTranslation;
	FVector3f m_augmentedPosition;
	FVector3f m_augmentedRotation;
	float m_augmentedScale;


	FCamera m_camera;
	FLineTracker* m_pLineTracker;
	FPoseDetector* m_pPoseDetector;
	FDetectorThread* m_pDetectorThread;
	FDTPixel* m_pDTBuffer;

	bool m_detectionEnabled;
	bool m_detectionAlwaysOn;
	bool m_wantRedraw;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FSTREAMENGINEBASE_H