// ----------------------------------------------------------------------------------------------------
//  Title			FLineTrackerGPU2.h
//  Description		Header file for FLineTrackerGPU2.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 10 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FLINETRACKERGPU2_H
#define FLINETRACKERGPU2_H

#include "FlowCoreDefs.h"
#include "FlowMath.h"
#include "FlowGL.h"
#include "FLineTrackerBase.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLineTrackerGPU2
// ----------------------------------------------------------------------------------------------------

class FLineTrackerGPU2 : public FLineTrackerBase
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FLineTrackerGPU2();
	/// Virtual destructor.
	virtual ~FLineTrackerGPU2();

	//  Overrides --------------------------------------------------------------

protected:
	virtual void onReset();
	virtual void onProjectModel();
	virtual const FGLTextureRect& onProcessedFrame(size_t index) const;

	//  Internal functions -----------------------------------------------------

private:
	void _resetGL();
	void _projectModel();

	//  Internal data members --------------------------------------------------

private:
	QSize m_transferSize;
	FGLOverlayRect m_transferRect;

	FGLProgram m_prgModelSample;
	FGLProgram m_prgModelEdgeSuppress;
	int m_uImageSize;
	int m_uTransferSize;
	int m_suHiddenLineModel;
	int m_suImage;
	int m_suFilterKernel;

	FGLTextureRect m_fbTexModelSearchIntermediate;
	FGLFramebuffer m_fbModelSearchIntermediate;
	FGLTextureRect m_fbTexModelSearchResult;
	FGLFramebuffer m_fbModelSearchResult;

	FPixelRGBA32f* m_pSearchResult;

	FGLBuffer m_bufBlurFilter;
	FGLTextureBuffer m_texBlurFilter;

	// Parameter
	int m_uSearchRange;
	int m_uSamplingDistance;
	int m_uEdgeLumaWeight;
	int m_uEdgeChromaWeight;
	int m_uEdgeThreshold;
	int m_uColorPeekDistance;
	int m_uColorFullEdgeThreshold;
	int m_uColorHalfEdgeThreshold;
	int m_uFilterWidth;
};

// ----------------------------------------------------------------------------------------------------

#endif // FLINETRACKERGPU2_H