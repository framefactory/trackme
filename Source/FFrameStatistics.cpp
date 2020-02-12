// ----------------------------------------------------------------------------------------------------
//  Title			FFrameStatistics.cpp
//  Description		Implementation of class FFrameStatistics
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FFrameStatistics.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FFrameStatistics
// ----------------------------------------------------------------------------------------------------

// Static members -------------------------------------------------------------------------------------

const int FFrameStatistics::typeId = qRegisterMetaType<FFrameStatistics>("FFrameStatistics");
const int FFrameStatistics::typeId2 = qRegisterMetaType<FLineTrackerState>("FLineTrackerState");

// ----------------------------------------------------------------------------------------------------