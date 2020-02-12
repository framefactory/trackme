// ----------------------------------------------------------------------------------------------------
//  Title			FContourFinder.h
//  Description		Header file for FContourFinder.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-09 13:00:10 +0200 (Fr, 09 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FCONTOURFINDER_H
#define FCONTOURFINDER_H

#include "FTrackMe.h"
#include "FlowMath.h"
#include "FlowGL.h"
#include "FPixelStruct.h"
#include "FDTPixel.h"
#include "FStopWatch.h"
#include "FFrameStatistics.h"
#include "FContourModel.h"
#include "FContour.h"

// ----------------------------------------------------------------------------------------------------
//  Class FContourFinder
// ----------------------------------------------------------------------------------------------------

class FContourFinder
{
	//  Public enumerations ----------------------------------------------------

public:
	enum extractionMode_t
	{
		Direct,
		LevelCurve
	};

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FContourFinder();
	/// Virtual destructor.
	virtual ~FContourFinder();

private:
	FContourFinder(const FContourFinder& other);
	FContourFinder& operator=(const FContourFinder& other);

	//  Public commands --------------------------------------------------------

public:
	/// Extracts closed contours in the given distance transform map.
	void findContours(const FGLTextureRect& distanceTransform, FDetectorStatistics* pStats = NULL);
	void findContours(FDTPixel* pDTImage, FDetectorStatistics* pStats = NULL);

	/// Extracts contours in the given distance transform map during training,
	/// using the information of the given contour model.
	void findContoursTraining(const FGLTextureRect& distanceTransform, const FContourModel& contourModel);

	/// Resets the contour finder and allocates space for data structures.
	bool reset(const QSize& frameSize);

	/// Choses a method for for contour extraction: 'Direct' directly
	/// follows the edge pixels, 'LevelCurve' follows the contour in a distance,
	/// using the distance transform.
	void setContourExtractionMode(extractionMode_t mode);

	void drawContourStatistics(FGLCanvas& canvas);

	//  Public queries ---------------------------------------------------------

	/// Returns the current frame size the algorithm operates on.
	const QSize& frameSize() const;
	/// Returns the texture containing the detected contours.
	const FGLTextureRect& contourView();

	/// Returns the number of detected and valid contours.
	size_t contourCount() const { return m_contCandCount; }
	
	/// Returns the contour at the given index.
	const FContour* contourAt(size_t index) {
		F_ASSERT(index < m_contCandCount);
		return m_contCandidates[index];
	}
	/// Returns the distance transform image in host memory.
	const FDTPixel* dtImage() const { return m_pFrameData; }

	//  Internal functions -----------------------------------------------------

private:
	void _clearContourData();
	void _prepareDTImage();
	
	void _findContoursLevelCurve();
	void _findContoursDirect();
	void _findContoursPostProcess();

	bool _followContourLevelCurve(FDTPixel* pData, int ci, int nx, int ny, float contourId);
	bool _followContourDirect(FDTPixel* pData, int ci, int nx, int ny, float contourId);

	void _fillArea(int x, int y, float minDist, float searchIndex, float replaceIndex);

	//  Simple stack implementation for 2d coordinates
	inline bool push(const FVector2i& p) {
		if (m_stackPointer + 1 < STACK_SIZE) {
			m_pointStack[++m_stackPointer] = p;
			return true;
		}
		else
			return false;
	}

	inline bool pop(FVector2i& p) {
		if (m_stackPointer >= 0) {
			p = m_pointStack[m_stackPointer--];
			return true;
		}
		else
			return false;
	}

	inline void emptyStack() {
		m_stackPointer = -1;
	}

	//  Internal data members --------------------------------------------------

private:
	QSize m_frameSize;
	FDTPixel* m_pFrameData;
	FDTPixel* m_pFrameDataInt;
	FGLTextureRect m_texContourData;

	FContour m_contFragments[FGlobalConstants::MAX_CONTOUR_FRAGMENTS];
	FContour* m_contCandidates[FGlobalConstants::MAX_CONTOUR_CANDIDATES];
	size_t m_contFragCount;
	size_t m_contCandCount;

	bool m_useDirectExtraction;

	// Statistics
	FStopWatch m_stopWatch;
	FDetectorStatistics* m_pStatistics;

	// Stack for flood fill
	static const size_t STACK_SIZE = 4096;
	FVector2i m_pointStack[STACK_SIZE];
	int m_stackPointer;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FCONTOURFINDER_H