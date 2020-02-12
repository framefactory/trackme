// ----------------------------------------------------------------------------------------------------
//  Title			FFernTest.cpp
//  Description		Implementation of class FFernTest
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-14 21:08:00 +0200 (So, 14 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "FRandom.h"
#include "FFernTest.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FFernTest
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FFernTest::FFernTest()
: m_numFerns(0),
  m_numBits(0),
  m_patchSize(0, 0),
  m_pPos(NULL)
{
}

FFernTest::FFernTest(size_t numFerns, size_t numBits, const QSize& patchSize)
: m_numFerns(numFerns),
  m_numBits(numBits),
  m_patchSize(patchSize)
{
	m_pPos = new quint32[m_numFerns * m_numBits * 2];
	create(-1);
}

FFernTest::~FFernTest()
{
	F_SAFE_DELETE_ARRAY(m_pPos);
}

// Public commands ------------------------------------------------------------------------------------

void FFernTest::create(int seed /* = -1 */)
{
	FRandom rand;
	if (seed >= 0)
		rand.setSeed(seed);

	quint32 mx = m_patchSize.width();
	quint32 my = m_patchSize.height();
	double width = m_patchSize.width();
	double height = m_patchSize.height();
	double cx = width / 2.0;
	double cy = height / 2.0;
	double sx = width / 4.0;
	double sy = height / 4.0;

	quint32 N = m_numFerns * m_numBits * 2;


	for (quint32 i = 0; i < N; i++)
	{
		
		quint32 x = UINT_MAX;
		quint32 y = UINT_MAX;

		/*
		while (x >= mx)
			x = (quint32)(rand.gaussianDouble() * sx + cx);
		while (y >= my)
			y = (quint32)(rand.gaussianDouble() * sy + cy);
		*/

		while (x >= mx)
			x = (quint32)(rand.uniformDouble() * width);
		while (y >= my)
			y = (quint32)(rand.uniformDouble() * height);

		m_pPos[i] = y * mx + x;
	}
}

void FFernTest::create(size_t numFerns, size_t numBits, const QSize& patchSize, int seed /* = -1 */)
{
	F_SAFE_DELETE_ARRAY(m_pPos);
	m_numFerns = numFerns;
	m_numBits = numBits;
	m_patchSize = patchSize;
	m_pPos = new quint32[m_numFerns * m_numBits * 2];

	create(seed);
}

void FFernTest::serialize(FArchive& ar)
{
	if (ar.isReading())
	{
		ar >> m_numFerns;
		ar >> m_numBits;
		ar >> m_patchSize;

		quint32 N = m_numFerns * m_numBits * 2;
		F_SAFE_DELETE_ARRAY(m_pPos);
		m_pPos = new quint32[N];
		for (quint32 i = 0; i < N; i++)
			ar >> m_pPos[i];
	}
	else
	{
		ar << m_numFerns;
		ar << m_numBits;
		ar << m_patchSize;

		quint32 N = m_numFerns * m_numBits * 2;
		for (quint32 i = 0; i < N; i++)
			ar << m_pPos[i];
	}

	//dump();
}

#ifdef QT_DEBUG
void FFernTest::dump(QDebug& debug) const
{
	for (quint32 f = 0; f < m_numFerns; f++)
	{
		for (quint32 b = 0; b < m_numBits; b++)
		{
			quint32 i0, i1;
			testIndex(f, b, i0, i1);
			quint32 x0 = i0 % m_patchSize.width();
			quint32 y0 = i0 / m_patchSize.width();
			quint32 x1 = i1 % m_patchSize.width();
			quint32 y1 = i1 / m_patchSize.width();

			debug << QString("BINARY TEXT f#%0, b#%1: (%2, %3) - (%4, %5)")
				.arg(f).arg(b).arg(x0).arg(y0).arg(x1).arg(y1);
		}
	}
}
#endif

// ----------------------------------------------------------------------------------------------------