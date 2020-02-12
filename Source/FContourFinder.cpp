// ----------------------------------------------------------------------------------------------------
//  Title			FContourFinder.cpp
//  Description		Implementation of class FContourFinder
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-09 13:00:10 +0200 (Fr, 09 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FContourFinder.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FContourFinder
// ----------------------------------------------------------------------------------------------------

static const float DIST_MIN = 0.0f;
static const float DIST_MAX = 2.0f;

// Constructors and destructor ------------------------------------------------------------------------

FContourFinder::FContourFinder()
: m_frameSize(0, 0),
  m_pFrameData(NULL),
  m_pFrameDataInt(NULL),
  m_pStatistics(NULL),
  m_contFragCount(0),
  m_contCandCount(0),
  m_useDirectExtraction(true)
{
	m_texContourData.create();
}

FContourFinder::~FContourFinder()
{
	F_SAFE_DELETE_ARRAY(m_pFrameDataInt);
}

// Public commands ------------------------------------------------------------------------------------

void FContourFinder::findContours(const FGLTextureRect& distanceTransform,
								  FDetectorStatistics* pStats /* = NULL */)
{
	F_ASSERT(distanceTransform.isValid());
	m_pStatistics = pStats;

	if (m_pStatistics)
	{
		m_stopWatch.reset();
		m_stopWatch.start();
	}

	// Read the distance transform map back to host memory
	m_pFrameData = m_pFrameDataInt;
	distanceTransform.read(FGLDataFormat::RGBA, FGLDataType::Float, m_pFrameData);

	_clearContourData();
	_prepareDTImage();

	if (m_useDirectExtraction)
		_findContoursDirect();
	else
		_findContoursLevelCurve();
	_findContoursPostProcess();
}

void FContourFinder::findContours(FDTPixel* pDTImage,
								  FDetectorStatistics* pStats /* = NULL */)
{
	F_ASSERT(pDTImage);
	m_pFrameData = pDTImage;
	m_pStatistics = pStats;

	if (m_pStatistics)
	{
		m_stopWatch.reset();
		m_stopWatch.start();
	}

	_clearContourData();
	_prepareDTImage();

	if (m_useDirectExtraction)
		_findContoursDirect();
	else
		_findContoursLevelCurve();
	_findContoursPostProcess();
}

void FContourFinder::findContoursTraining(const FGLTextureRect& distanceTransform,
										  const FContourModel& contourModel)
{
	F_ASSERT(distanceTransform.isValid());
	m_pFrameData = m_pFrameDataInt;
	distanceTransform.read(FGLDataFormat::RGBA, FGLDataType::Float, m_pFrameData);

	_clearContourData();

	FDTPixel* pData = m_pFrameData;
	int nx = m_frameSize.width();
	int ny = m_frameSize.height();

	for (int y = 1; y < ny - 1; y++)
	{
		int yy = y * nx;
		for (int x = 1; x < nx - 1; x++)
		{
			float distance = pData[yy + x].distance;
			float index = pData[yy + x].index;
			float idf = floorf(index * 255.0f + 0.5f) - 1.0f;
			pData[yy + x].index = idf;

			if (distance == 0.0f && index != 0.0f)
			{
				quint32 id = (quint32)idf;

				if (id < FGlobalConstants::MAX_TEMPLATES)
				{
					if (m_contFragments[id].length < FContour::MAX_CONTOUR_LENGTH)
					{
						int p = m_contFragments[id].length++;
						m_contFragments[id].pos[p].set(x, y);
					}
				}
			}
		}
	}

	m_contFragCount = 0;
	m_contCandCount = 0;

	for (size_t i = 0; i < FGlobalConstants::MAX_TEMPLATES; i++)
	{
		FContour* pContour = &m_contFragments[i];
		if (pContour->length > 0)
		{
			m_contFragCount = fMax(m_contFragCount, i + 1);
			pContour->process(m_frameSize.width(), m_frameSize.height());
			if (pContour->isValid())
			{
				pContour->normalize();
				pContour->_setIndex(i);
				m_contCandidates[m_contCandCount] = pContour;
				
				m_contCandCount++;
				if (m_contCandCount >= FGlobalConstants::MAX_CONTOUR_CANDIDATES)
					break;
			}
		}
	}

	F_CONSOLE("\nCONTOUR FINDER\nTraining: Frags: " << m_contFragCount << ", Candidates: " << m_contCandCount);
}

bool FContourFinder::reset(const QSize& frameSize)
{
	F_ASSERT(!frameSize.isEmpty());
	m_frameSize = frameSize;

	_clearContourData();

	F_SAFE_DELETE_ARRAY(m_pFrameData);
	m_pFrameDataInt = new FDTPixel[frameSize.width() * frameSize.height()];
	m_pFrameData = m_pFrameDataInt;

	return (m_pFrameDataInt != NULL);
}

void FContourFinder::setContourExtractionMode(extractionMode_t mode)
{
	m_useDirectExtraction = (mode == Direct);
}

void FContourFinder::drawContourStatistics(FGLCanvas& canvas)
{
	canvas.setCanvasRect(FRect2f(m_frameSize.width(), m_frameSize.height()));

	for (int c = 0; c < m_contCandCount; c++)
	{
		FContour* pContour = m_contCandidates[c];
		F_ASSERT(pContour->isNormalized());

		//canvas.addRectangle(pContour->boundingBox(), FColor::colorLightGray);
		//canvas.addMarker(pContour->barycenter(), 2.0f, FColor::colorWhite);
		//FLine2f line;
		//line.setStart(pContour->barycenter());
		//line.setDirection(pContour->orientation() * 50.0f);
		//canvas.addLine(line, FColor::colorLightGray);
			
		for (int i = 0; i < 4; i++)
		{
			canvas.addLine(pContour->quadCoords().edgeSegment(i),
				FColor::colorWhite);
		}
	}
}

// Public queries -------------------------------------------------------------------------------------

const FGLTextureRect& FContourFinder::contourView()
{
	size_t numBytes = m_frameSize.width() * m_frameSize.height() * sizeof(FDTPixel);

	m_texContourData.initialize(FGLPixelFormat::R32G32B32A32_Float, m_frameSize,
		FGLDataFormat::RGBA, FGLDataType::Float, numBytes, m_pFrameData, false); // Do not flip

	return m_texContourData;
}

// Internal functions ---------------------------------------------------------------------------------

void FContourFinder::_clearContourData()
{
	for (int i = 0; i < FGlobalConstants::MAX_CONTOUR_FRAGMENTS; i++)
		m_contFragments[i].clear();
	m_contFragCount = 0;
	m_contCandCount = 0;
}

void FContourFinder::_prepareDTImage()
{
	int nx = m_frameSize.width();
	int ny = m_frameSize.height();

	// ensure all pixels have index -1
	int n = nx * ny;
	for (int i = 0; i < n; i++)
		m_pFrameData[i].index = -1.0f;

	// set distance to -1 at all border pixels of image
	// so we don't have to check boundary conditions later
	
	int yy = (ny - 1) * nx;

	for (int x = 0; x < nx; x++)
	{
		m_pFrameData[x].distance = -1.0f;
		m_pFrameData[x].offset.makeZero();
	}
	for (int x = yy; x < yy + nx; x++)
	{
		m_pFrameData[x].distance = -1.0f;
		m_pFrameData[x].offset.makeZero();
	}

	for (int y = 0; y < ny; y++)
	{
		m_pFrameData[y * nx].distance = -1.0f;
		m_pFrameData[y * nx].offset.makeZero();
	}
	for (int y = 1; y <= ny; y++)
	{
		m_pFrameData[y * nx - 1].distance = -1.0f;
		m_pFrameData[y * nx - 1].offset.makeZero();
	}
}

void FContourFinder::_findContoursLevelCurve()
{
	int nx = m_frameSize.width();
	int ny = m_frameSize.height();
	FDTPixel* pData = m_pFrameData;

	float contourIndex = 0.0f;
	bool contourLimitExceeded = false;
	const size_t maxContFrags = FGlobalConstants::MAX_CONTOUR_FRAGMENTS;

	// scan dt image for pixels next to a contour and follow the contour
	for (int y = 1; y < ny - 1; y++)
	{
		int yy = y * nx;
		for (int x = 1; x < nx - 1; x++)
		{
			int ci = yy + x;

			// if on the right distance and not visited yet, follow contour
			if (pData[ci].distance > DIST_MIN && pData[ci].distance <= DIST_MAX && pData[ci].index < 0.0f)
			{
				_followContourLevelCurve(pData, ci, nx, ny, contourIndex);
				contourIndex++;

				if ((size_t)contourIndex >= maxContFrags)
				{
					contourLimitExceeded = true;
					F_CONSOLE("FContourFinder::_findContoursLevelCurve - WARNING: Contour fragment limit exceeded");
					break;
				}
			}
		}

		if (contourLimitExceeded)
			break;
	}

	// set index of all pixels according to the contour they belong to
	// collect contour pixels into contour bins
	for (int y = 1; y < ny - 1; y++)
	{
		int yy = y * nx;
		for (int x = 1; x < nx - 1; x++)
		{
			int i = yy + x;
			if (pData[i].distance > 0.0f)
			{
				int sx = x + pData[i].offset.x();
				int sy = y + pData[i].offset.y();
				int si = sy * nx + sx;
				pData[i].index = pData[si].index;
			}
			else if (pData[i].distance == 0.0f)
			{
				int cId = (int)pData[i].index;
				if (cId >= 0 && cId < maxContFrags)
				{
					int j = m_contFragments[cId].length;
					if (j < FContour::MAX_CONTOUR_LENGTH - 1)
					{
						m_contFragments[cId].pos[j].set((float)x, (float)y);
						m_contFragments[cId].length++;
					}
					else
						F_CONSOLE("FContourFinder::_findContours - WARNING: Contour length exceeded");
				}
			}
		}
	}

	if (m_pStatistics)
	{
		m_pStatistics->timeContourExtraction = m_stopWatch.stop();
		m_stopWatch.reset();
		m_stopWatch.start();
	}
}

void FContourFinder::_findContoursDirect()
{
	int nx = m_frameSize.width();
	int ny = m_frameSize.height();
	FDTPixel* pData = m_pFrameData;

	float contourIndex = 0.0f;
	bool contourLimitExceeded = false;
	const size_t maxContFrags = FGlobalConstants::MAX_CONTOUR_FRAGMENTS;

	// scan dt image for pixels next to a contour and follow the contour
	for (int y = 1; y < ny - 1; y++)
	{
		int yy = y * nx;
		for (int x = 1; x < nx - 1; x++)
		{
			int ci = yy + x;

			// if on a contour pixel and not visited yet, follow contour
			if (pData[ci].distance == 0.0f && pData[ci].index < 0.0f)
			{
				bool isClosed = _followContourDirect(pData, ci, nx, ny, contourIndex);
				if (!isClosed)
					m_contFragments[(int)contourIndex].discardNonClosed();
								
				contourIndex++;

				if ((size_t)contourIndex >= maxContFrags)
				{
					contourLimitExceeded = true;
					F_CONSOLE("FContourFinder::_findContoursDirect - WARNING: Contour fragment limit exceeded");
					break;
				}
			}
		}

		if (contourLimitExceeded)
			break;
	}

	// set index of all pixels according to the contour they belong to
	for (int y = 1; y < ny - 1; y++)
	{
		int yy = y * nx;
		for (int x = 1; x < nx - 1; x++)
		{
			int i = yy + x;
			if (pData[i].distance > 0.0f)
			{
				int sx = x + pData[i].offset.x();
				int sy = y + pData[i].offset.y();
				int si = sy * nx + sx;
				pData[i].index = pData[si].index;
			}
		}
	}

	if (m_pStatistics)
	{
		m_pStatistics->timeContourExtraction = m_stopWatch.stop();
		m_stopWatch.reset();
		m_stopWatch.start();
	}
}

void FContourFinder::_findContoursPostProcess()
{
	int nx = m_frameSize.width();
	int ny = m_frameSize.height();
	const size_t maxContFrags = FGlobalConstants::MAX_CONTOUR_FRAGMENTS;

	// calculate mean, variance and bounding box of all contours
	m_contFragCount = 0;
	m_contCandCount = 0;

	for (quint32 c = 0; c < maxContFrags; c++)
	{
		FContour* pContour = &m_contFragments[c];
		if (pContour->length > 0)
		{
			m_contFragCount = fMax(m_contFragCount, c + 1);
			pContour->process(nx, ny);

			if (pContour->isValid())
			{
				pContour->normalize();
				pContour->_setIndex(c);
				m_contCandidates[m_contCandCount] = pContour;
				m_contCandCount++;

				if (m_contCandCount >= FGlobalConstants::MAX_CONTOUR_CANDIDATES)
				{
					F_CONSOLE("FContourFinder::_findContours - WARNING: Contour candidate limit exceeded");
					break;
				}
			}
		}
	} // end for

	F_CONSOLE("\nCONTOUR FINDER\nFrags: " << m_contFragCount << ", Candidates: " << m_contCandCount);

	if (m_pStatistics)
	{
		m_pStatistics->timeContourNormalization = m_stopWatch.stop();
		m_stopWatch.reset();
		m_stopWatch.start();
	}
}

bool FContourFinder::_followContourLevelCurve(FDTPixel* pData,
											  int ci, int nx, int ny,
											  float contourId)
{
	// si: index of seed pixel, ci: index of contour pixel
	int next_ci, prev_si, next_si, nsi;
	int si = -1;
	int prev_ci = -1;
	int restart = 0;
	int start_ci = ci;

	while(true)
	{
		prev_si = si;
		si = ci + pData[ci].offset.y() * nx + pData[ci].offset.x();

		
		// check if contour has already been (partially) traced with another index
		if (si != prev_si && restart == 0 && pData[si].index >= 0.0f && pData[si].index != contourId)
		{
			// change contour id and trace again
			contourId = pData[si].index;
			ci = start_ci;
			si = ci + pData[ci].offset.y() * nx + pData[ci].offset.x();
			restart = 1;
		}
		
		
		// mark current contour and seed pixels (incl. neighbors) as visited
		pData[ci].index = contourId;

		// seed must not be on border
		if (pData[si].distance == 0.0)
		{
			pData[si].index = contourId;

			nsi = si - 1;
			if (pData[nsi].distance == 0.0)
			{
				pData[nsi].index = contourId;
				nsi = nsi - 1;
				if (pData[nsi].distance == 0.0)
					pData[nsi].index = contourId;
			}
			nsi = si - nx;
			if (pData[nsi].distance == 0.0)
			{
				pData[nsi].index = contourId;
				nsi = nsi - nx;
				if (pData[nsi].distance == 0.0)
					pData[nsi].index = contourId;
			}
			nsi = si + 1;
			if (pData[nsi].distance == 0.0)
			{
				pData[nsi].index = contourId;
				nsi = nsi + 1;
				if (pData[nsi].distance == 0.0)
					pData[nsi].index = contourId;
			}
			nsi = si + nx;
			if (pData[nsi].distance == 0.0)
			{
				pData[nsi].index = contourId;
				nsi = nsi + nx;
				if (pData[nsi].distance == 0.0)
					pData[nsi].index = contourId;
			}
		}

		// move on to next contour pixel, first consider direct 4-neighbors
		next_ci = ci - 1;
		if (next_ci != prev_ci && pData[next_ci].index != contourId
			&& pData[next_ci].distance > DIST_MIN && pData[next_ci].distance <= DIST_MAX)
				goto found;

		next_ci = ci - nx;
		if (next_ci != prev_ci && pData[next_ci].index != contourId
			&& pData[next_ci].distance > DIST_MIN && pData[next_ci].distance <= DIST_MAX)
			goto found;

		next_ci = ci + 1;
		if (next_ci != prev_ci && pData[next_ci].index != contourId
			&& pData[next_ci].distance > DIST_MIN && pData[next_ci].distance <= DIST_MAX)
			goto found;

		next_ci = ci + nx;
		if (next_ci != prev_ci && pData[next_ci].index != contourId
			&& pData[next_ci].distance > DIST_MIN && pData[next_ci].distance <= DIST_MAX)
			goto found;
		
		next_ci = ci - 1 - nx;
		if (next_ci != prev_ci && pData[next_ci].index != contourId
			&& pData[next_ci].distance > DIST_MIN && pData[next_ci].distance <= DIST_MAX)
			goto found;

		next_ci = ci + 1 - nx;
		if (next_ci != prev_ci && pData[next_ci].index != contourId
			&& pData[next_ci].distance > DIST_MIN && pData[next_ci].distance <= DIST_MAX)
			goto found;

		next_ci = ci + 1 + nx;
		if (next_ci != prev_ci && pData[next_ci].index != contourId
			&& pData[next_ci].distance > DIST_MIN && pData[next_ci].distance <= DIST_MAX)
			goto found;

		next_ci = ci - 1 + nx;
		if (next_ci != prev_ci && pData[next_ci].index != contourId
			&& pData[next_ci].distance > DIST_MIN && pData[next_ci].distance <= DIST_MAX)
			goto found;
		
		// no further unseen pixels on contour -> abort
		return (restart == 0);

found:
		// make next pixel current
		prev_ci = ci;
		ci = next_ci;

	} // end while(true)
}

bool FContourFinder::_followContourDirect(FDTPixel *pData, int ci, int nx, int ny, float contourId)
{
	int cId = (int)contourId;

	// si: index of seed pixel, ci: index of contour pixel
	int next_ci;
	int prev_ci = -1;
	int start_ci = ci;

	while(true)
	{
		// mark current contour and seed pixels (incl. neighbors) as visited
		pData[ci].index = contourId;
		int visitedCount = 0;

		// move on to next contour pixel, first consider direct 4-neighbors
		next_ci = ci - 1;
		if (next_ci != prev_ci && pData[next_ci].distance == 0.0f)
		{
			visitedCount++;
			if (pData[next_ci].index != contourId)
				goto found;
		}

		next_ci = ci - nx;
		if (next_ci != prev_ci && pData[next_ci].distance == 0.0f)
		{
			visitedCount++;
			if (pData[next_ci].index != contourId)
				goto found;
		}

		next_ci = ci + 1;
		if (next_ci != prev_ci && pData[next_ci].distance == 0.0f)
		{
			visitedCount++;
			if (pData[next_ci].index != contourId)
				goto found;
		}

		next_ci = ci + nx;
		if (next_ci != prev_ci && pData[next_ci].distance == 0.0f)
		{
			visitedCount++;
			if (pData[next_ci].index != contourId)
				goto found;
		}

		// now consider 8-connected neighbors
		next_ci = ci - 1 - nx;
		if (next_ci != prev_ci && pData[next_ci].distance == 0.0f)
		{
			visitedCount++;
			if (pData[next_ci].index != contourId)
				goto found;
		}

		next_ci = ci + 1 - nx;
		if (next_ci != prev_ci && pData[next_ci].distance == 0.0f)
		{
			visitedCount++;
			if (pData[next_ci].index != contourId)
				goto found;
		}

		next_ci = ci + 1 + nx;
		if (next_ci != prev_ci && pData[next_ci].distance == 0.0f)
		{
			visitedCount++;
			if (pData[next_ci].index != contourId)
				goto found;
		}

		next_ci = ci - 1 + nx;
		if (next_ci != prev_ci && pData[next_ci].distance == 0.0f)
		{
			visitedCount++;
			if (pData[next_ci].index != contourId)
				goto found;
		}

		// returns true if the contour is closed, false if not
		return (visitedCount > 0);

found:
		// add current position to list of contour positions
		float py = (float)(next_ci / nx);
		float px = (float)(next_ci % nx);

		int j = m_contFragments[cId].length;
		if (j < FContour::MAX_CONTOUR_LENGTH - 1)
		{
			m_contFragments[cId].pos[j].set((float)px, (float)py);
			m_contFragments[cId].length++;
		}
		else
			F_CONSOLE("FContourFinder::_findContours - WARNING: Contour length exceeded");

		// make next pixel current
		prev_ci = ci;
		ci = next_ci;

	} // end while(true)
}

// ----------------------------------------------------------------------------------------------------