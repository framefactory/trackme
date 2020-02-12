// ----------------------------------------------------------------------------------------------------
//  Title			FLineTrackerCPU.h
//  Description		Header file for FLineTrackerCPU.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 10 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FLINETRACKERCPU_H
#define FLINETRACKERCPU_H

#include <cmath>
#include "FlowCoreDefs.h"
#include "FPixelStruct.h"
#include "FStopWatch.h"
#include "FLineTrackerBase.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLineTrackerCPU
// ----------------------------------------------------------------------------------------------------

class FLineTrackerCPU : public FLineTrackerBase
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FLineTrackerCPU();
	/// Virtual destructor.
	virtual ~FLineTrackerCPU();

	//  Overrides --------------------------------------------------------------

protected:
	virtual void onReset();
	virtual void onProjectModel();
	virtual const FGLTextureRect& onProcessedFrame(size_t index) const;

	//  Internal functions -----------------------------------------------------

private:
	void _reset();
	void _projectModel();

	void _searchLine(int edgeId, int sampleId, const FVector2f& direction, const FVector2i& p0, const FVector2i& p1);
	void _testPoint(int edgeId, int sampleId, const FVector2f& direction, int x, int y);
	FVector3f _filterPixel(int angle, int x, int y);

	inline bool _pixVisible(float x, float y) {
		int ix = floorf(x + 0.5f);
		int iy = floorf(y + 0.5f);
		return (ix >= 0 && ix < m_frameWidth && iy >= 0 && iy < m_frameHeight);
	}

	inline int _pixIndex(float x, float y) {
		int ix = floorf(x + 0.5f);
		int iy = floorf(y + 0.5f);
		if (ix < 0 || ix >= m_frameWidth || iy < 0 || iy >= m_frameHeight)
			return -1;
		return iy * m_frameWidth + ix;
	}

	//  Internal data members --------------------------------------------------

private:
	int m_frameWidth;
	int m_frameHeight;

	FGLTextureRect m_fbTexAnnotationCanvas2;
	FGLFramebuffer m_fbAnnotationCanvas2;
	FGLCanvas m_annotationCanvas2;

	float* m_pDepthBuffer;
	FPixelRGBA32f* m_pImageBuffer;
};

// ----------------------------------------------------------------------------------------------------

#endif // FLINETRACKERCPU_H