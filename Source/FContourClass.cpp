// ----------------------------------------------------------------------------------------------------
//  Title			FContourClass.cpp
//  Description		Implementation of class FContourClass
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-30 20:28:52 +0200 (Di, 30 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "Eigen/Dense"

#include "FContourClass.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FContourClass
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FContourClass::FContourClass()
: m_templateIndex(0),
  m_pFrequency(NULL),
  m_classFrequency(0),
  m_numFerns(0),
  m_numBits(0),
  m_mseSum(0.0),
  m_dpSum(0.0)
{
}

FContourClass::FContourClass(const FContour* pContour,
							 quint32 numFerns,
							 quint32 numBits,
							 const QSize& templateSize)
: m_template(templateSize),
  m_classFrequency(0),
  m_numFerns(numFerns),
  m_numBits(numBits),
  m_mseSum(0.0),
  m_dpSum(0.0)
{
	m_numTestsPerFern = (1 << m_numBits);
	quint32 numTests = m_numFerns * m_numTestsPerFern;
	m_pFrequency = new quint32[numTests];
	memset(m_pFrequency, 0, numTests * sizeof(quint32));

	m_templateIndex = pContour->index();
	m_template.createMaps(pContour);

	F_CONSOLE("New class for contour #" << m_templateIndex
		<< " (" << m_numFerns << " ferns, " << m_numBits << " bits)");
}

FContourClass::~FContourClass()
{
	F_SAFE_DELETE_ARRAY(m_pFrequency);
}

// Public commands ------------------------------------------------------------------------------------

void FContourClass::setTransform(const FMatrix3f& matNKP)
{
	m_matNKP = matNKP;
}

void FContourClass::serialize(FArchive& ar)
{
	m_template.serialize(ar);

	if (ar.isReading())
	{
		ar >> m_matNKP;

		ar >> m_templateIndex;
		ar >> m_numFerns;
		ar >> m_numBits;

		ar >> m_classFrequency;

		ar >> m_numFerns;
		ar >> m_numBits;

		ar >> m_mseSum;
		ar >> m_dpSum;

		m_numTestsPerFern = (1 << m_numBits);
		quint32 numTests = m_numFerns * m_numTestsPerFern;
		F_SAFE_DELETE_ARRAY(m_pFrequency);
		m_pFrequency = new quint32[numTests];
		for (quint32 i = 0; i < numTests; i++)
			ar >> m_pFrequency[i];
	}
	else
	{
		ar << m_matNKP;

		ar << m_templateIndex;
		ar << m_numFerns;
		ar << m_numBits;

		ar << m_classFrequency;

		ar << m_numFerns;
		ar << m_numBits;

		ar << m_mseSum;
		ar << m_dpSum;

		quint32 numTests = m_numFerns * m_numTestsPerFern;
		for (quint32 i = 0; i < numTests; i++)
			ar << m_pFrequency[i];
	}
}

// Public queries -------------------------------------------------------------------------------------

void FContourClass::dump(QDebug& debug) const
{
	debug.nospace() << "ID: " << m_templateIndex << ", Ferns: " << m_numFerns << ", Bits: " << m_numBits
		<< ", Freq: " << m_classFrequency << ", \tAccuracy: " << fittingAccuracy()
		<< ", \tAmbiguity: " << poseAmbiguity();
}

// ----------------------------------------------------------------------------------------------------