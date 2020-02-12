// ----------------------------------------------------------------------------------------------------
//  Title			FTrainingStatistics.h
//  Description		Header file for FTrainingStatistics.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-17 00:16:08 +0200 (Mi, 17 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FTRAININGSTATISTICS_H
#define FTRAININGSTATISTICS_H

#include "FTrackMe.h"
#include "FCameraPose.h"

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingStatistics
// ----------------------------------------------------------------------------------------------------

class FTrainingStatistics
{
public:
	FTrainingStatistics() { }

	void clear() {
		for (size_t i = 0; i < MAX_CONTOURS; i++)
			contour[i].clear();
		runCount = totalClassCount = totalSampleCount = 0;
	}

	static const size_t MAX_CONTOURS = 64;

	struct contourInfo_t
	{
		contourInfo_t() : isPresent(false), isMatch(false),
			meanSquaredError(0.0f), scaledMSE(0.0f), normalizationScale(1.0f, 1.0f),
			normalizationAngle(0.0f), classCount(0), sampleCount(0) { }

		void clear() { isPresent = isMatch = false;
			meanSquaredError = scaledMSE = normalizationAngle = 0.0f;
			normalizationScale.makeZero();
			classCount = sampleCount = 0; }

		bool isPresent;
		bool isMatch;
		float meanSquaredError;
		float scaledMSE;
		float fittingAccuracy;
		float poseAmbiguity;
		FVector2f normalizationScale;
		float normalizationAngle;
		QString name;
		quint32 classCount;
		quint32 sampleCount;
	};

	contourInfo_t contour[MAX_CONTOURS];
	FCameraPose currentPose;
	quint32 runCount;
	quint32 totalClassCount;
	quint32 totalSampleCount;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FTRAININGSTATISTICS_H