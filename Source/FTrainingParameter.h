// ----------------------------------------------------------------------------------------------------
//  Title			FTrainingParameter.h
//  Description		Header file for FTrainingParameter.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-17 00:16:08 +0200 (Mi, 17 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FTRAININGPARAMETER_H
#define FTRAININGPARAMETER_H

#include "FTrackMe.h"
#include "FMath.h"
#include "FVectorT.h"
#include "FArchive.h"
#include "FCameraPose.h"

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingParameter
// ----------------------------------------------------------------------------------------------------

/// Parameters for pose training. All rotation angles are given in degrees.
struct FTrainingParameter
{
	FTrainingParameter() {
		posMin.set(-0.0f, -0.0f, -150.0f);
		posMax.set(0.0f, 0.0f, -150.0f);
		yprMin.set(-60.0f, -60.0f, -180.0f);
		yprMax.set( 60.0f,  60.0f,  180.0f);
		focalLength = 35.0f;
		inverseRotationOrder = false;
	}

	void getMinPose(FCameraPose& pose) {
		pose.setTranslation(posMin);
		pose.setRotationEuler(yprMin * (float)FMath::d2r, inverseRotationOrder);
		pose.setFocalLength(focalLength);
	}

	void getMaxPose(FCameraPose& pose) {
		pose.setTranslation(posMax);
		pose.setRotationEuler(yprMax * (float)FMath::d2r, inverseRotationOrder);
		pose.setFocalLength(focalLength);
	}

	void getMeanPose(FCameraPose& pose) {
		pose.setTranslation((posMin + posMax) * 0.5f);
		pose.setRotationEuler((yprMin + yprMax) * 0.5f * (float)FMath::d2r, inverseRotationOrder);
		pose.setFocalLength(focalLength);
	}

	void dump(QDebug& debug) const;

	friend FArchive& operator<<(FArchive& ar, const FTrainingParameter& obj);
	friend FArchive& operator>>(FArchive& ar, FTrainingParameter& obj);

	FVector3f posMin;
	FVector3f posMax;
	FVector3f yprMin;
	FVector3f yprMax;

	float focalLength;
	bool inverseRotationOrder;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FTRAININGPARAMETER_H