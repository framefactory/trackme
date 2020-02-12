// ----------------------------------------------------------------------------------------------------
//  Title			FFernTracker.h
//  Description		Header file for FFernTracker.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 6 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FFERNTRACKER_H
#define FFERNTRACKER_H

#include "FTrackMe.h"
#include "FlowMath.h"
#include "FlowGL.h"

// ----------------------------------------------------------------------------------------------------
//  Class FFernTracker
// ----------------------------------------------------------------------------------------------------

class FFernTracker
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FFernTracker();
	/// Virtual destructor.
	virtual ~FFernTracker();

	//  Public commands --------------------------------------------------------

public:
	void reset(const QSize& frameSize);
	void track(const FGLTextureRect& sourceFrame);

	//  Public queries ---------------------------------------------------------

	const FGLTextureRect& harris() const;

	//  Internal functions -----------------------------------------------------

private:
	void _initGL();
	void _renderFilter();

	//  Internal data members --------------------------------------------------

private:
	QSize m_frameSize;
	FGLTextureRect m_sourceFrame;
	FGLOverlayRect m_overlayRect;

	FGLRenderTargetRect m_frameBuffer1;
	FGLRenderTargetRect m_frameBuffer2;

	FGLProgram m_prgGaussian3x3;
	FGLProgram m_prgGaussianVert;
	FGLProgram m_prgGaussianHorz;
	FGLProgram m_prgHarrisDerivatives;
	FGLProgram m_prgHarrisCorners;
	FGLProgram m_prgHarrisSuppression;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FFERNTRACKER_H