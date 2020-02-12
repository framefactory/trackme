// ----------------------------------------------------------------------------------------------------
//  Title			FContourClass.h
//  Description		Header file for FContourClass.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-25 21:02:54 +0200 (Do, 25 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FCONTOURCLASS_H
#define FCONTOURCLASS_H

#include "FTrackMe.h"
#include "FVectorT.h"
#include "FMatrix3T.h"
#include "FArchive.h"
#include "FContourTemplate.h"
#include "FContour.h"
#include "FCameraPose.h"

// ----------------------------------------------------------------------------------------------------
//  Class FContourClass
// ----------------------------------------------------------------------------------------------------

class FContourClass
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FContourClass();
	/// Creates a new contour class from the given contour.
	FContourClass(const FContour* pContour,
		quint32 numFerns, quint32 numBits, const QSize& templateSize);
	/// Destructor.
	~FContourClass();

private:
	FContourClass(const FContourClass& other);
	FContourClass& operator=(const FContourClass& other);

	//  Public commands --------------------------------------------------------

public:
	/// Adds the given descriptors to the class, updating it's posterior probability.
	/// MSE is the mean square error from contour fitting, DP the Frobenius norm of the
	/// delta pose (current contour against base contour template of class).
	void increment(quint32* pDescriptors, double mse, double dp);

	/// Sets the transformation of this class' contour template.
	/// The given matrix is the product of the normalization, camera intrinsics and 3D pose.
	void setTransform(const FMatrix3f& matNKP);

	/// Serialization
	void serialize(FArchive& ar);

	//  Public queries ---------------------------------------------------------

	/// Returns the probability of this class given the fern descriptors.
	double probability(quint32* pDescriptors) const;

	/// Matches the given normalized contour pixels to the distance map of
	/// this class by optimizing a homography warp. The function returns the
	/// parameters of the homography and the mean squared error of the match.
	/// pInfo[0]: MSE, pInfo[1]: PC covariance, pInfo[2]: determinant homography
	void matchContour(const FContour* pNormalizedContour,
		OUT FMatrix3f& homography, OUT float* pInfo = NULL) const;

	/// Returns the id of the contour template.
	quint32 templateIndex() const { return m_templateIndex; }
	/// Returns the distance map and gradient map template.
	const FContourTemplate& contourTemplate() const { return m_template; }

	/// Returns the transformation of this class' contour template.
	const FMatrix3f& matrixNKP() const { return m_matNKP; }
	
	/// Returns the total number of training samples for this class.
	size_t sampleCount() const { return m_classFrequency; }
	
	/// Returns a measure of fitting accuracy (lower value > more accurate)
	double cumulatedFittingAccuracy() const { return m_mseSum; }
	double fittingAccuracy() const { return m_mseSum / (double)m_classFrequency; }
	
	/// Returns a measure of pose ambiguity (lower value > more reliable)
	double cumulatedPoseAmbiguity() const { return m_dpSum; }
	double poseAmbiguity() const { return m_dpSum / (double)m_classFrequency; }

	/// Writes information about the internal state to the given debug object.
	void dump(QDebug& debug) const;

	//  Internal data members --------------------------------------------------

private:
	FContourTemplate m_template;// distance and gradient map
	quint32* m_pFrequency;		// classifier data
	quint32 m_classFrequency;
	quint32 m_numFerns;
	quint32 m_numBits;

	quint32 m_templateIndex;	// contour identification
	FMatrix3f m_matNKP;			// precomputed N*K*P transformation matrix

	double m_mseSum;			// sum of fitting mse (measure of fitting accuracy)
	double m_dpSum;				// sum of pose differences (measure of pose variablility)

	// temporary
	quint32 m_numTestsPerFern;
};

// Inline functions -----------------------------------------------------------------------------------

inline void FContourClass::increment(quint32* pDescriptors, double mse, double dp)
{
	// covariance from contour alignment
	m_mseSum += mse;
	m_dpSum += dp;

	// learn patch descriptor
	for (quint32 f = 0; f < m_numFerns; f++)
	{
		quint32 d = pDescriptors[f];
		F_ASSERT(d < (1 << m_numBits));
		m_pFrequency[f * m_numBits + pDescriptors[f]]++;
	}

	m_classFrequency++;
}

inline void FContourClass::matchContour(const FContour* pNormalizedContour,
										OUT FMatrix3f& homography, OUT float* pInfo) const
{
	m_template.matchContour(pNormalizedContour, homography, pInfo);
}

inline double FContourClass::probability(quint32* pDescriptors) const
{
	double p = 1.0;
	double r = 1.0 / ((double)(m_classFrequency + m_numTestsPerFern));
	for (quint32 f = 0; f < m_numFerns; f++)
	{
		quint32 d = pDescriptors[f];
		p *= ((double)m_pFrequency[f * m_numBits + pDescriptors[f]] + 1.0) * r;
	}

	return p;
}
	
// ----------------------------------------------------------------------------------------------------

#endif // FCONTOURCLASS_H