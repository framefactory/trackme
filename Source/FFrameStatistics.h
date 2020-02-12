// ----------------------------------------------------------------------------------------------------
//  Title			FFrameStatistics.h
//  Description		Header file for FFrameStatistics.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FFRAMESTATISTICS_H
#define FFRAMESTATISTICS_H

#include "FTrackMe.h"
#include "FLineTrackerState.h"

// ----------------------------------------------------------------------------------------------------
//  Struct FTrackerStatistics
// ----------------------------------------------------------------------------------------------------

struct FTrackerStatistics
{
	FTrackerStatistics() {
		memset(this, 0, sizeof(FTrackerStatistics));
	}

	enum stage_t
	{
		Start					= 0,
		Prediction				= 1,
		OptimizationA			= 2,
		OptimizationB			= 3,
		NumStages				= 4
	};

	double errorMedian[NumStages];
	double errorMean[NumStages];
	double errorSD[NumStages];

	double timeSearch;
	double timeOptimizationA;
	double timeOptimizationB;

	double variance[7];

	FLineTrackerState state;
};

// ----------------------------------------------------------------------------------------------------
//  Struct FDetectorStatistics 
// ----------------------------------------------------------------------------------------------------

struct FDetectorStatistics
{
	FDetectorStatistics() {
		memset(this, 0, sizeof(FDetectorStatistics));
	}

	double timeContourExtraction;
	double timeContourNormalization;
	double timeContourAlignment;
	double timePoseReconstruction;

	int numPoses;
	int poseUsed;
};

// ----------------------------------------------------------------------------------------------------
//  Struct FFrameStatistics
// ----------------------------------------------------------------------------------------------------

struct FFrameStatistics
{
	FFrameStatistics() {
		for (int i = 0; i < 7; i++)
			finalPose[i] = finalPoseSmooth[i] = 0.0;
		frameNumber = 0;
	}

	FTrackerStatistics tracker;
	FDetectorStatistics detector;

	double finalPose[7];
	double finalPoseSmooth[7];

	size_t frameNumber;

private:
	static const int typeId;
	static const int typeId2;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FFRAMESTATISTICS_H