// ----------------------------------------------------------------------------------------------------
//  Title			FTrainingParameter.cpp
//  Description		Implementation of class FTrainingParameter
// ----------------------------------------------------------------------------------------------------
//  $Author: Ralph Wiedemeier $
//  $Revision: 1 $
//  $Date: 2011/08/14 $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FTrainingParameter.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingParameter
// ----------------------------------------------------------------------------------------------------

// Serialization operators ----------------------------------------------------------------------------

FArchive& operator<<(FArchive& ar, const FTrainingParameter& obj)
{
	ar << obj.posMin;
	ar << obj.posMax;
	ar << obj.yprMin;
	ar << obj.yprMax;
	ar << obj.focalLength;
	ar << obj.inverseRotationOrder;
	return ar;
}

FArchive& operator>>(FArchive& ar, FTrainingParameter& obj)
{
	ar >> obj.posMin;
	ar >> obj.posMax;
	ar >> obj.yprMin;
	ar >> obj.yprMax;
	ar >> obj.focalLength;
	ar >> obj.inverseRotationOrder;
	return ar;
}

void FTrainingParameter::dump(QDebug& debug) const
{
	debug << "\n\n--- FTrainingParameter ---";
	debug << "\n     Position min:   " << posMin.toString();
	debug << "\n     Position max:   " << posMax.toString();
	debug << "\n     YPR min:        " << yprMin.toString();
	debug << "\n     YPR max:        " << yprMax.toString();
	debug << "\n     Focal length:   " << focalLength;
	debug << "\n     Rotation order: " << inverseRotationOrder;
}

// ----------------------------------------------------------------------------------------------------