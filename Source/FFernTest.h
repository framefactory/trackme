// ----------------------------------------------------------------------------------------------------
//  Title			FFernTest.h
//  Description		Header file for FFernTest.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-14 21:08:00 +0200 (So, 14 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FFERNTEST_H
#define FFERNTEST_H

#include "FTrackMe.h"
#include "FArchive.h"

// ----------------------------------------------------------------------------------------------------
//  Class FFernTest
// ----------------------------------------------------------------------------------------------------

/// Holds a binary test pattern for F ferns x B fern-bits, i.e. a total of F x B x 2 randomly
/// created, Gaussian-distributed test locations for a patch of given size.
class FFernTest
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor for de-serialization only.
	FFernTest();
	/// Creates a collection of binary tests.
	FFernTest(size_t numFerns, size_t numBits, const QSize& patchSize);
	/// Virtual destructor.
	virtual ~FFernTest();

private:
	FFernTest(const FFernTest& other);
	FFernTest& operator=(const FFernTest& other);

	//  Public commands --------------------------------------------------------

public:
	/// Creates the random binary tests using the given seed. For a specific seed >= 0,
	/// the same test sequence is generated each time this function is called.
	void create(int seed = -1);

	/// Creates random binary tests using the given parameters and seed.
	void create(size_t numFerns, size_t numBits, const QSize& patchSize, int seed = -1);

	/// Serialization.
	void serialize(FArchive& ar);

#ifdef QT_DEBUG
	/// Writes information about the internal state to the given debug object.
	void dump(QDebug& debug) const;
#endif

	//  Public queries ---------------------------------------------------------

	/// Returns the two position indices of the test at the given fern and bit.
	inline void testIndex(quint32 fern, quint32 bit, quint32& posIndex0, quint32& posIndex1) const {
		F_ASSERT(m_pPos);
		quint32 i = (fern * m_numBits + bit) * 2;
		posIndex0 = m_pPos[i];
		posIndex1 = m_pPos[i+1];
	}

	/// Returns the two position indices of the test at given table position.
	inline void testIndex(quint32 tableIndex, quint32& posIndex0, quint32& posIndex1) const {
		F_ASSERT(m_pPos);
		posIndex0 = m_pPos[tableIndex];
		posIndex1 = m_pPos[tableIndex+1];
	}

	/// Returns the total number of ferns.
	quint32 numFerns() const { return m_numFerns; }
	/// Returns the total number of bits per fern.
	quint32 numBits() const { return m_numBits; }
	/// Returns the total number of binary tests (each test encompasses 2 locations).
	quint32 numTests() const { return m_numFerns * m_numBits; }

	/// Returns the size of the patch the tests are performed within.
	const QSize& patchSize() const { return m_patchSize; }

	//  Internal data members --------------------------------------------------

private:
	quint32 m_numFerns;
	quint32 m_numBits;
	QSize m_patchSize;

	quint32* m_pPos;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FFERNTEST_H