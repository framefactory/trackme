// ----------------------------------------------------------------------------------------------------
//  Title			FPerformanceStats.cpp
//  Description		Implementation of class FPerformanceStats
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include <stdio.h>

#include "FPerformanceStats.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FPerformanceStats
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FPerformanceStats::FPerformanceStats()
: m_isMeasuring(false)
{
}

FPerformanceStats::~FPerformanceStats()
{
}

// Public commands ------------------------------------------------------------------------------------

void FPerformanceStats::begin()
{
	for (size_t i = 0; i < SAMPLE_SLOTS; i++)
	{
		m_countPerSample[i] = 0;
		m_searchTimePerSample[i] = 0.0;
	}

	for (size_t i = 0; i < PIXEL_SLOTS; i++)
	{
		m_countPerPixel[i] = 0;
		m_searchTimePerPixel[i] = 0.0;
	}

	for (size_t i = 0; i < CANDIDATE_SLOTS; i++)
	{
		m_countPerCandidate[i] = 0;
		m_optimizationTimePerCandidate[i] = 0.0;
	}

	m_isMeasuring = true;
}

void FPerformanceStats::addMeasurement(double searchTime, double optimizationTime,
										size_t numSamples, size_t numPixels, size_t numCandidates)
{
	F_ASSERT(m_isMeasuring);
	if (!m_isMeasuring)
		return;

	F_ASSERT(numSamples < SAMPLE_SLOTS);
	F_ASSERT(numPixels < PIXEL_SLOTS);
	F_ASSERT(numCandidates < CANDIDATE_SLOTS);

	m_searchTimePerSample[numSamples] += searchTime;
	m_searchTimePerPixel[numPixels] += searchTime;
	m_optimizationTimePerCandidate[numCandidates] += optimizationTime;

	m_countPerSample[numSamples]++;
	m_countPerPixel[numPixels]++;
	m_countPerCandidate[numCandidates]++;
}

void FPerformanceStats::end()
{
	m_isMeasuring = false;

	for (size_t i = 0; i < SAMPLE_SLOTS; i++)
	{
		if (m_countPerSample[i] > 0)
			m_searchTimePerSample[i] /= (double)m_countPerSample[i];
	}

	for (size_t i = 0; i < PIXEL_SLOTS; i++)
	{
		if (m_countPerPixel[i] > 0)
			m_searchTimePerPixel[i] /= (double)m_countPerPixel[i];
	}

	for (size_t i = 0; i < CANDIDATE_SLOTS; i++)
	{
		if (m_countPerCandidate[i] > 0)
			m_optimizationTimePerCandidate[i] /= (double)m_countPerCandidate[i];
	}
}

void FPerformanceStats::write(const char* fileName)
{
	FILE* pFile = NULL;
	fopen_s(&pFile, fileName, "w");
	F_ASSERT(pFile);
	
	fprintf(pFile, "\nSearch time by number of sample lines drawn\n\n");
	for (size_t i = 0; i < SAMPLE_SLOTS; i++)
	{
		if (m_countPerSample[i] > 0)
			fprintf(pFile, "%d \t%f \n", i, m_searchTimePerSample[i]);
	}

	fprintf(pFile, "\nSearch time by number of pixels drawn\n\n");
	for (size_t i = 0; i < PIXEL_SLOTS; i++)
	{
		if (m_countPerPixel[i] > 0)
			fprintf(pFile, "%d \t%f \n", i, m_searchTimePerPixel[i]);
	}

	fprintf(pFile, "\nOptimization time by number of edge candidates\n\n");
	for (size_t i = 0; i < CANDIDATE_SLOTS; i++)
	{
		if (m_countPerCandidate[i] > 0)
			fprintf(pFile, "%d \t%f \n", i, m_optimizationTimePerCandidate[i]);
	}

	fclose(pFile);
}

// Public queries -------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------