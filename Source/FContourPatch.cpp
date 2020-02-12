// ----------------------------------------------------------------------------------------------------
//  Title			FContourPatch.cpp
//  Description		Implementation of class FContourPatch
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-11 20:14:11 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FlowGL.h"
#include "FContour.h"

#include "FContourPatch.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FContourPatch
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FContourPatch::FContourPatch()
: m_patchSize(0, 0),
  m_pPatch(NULL),
  m_pContour(NULL)
{
}

FContourPatch::FContourPatch(const QSize& patchSize)
: m_patchSize(patchSize),
  m_pContour(NULL)
{
	F_ASSERT(!patchSize.isEmpty());
	m_pPatch = new float[patchSize.width() * patchSize.height()];
}

FContourPatch::~FContourPatch()
{
	F_SAFE_DELETE_ARRAY(m_pPatch);
}

// Public commands ------------------------------------------------------------------------------------

void FContourPatch::warpImage(const FDTPixel* pImage, int nx, int ny, const FContour* pContour)
{
	F_ASSERT(pImage);
	F_ASSERT(pContour);
	F_ASSERT(!m_patchSize.isEmpty());
	F_ASSERT(m_pPatch != NULL);

	m_pContour = pContour;
	int index = pContour->index();

	FVector2f p0 = pContour->quadCoords()[0];
	FVector2f p1 = pContour->quadCoords()[1];
	FVector2f d0 = pContour->quadCoords()[3] - p0;
	FVector2f d1 = pContour->quadCoords()[2] - p1;

	int sx = m_patchSize.width();
	int sy = m_patchSize.height();

	float dx = 1.0f / sx;
	float dy = 1.0f / sy;

	for (int y = 0; y < sy; y++)
	{
		float ty = y * dy;
		FVector2f p2 = p0 + d0 * ty;
		FVector2f d2 = p1 + d1 * ty - p2;
		int yy = y * sx;

		for (int x = 0; x < sx; x++)
		{
			float tx = x * dx;
			FVector2f p = p2 + tx * d2;

			// coordinates of the four neighboring pixels for bi-linear interpolation
			int ix0 = p.x();
			int ix1 = ix0 + 1;
			float ixt = p.x() - ix0;
			float ixtInv = 1.0f - ixt;
			int iy0 = p.y();
			int iy1 = iy0 + 1;
			float iyt = p.y() - iy0;
			float iytInv = 1.0f - iyt;

			if(ix0 >= 0 && ix1 < nx && iy0 >= 0 && iy1 < ny)
			{
				const FDTPixel& pix00 = pImage[iy0 * nx + ix0];
				const FDTPixel& pix01 = pImage[iy0 * nx + ix1];
				const FDTPixel& pix10 = pImage[iy1 * nx + ix0];
				const FDTPixel& pix11 = pImage[iy1 * nx + ix1];

				int i00 = (int)pix00.index;
				int i01 = (int)pix01.index;
				int i10 = (int)pix10.index;
				int i11 = (int)pix11.index;

				// bi-linear interpolation of distance value
				float d = (pix00.distance * ixtInv + pix01.distance * ixt) * iytInv
					+ (pix10.distance * ixtInv + pix11.distance * ixt) * iyt;

				if (index == -1 || (i00 == index && i01 == index && i10 == index && i11 == index))
				{
					m_pPatch[yy + x] = d;
				}
				else // mask out pixels with a different contour index
				{
					m_pPatch[yy + x] = -d - 10.0f;
				}
			}
			else // mask out pixels outside image boundaries
			{
				m_pPatch[yy + x] = FLT_MAX;
			}

		} // for x
	} // for y

	// mark outer area of contour shape
	float minSqDist = 7.0f * 7.0f;
	float neqSqDist = -minSqDist - 10.0f;
	int y = sy - 1;
	int yy = y * sx;
	
	for (int x = 0; x < sx; x++)
	{
		if (m_pPatch[x] >= minSqDist || m_pPatch[x] <= neqSqDist)
			_fillArea(x, 0, minSqDist);
		if (m_pPatch[yy + x] >= minSqDist || m_pPatch[yy + x] <= neqSqDist)
			_fillArea(x, y, minSqDist);
	}

	int x = sx - 1;
	for (int y = 0; y < sy; y++)
	{
		if (m_pPatch[y * sx] >= minSqDist || m_pPatch[y * sx] <= neqSqDist)
			_fillArea(0, y, minSqDist);
		if (m_pPatch[y * sx + x] >= minSqDist || m_pPatch[y * sx + x] <= neqSqDist)
			_fillArea(x, y, minSqDist);
	}

	// mark inner area of contour image
	for (int i = 0, n = sx * sy; i < n; i++)
		if (m_pPatch[i] < -10.0f)
			//m_pPatch[i] = -3.0f;
			m_pPatch[i] = -m_pPatch[i] - 10.0f;
}

void FContourPatch::setPatchSize(const QSize& patchSize)
{
	F_ASSERT(!patchSize.isEmpty());
	F_SAFE_DELETE_ARRAY(m_pPatch);
	m_patchSize = patchSize;
	m_pPatch = new float[patchSize.width() * patchSize.height()];
}

// Public queries -------------------------------------------------------------------------------------

void FContourPatch::getDescriptor(const FFernTest& test, quint32* pResult) const
{
	quint32 nf = test.numFerns();
	quint32 nb = test.numBits();

	for (quint32 f = 0; f < nf; f++)
	{
		quint32 dc = 0;
		
		for (quint32 b = 0; b < nb; b++)
		{
			quint32 i0, i1;
			test.testIndex(f, b, i0, i1);
			F_ASSERT(i0 < m_patchSize.width() * m_patchSize.height());
			F_ASSERT(i1 < m_patchSize.width() * m_patchSize.height());

			float d0 = m_pPatch[i0];
			float d1 = m_pPatch[i1];

			dc <<= 1;
			if (d0 >= 0.0f && d1 > d0)
				dc++;
		}

		F_ASSERT(dc < (1 << nb));
		pResult[f] = dc;
	}
}

void FContourPatch::drawToTexture(FGLTextureRect& texture)
{
	size_t numBytes = m_patchSize.width() * m_patchSize.height() * sizeof(float);

	texture.createInitialize(FGLPixelFormat::R32_Float, m_patchSize,
		FGLDataFormat::Red, FGLDataType::Float, numBytes, m_pPatch, false); // DO NOT FLIP
}

// Internal functions ---------------------------------------------------------------------------------

void FContourPatch::_fillArea(int x, int y, float minDist)
{
	int cx = m_patchSize.width();
	int cy = m_patchSize.height();

	bool spanLeft, spanRight;
	float* pLine = NULL;
	float pixel = 0.0f;
	FVector2i p(x, y);

	emptyStack();
	push(p);

	while(pop(p))
	{    
		x = p.x();
		y = p.y();

		pLine = m_pPatch + y * cx;
		pixel = pLine[x];

		while (pixel >= minDist || pixel <= -10.0f)
		{
			y--;
			if (y < 0)
				break;

			pLine = m_pPatch + y * cx;
			pixel = pLine[x];
		}

		y++;
		spanLeft = spanRight = false;

		if (y < cy)
		{
			pLine = m_pPatch + y * cx;
			pixel = pLine[x];

			while (pixel >= minDist || pixel <= -10.0f)
			{
				if (pLine[x] < -10.0f)
					pLine[x] = -1.0f;
				else
					pLine[x] = -2.0f;

				if (x > 0)
				{
					BOOL isFeat = (pLine[x-1] >= minDist || pLine[x-1] <= -10.0f);
					if (!spanLeft && isFeat)
					{
						if (!push(FVector2i(x - 1, y)))
							return;
						spanLeft = true;
					}
					else if (spanLeft && !isFeat)
						spanLeft = false;
				}
				if (x < cx - 1)
				{
					BOOL isFeat = (pLine[x+1] >= minDist || pLine[x+1] <= -10.0f);
					if (!spanRight && isFeat)
					{
						if (!push(FVector2i(x + 1, y)))
							return;
						spanRight = true;
					}
					else if (spanRight && !isFeat)
						spanRight = false;
				}

				y++;
				if (y >= cy)
					break;

				pLine = m_pPatch + y * cx;
				pixel = pLine[x];
			}

		}
	}
}

// ----------------------------------------------------------------------------------------------------