// ----------------------------------------------------------------------------------------------------
//  Title			FLineTrackerBase.h
//  Description		Header file for FLineTrackerBase.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FLINETRACKERBASE_H
#define FLINETRACKERBASE_H

#include "FlowCoreDefs.h"

#include "FlowMath.h"
#include "FlowGL.h"
#include "FPixelStruct.h"

#include "FLineModel.h"
#include "FCamera.h"
#include "FStopWatch.h"
#include "FPerformanceStats.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLineTrackerBase
// ----------------------------------------------------------------------------------------------------

class FLineTrackerBase
{
	//  Static callbacks -------------------------------------------------------

private:
	inline static void sLevmarUpdate(double* p, double* hx, int m, int n, void* pData) {
		//F_ASSERT(pData);
		//((FLineTrackerBase*)pData)->_levmarUpdate(p, hx, m, n);
		s_pActiveTracker->_levmarUpdate(p, hx, m, n);
	}
	inline static void sLevmarJacobian(double* p, double* j, int m, int n, void* pData) {
		//F_ASSERT(pData);
		//((FLineTrackerBase*)pData)->_levmarJacobian(p, j, m, n);
		s_pActiveTracker->_levmarJacobian(p, j, m, n);
	}

	static FLineTrackerBase* s_pActiveTracker;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FLineTrackerBase();
	/// Virtual destructor.
	virtual ~FLineTrackerBase();

	//  Public commands --------------------------------------------------------

public:
	/// Searches the input image for edges.
	void track(const FGLTextureRect& inputFrame);
	/// Resets the tracker and prepares for the given frame size.
	void reset(const QSize& frameSize);

	/// Sets the camera to be used for model projection.
	void setCamera(FCamera* pCamera);
	/// Sets the edge model to be used for tracking.
	void setModel(FLineModel* pModel);

	/// Sets the rotation matrix for model positioning.
	void setModelTransform(const FMatrix4f& modelTransform);
	/// Enables/disables tracking.
	void setTrackingEnabled(bool state) { m_trackingEnabled = state; }

	/// Draws the solid model.
	void drawSolidModel();

	//  Public queries ---------------------------------------------------------

	/// Returns the depth texture from solid model rendering.
	const FGLTextureRect& depthPass() const { return m_fbTexModelDepth; }
	/// Creates and returns an overlay texture with the predicted line model,
	/// search lines and edge candidate points.
	const FGLTextureRect& initialModel();
	/// Creates and returns an overlay texture with the fitted line model.
	const FGLTextureRect& fittedModel();

	/// Returns the edge model used for tracking.
	FLineModel* model() const { return m_pModel; }

	//  Parameter --------------------------------------------------------------

	void setSearchRange(double val) { m_searchRange = val; }
	void setSamplingDistance(double val) { m_samplingDistance = val; }
	void setEdgeLumaWeight(double val) { m_edgeLumaWeight = val; }
	void setEdgeChromaWeight(double val) { m_edgeChromaWeight = val; }
	void setEdgeThreshold(double val) { m_edgeThreshold = val; }
	void setColorToleranceEnabled(bool state) { m_colorToleranceEnabled = state; }
	void setColorPeekDistance(double val) { m_colorPeekDistance = val; }
	void setColorFullEdgeThreshold(double val) { m_colorFullEdgeThreshold = val; }
	void setColorHalfEdgeThreshold(double val) { m_colorHalfEdgeThreshold = val; }

	//  Overridables -----------------------------------------------------------

protected:
	virtual void onReset() = 0;
	virtual void onProjectModel() = 0;
	virtual const FGLTextureRect& onProcessedFrame(size_t index) const = 0;

	//  Internal functions -----------------------------------------------------

private:
	void _initParameters();
	void _resetGL();
	void _drawDepthModel();
	void _optimizePose();
	void _drawInitialPose();
	void _drawFittedPose();
	void _reprojectModel();
	void _levmarUpdate(double* p, double* hx, int m, int n);
	void _levmarJacobian(double* p, double* j, int m, int n);
	void _renderFilter();
	void _calculateBlurFilter(float sigmaParallel, float sigmaOrthogonal);

	//  Internal data members --------------------------------------------------

protected:
	QSize m_frameSize;
	FCamera* m_pCamera;
	FLineModel* m_pModel;

	FGLTextureRect m_sourceFrame;
	FGLOverlayRect m_overlayRect;

	FGLProgram m_prgModelDepth;
	FGLTextureRect m_fbTexModelColor;
	FGLTextureRect m_fbTexModelDepth;
	FGLFramebuffer m_fbModelDepthColor;

	FGLProgram m_prgModelSolid;
	FGLProgram m_prgModelLine;

	FGLTextureRect m_texOverlayInitialPose;
	FGLFramebuffer m_fbOverlayInitialPose;

	FGLTextureRect m_texOverlayFittedPose;
	FGLFramebuffer m_fbOverlayFittedPose;
	FGLCanvas m_canvasFittedPose;

	FGLBuffer m_bufModelTransform;
	FGLBuffer m_bufModelTransform2;

	size_t m_blurFilterSteps;
	size_t m_blurFiterWidth;
	float m_sigmaParallel;
	float m_sigmaOrthogonal;
	float* m_pBlurFilter;

	// Parameter
	float m_searchRange;
	float m_samplingDistance;
	float m_edgeLumaWeight;
	float m_edgeChromaWeight;
	float m_edgeThreshold;
	float m_colorPeekDistance;
	float m_colorFullEdgeThreshold;
	float m_colorHalfEdgeThreshold;
	bool m_colorToleranceEnabled;

	bool m_trackingEnabled;

	// Statistics
	FPerformanceStats m_statistics;
	FStopWatch m_stopWatch;
	size_t m_sampleCount;
	size_t m_candidateCount;
	size_t m_pixelCount;
};

// ----------------------------------------------------------------------------------------------------


#endif // FLINETRACKERBASE_H