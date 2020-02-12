// ----------------------------------------------------------------------------------------------------
//  Title			FDistanceTransform.h
//  Description		Header file for FDistanceTransform.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-14 22:42:33 +0200 (So, 14 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FDISTANCETRANSFORM_H
#define FDISTANCETRANSFORM_H

#include "FTrackMe.h"
#include "FlowGL.h"
#include "FDTPixel.h"

// ----------------------------------------------------------------------------------------------------
//  Class FDistanceTransform
// ----------------------------------------------------------------------------------------------------

class FDistanceTransform
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FDistanceTransform();
	/// Virtual destructor.
	virtual ~FDistanceTransform();

	//  Public commands --------------------------------------------------------

public:
	/// Calculates a distance transform on the given binary image.
	/// The image's first (red) channel contains the seed information.
	/// A value of >0 is considered a seed.
	void calculateDt(const FGLTextureRect& inputImage);
	void calculateCannyDt(const FGLTextureRect& inputImage);

	/// Allocates the needed OpenGL resources for the given image size.
	bool reset(const QSize& imageSize);

	// canny

	void setEdgeThresholdLow(float val) { m_edgeThresholdLow = val; }
	void setEdgeThresholdHigh(float val) { m_edgeThresholdHigh = val; }

	//  Public queries ---------------------------------------------------------

	/// Returns true if all resources have been initialized properly.
	bool isValid() const { return m_isValid; }
	/// Returns a texture holding the resulting distance transform.
	const FGLTextureRect& result() const;
	/// Copies the resulting distance transform map to the given array.
	void getResult(FDTPixel* pDTImage);

	//  Internal functions -----------------------------------------------------

private:
	void _runCanny(const FGLTextureRect& inputImage);
	void _runDistanceTransform(const FGLTextureRect& inputImage);
	bool _initGL();
	bool _resetGL();

	//  Internal data members --------------------------------------------------

	bool m_isValid;

	QSize m_imageSize;
	FGLFramebuffer m_fbBuffer[2];
	FGLTextureRect m_texBuffer[2];
	FGLOverlayRect m_overlayRect;

	// parameter
	float m_edgeThresholdLow;
	quint32 m_uEdgeThresholdLow;
	float m_edgeThresholdHigh;
	quint32 m_uEdgeThresholdHigh;
	
	// canny
	FGLProgram m_prgGaussianVert;
	FGLProgram m_prgGaussianHorz;
	FGLProgram m_prgCannyDetect;
	FGLProgram m_prgCannySuppress;
	FGLProgram m_prgCannyReduce1;
	FGLProgram m_prgCannyReduce2;
	FGLProgram m_prgCannyThreshold;

	// distance transform
	FGLProgram m_prgJumpFloodInit;
	FGLProgram m_prgJumpFloodStep;
	FGLSampler m_samplerBorder;

	quint32 m_uImage;
	quint32 m_uStepSize;
	int m_stepCount;
	int m_maxStepSize;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FDISTANCETRANSFORM_H