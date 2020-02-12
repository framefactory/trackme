// ----------------------------------------------------------------------------------------------------
//  Title			FPerformanceStats.h
//  Description		Header file for FPerformanceStats.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FPERFORMANCESTATS_H
#define FPERFORMANCESTATS_H

#include "FTrackMe.h"

// ----------------------------------------------------------------------------------------------------
//  Class FPerformanceStats
// ----------------------------------------------------------------------------------------------------

class FPerformanceStats
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FPerformanceStats();
	/// Virtual destructor.
	virtual ~FPerformanceStats();

	//  Public commands --------------------------------------------------------

public:
	/// Clears the statistics and prepares for measurements.
	void begin();

	/// Adds a time measurement for a specific number of samples and candidates.
	void addMeasurement(double searchTime, double optimizationTime,
		size_t numSamples, size_t numPixels, size_t numCandidates);

	/// Finalizes the statistics.
	void end();

	/// Writes the measurement results to a file.
	void write(const char* fileName);

	//  Public queries ---------------------------------------------------------



	//  Internal data members --------------------------------------------------

private:
	static const size_t MAX_LINES = 128;
	static const size_t MAX_SAMPLES = 32;
	static const size_t MAX_CANDIDATES = 8;
	static const size_t MAX_SEARCH_PIXELS = 50;

	static const size_t SAMPLE_SLOTS = MAX_LINES * MAX_CANDIDATES;
	static const size_t PIXEL_SLOTS = SAMPLE_SLOTS * MAX_SEARCH_PIXELS;
	static const size_t CANDIDATE_SLOTS = SAMPLE_SLOTS * MAX_CANDIDATES;

	double m_searchTimePerSample[SAMPLE_SLOTS];
	double m_searchTimePerPixel[PIXEL_SLOTS];
	double m_optimizationTimePerCandidate[CANDIDATE_SLOTS];

	size_t m_countPerSample[SAMPLE_SLOTS];
	size_t m_countPerPixel[PIXEL_SLOTS];
	size_t m_countPerCandidate[CANDIDATE_SLOTS];

	bool m_isMeasuring;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FPERFORMANCESTATS_H