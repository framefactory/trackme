// ----------------------------------------------------------------------------------------------------
//  Title			FLineTracker.h
//  Description		Header file for FLineTracker.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FLINETRACKER_H
#define FLINETRACKER_H

#include "FTrackMe.h"
#include "FlowMath.h"
#include "FlowGL.h"
#include "FPixelStruct.h"

#include "FLineModel.h"
#include "FCamera.h"
#include "FStopWatch.h"
#include "FFrameStatistics.h"
#include "FLineTrackerState.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLineTracker
// ----------------------------------------------------------------------------------------------------

/// Adaptive line-tracking algorithm. Single-threaded.
/// Expects the current and previous frames as OpenGL rect textures.
class FLineTracker
{
	//  Static callbacks -------------------------------------------------------

private:
	inline static void sLevmarUpdate(double* p, double* hx, int m, int n, void* pData) {
		F_ASSERT(pData);
		((FLineTracker*)pData)->_levmarUpdate(p, hx, m, n);
	}
	inline static void sLevmarJacobian(double* p, double* j, int m, int n, void* pData) {
		F_ASSERT(pData);
		((FLineTracker*)pData)->_levmarJacobian(p, j, m, n);
	}

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FLineTracker();
	/// Virtual destructor.
	virtual ~FLineTracker();

	//  Public commands --------------------------------------------------------

public:
	/// Runs the edge candidate search on the GPU.
	void searchCandidates(const FGLTextureRect& currentFrame, const FGLTextureRect& previousFrame,
		FTrackerStatistics* pStats = NULL);
	/// Optimizes the pose based on the candidates found in the previous step.
	void optimizePose();

	/// Resets the tracker and prepares for the given frame size.
	void reset(const QSize& frameSize);

	/// Sets the camera to be used for model projection.
	void setCamera(FCamera* pCamera);
	/// Sets the edge model to be used for tracking.
	void loadModel(const QString& modelFilePath);

	/// Sets the rotation matrix for model positioning.
	void setModelTransform(const FMatrix4f& modelTransform);
	/// Enables/disables tracking.
	void setTrackingEnabled(bool state);

	/// Draws the solid model.
	void drawSolidModel();

	//  Public queries ---------------------------------------------------------

	/// Returns the state of the tracker.
	FLineTrackerState state() const { return m_trackerState; }
	/// Returns the last optimization error.
	float error() const { return m_trackingError; }

	/// Returns the depth texture from solid model rendering.
	const FGLTextureRect& depthPass() const { return m_fbTexModelDepth; }
	/// Creates and returns an overlay texture with the predicted line model,
	/// search lines and edge candidate points.
	const FGLTextureRect& initialModel();
	/// Creates and returns an overlay texture with the fitted line model.
	const FGLTextureRect& fittedModel();
	/// Creates and returns an overlay texture with the reference sample colors.
	const FGLTextureRect& referenceColors();

	/// Returns the edge model used for tracking.
	FLineModel* model() const { return m_pModel; }

	//  Parameter --------------------------------------------------------------

	void setSearchRange(double val) { m_searchRange = val; }
	void setSamplingDistance(double val) { m_samplingDistance = val; }
	void setSamplingAdaptiveDensity(double val) { m_sampleAdaptiveDensity = val; }
	void setMotionCompensation(double val) { m_motionCompensation = val; }
	void setSurfaceAngleLimit(double val) { m_surfaceAngleLimit = val; }
	void setContourAngleLimit(double val) { m_contourAngleLimit = val; }
	void setEdgeLumaWeight(double val) { m_edgeLumaWeight = val; }
	void setEdgeChromaWeight(double val) { m_edgeChromaWeight = val; }
	void setEdgeThreshold(double val) { m_edgeThreshold = val; }
	void setColorToleranceEnabled(bool state) { m_colorToleranceEnabled = state; }
	void setColorTolerance(double val)  { m_colorTolerance = val; }
	void setColorAdaptability(double val) { m_colorAdaptability = val; }
	void setColorPeekDistance(double val) { m_colorPeekDistance = val; }
	void setColorFullEdgeThreshold(double val) { m_colorFullEdgeThreshold = val; }
	void setColorHalfEdgeThreshold(double val) { m_colorHalfEdgeThreshold = val; }
	void setInitializationThreshold(double val) { m_initializationErrorThreshold = val; }
	void setFailureThreshold(double val) { m_failureErrorThreshold = val; }
	void setMultipleHypothesesEnabled(bool state);
	void setPredictionFactor(double val) { m_motionPredictionFactor = val; }
	void setInterpolationRate(const FVector2f& val) { m_interpolationRate = val; }
	void setEstimatorType(int val) { m_estimatorType = val; }
	void setEstimatorLimit(double val) { m_estimatorLimit = val; }
	void setRejectionFactorA(double val) { m_rejectionFactorA = val; }
	void setRejectionFactorB(double val) { m_rejectionFactorB = val; }

	//  Internal functions -----------------------------------------------------

private:
	void _initParameters();
	void _initGL();
	void _frameSizeChanged_resetGL();
	void _modelChanged_resetGL();

	void _searchCandidates();
	float _optimizePose();
	void _updateColorStatistics();
	void _resetColorStatistics();

	void _drawInitialPose();
	void _drawFittedPose();
	void _drawReferenceColors();

	void _levmarUpdate(double* p, double* hx, int m, int n);
	void _levmarJacobian(double* p, double* j, int m, int n);

	void _renderFilter();
	void _calculateBlurFilter(float sigmaParallel, float sigmaOrthogonal);

	//  Internal data members --------------------------------------------------

protected:
	QSize m_frameSize;
	QSize m_transferSize;

	FCamera* m_pCamera;
	FLineModel* m_pModel;

	// State
	FLineTrackerState m_trackerState;
	float m_trackingError;
	FMatrix4f m_matMVPGL_Previous;
	FMatrix4f m_matMVPGL_Previous2;
	bool m_usePrevPose;

	// OpenGL
	FGLTextureRect m_currentFrame;
	FGLTextureRect m_previousFrame;

	FGLOverlayRect m_overlayRect;
	FGLOverlayRect m_transferRect;

	FGLProgram m_prgModelDepth;
	FGLTextureRect m_fbTexModelColor;
	FGLTextureRect m_fbTexModelDepth;
	FGLFramebuffer m_fbModelDepthColor;

	FGLProgram m_prgModelSample;
	FGLProgram m_prgModelEdgeSuppress;
	int m_uImageSize;
	int m_uTransferSize;
	int m_suDepthModel;
	int m_suImage;
	int m_suPrevImage;
	int m_suFilterKernel;
	int m_suColorBuffer1;
	int m_suColorBuffer2;
	int m_suResidualData;

	FGLSampler m_samplerLinear;
	FGLSampler m_samplerNearest;

	int m_bufIndex;
	FGLTextureRect m_fbTexModelSearchIntermediate[2];
	FGLFramebuffer m_fbModelSearchIntermediate;
	FGLTextureRect m_fbTexModelSearchResult;
	FGLFramebuffer m_fbModelSearchResult;

	float* m_pResidualData;
	FGLBuffer m_bufResidualData;
	FGLTextureBuffer m_texResidualData;

	FPixelRGBA32f* m_pSearchResult;

	FGLBuffer m_bufModelTransform;

	FGLBuffer m_bufBlurFilter;
	FGLTextureBuffer m_texBlurFilter;
	size_t m_blurFilterSteps;
	size_t m_blurFiterWidth;
	float m_sigmaParallel;
	float m_sigmaOrthogonal;
	float* m_pBlurFilter;

	// parameter values
	float     m_searchRange;
	float     m_samplingDistance;
	float     m_sampleAdaptiveDensity;
	float     m_motionCompensation;
	float     m_surfaceAngleLimit;
	float     m_contourAngleLimit;
	float     m_edgeLumaWeight;
	float     m_edgeChromaWeight;
	float     m_edgeThreshold;
	float     m_colorTolerance;
	float     m_colorAdaptability;
	float     m_colorPeekDistance;
	float     m_colorFullEdgeThreshold;
	float     m_colorHalfEdgeThreshold;
	bool      m_colorToleranceEnabled;
	float     m_initializationErrorThreshold;
	float     m_failureErrorThreshold;
	bool      m_multiHypothesesEnabled;
	float     m_motionPredictionFactor;
	FVector2f m_interpolationRate;
	int       m_estimatorType;
	float     m_estimatorLimit;
	float     m_rejectionFactorA;
	float     m_rejectionFactorB;

	// parameter uniform locations
	int m_uSearchRange;
	int m_uSamplingDistance;
	int m_uSampleAdaptiveDensity;
	int m_uSurfaceAngleLimit;
	int m_uContourAngleLimit;
	int m_uEdgeLumaWeight;
	int m_uEdgeChromaWeight;
	int m_uEdgeThreshold;
	int m_uColorPeekDistance;
	int m_uColorFullEdgeThreshold;
	int m_uColorHalfEdgeThreshold;
	int m_uColorAdaptability;
	int m_uFilterWidth;
	int m_uMotionCompensation;

	// View
	FGLProgram m_prgModelSolid;
	FGLProgram m_prgModelEdges;
	
	FGLProgram m_prgModelSearchLines;
	int m_uSearchRange2;
	int m_uSamplingDistance2;
	int m_uImageSize2;
	int m_suDepthModel2;
	int m_uSampleAdaptiveDensity2;
	int m_uMotionCompensation2;
	
	FGLTextureRect m_texOverlayInitialPose;
	FGLFramebuffer m_fbOverlayInitialPose;

	FGLTextureRect m_texOverlayFittedPose;
	FGLFramebuffer m_fbOverlayFittedPose;

	FGLTextureRect m_texOverlayReferenceColors;
	FGLFramebuffer m_fbOverlayReferenceColors;

	FGLCanvas m_canvasFittedPose;

	// Adaptive color matching V2
	int m_toggleId;

	FGLOverlayRect m_colorMemoryRect;
	FGLTextureRect m_texColorBuffer;
	FGLFramebuffer m_fbColorBuffer;

	static const size_t NUM_COLOR_BUFFERS = 4;
	FGLTextureRect m_texColorMemory[NUM_COLOR_BUFFERS * 2];
	FGLFramebuffer m_fbColorMemory[2];

	FGLProgram m_prgSampleColors;

	int m_uUC_ImageSize;
	int m_uUC_TransferSize;
	int m_uUC_MinSampleDistance;
	int m_uUC_SampleAdaptiveDensity;
	int m_uUC_SearchRange;
	int m_uUC_ColorPeekDistance;
	int m_uUC_SurfaceAngleLimit;
	int m_uUC_ContourAngleLimit;
	int m_uUC_FilterWidth;
	int m_suUC_DepthModel;
	int m_suUC_ResidualData;
	int m_suUC_Image;
	int m_suUC_FilterKernel;

	FGLProgram m_prgUpdateColors;

	int m_uUC_UpdateColorParams;
	int m_suUC_SampledColors;
	int m_suUC_ColorMemory0;
	int m_suUC_ColorMemory1;
	int m_suUC_ColorMemory2;
	int m_suUC_ColorMemory3;

	FPixelRGBA32f* m_pColorMemoryData;

	// Statistics
	FTrackerStatistics* m_pStatistics;
	FStopWatch m_stopWatch;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FLINETRACKER_H