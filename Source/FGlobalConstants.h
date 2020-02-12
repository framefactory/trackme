// ----------------------------------------------------------------------------------------------------
//  Title			FGlobalConstants.h
//  Description		Header file for FGlobalConstants.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: Ralph Wiedemeier $
//  $Revision: 1 $
//  $Date: 2011/08/11 $
// ----------------------------------------------------------------------------------------------------

#ifndef FGLOBALCONSTANTS_H
#define FGLOBALCONSTANTS_H

#include "FTrackMe.h"

// ----------------------------------------------------------------------------------------------------
//  Class FGlobalConstants
// ----------------------------------------------------------------------------------------------------

/// Class provides static constant values.
class FGlobalConstants
{
	//  Public members ---------------------------------------------------------

public:
	/// Major version
	static const size_t VERSION_MAJOR = 0;
	/// Minor version
	static const size_t VERSION_MINOR = 92;

	/// If set to 1, use second version of adaptive color matching
	static const size_t USE_ACM_V2 = 0;

	/// Maximum number of samples per edge
	static const size_t MAX_SAMPLES_PER_EDGE = 24;
	/// Maximum number of edge candidates per sample
	static const size_t MAX_CANDIDATES_PER_SAMPLE = 16;
	/// Length of sample search line
	static const size_t SEARCH_LINE_LENGTH = 64;

	/// Pose detection: The maximum number of contour fragments that can be processed in an image.
	static const size_t MAX_CONTOUR_FRAGMENTS = 1024;
	/// Pose detection: The maximum number of valid contour candidates in an image.
	static const size_t MAX_CONTOUR_CANDIDATES = 128;
	/// Pose detection: The maximum number of different contours that can be identified.
	static const size_t MAX_TEMPLATES = 16;
	/// Pose detection: The number of ferns used by the classifier.
	static const size_t NUM_FERNS = 32;
	/// Pose detection: The number of bits per fern used by the classifier.
	static const size_t NUM_BITS = 11;

	// TODO: Dirty hack!
	static float* pLevmarDetectionWorkspace;
	static double* pLevmarTrackingWorkspace;


	//  Internal functions -----------------------------------------------------

private:
	FGlobalConstants();
	~FGlobalConstants();
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FGLOBALCONSTANTS_H