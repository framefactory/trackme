// ----------------------------------------------------------------------------------------------------
//  Title			FContourPatch.h
//  Description		Header file for FContourPatch.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-11 20:14:11 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FCONTOURPATCH_H
#define FCONTOURPATCH_H

#include "FTrackMe.h"
#include "FQuad2T.h"
#include "FDTPixel.h"
#include "FFernTest.h"

class FGLTextureRect;
class FContour;

// ----------------------------------------------------------------------------------------------------
//  Class FContourPatch
// ----------------------------------------------------------------------------------------------------

/// A normalized distance transform patch of a single contour.
/// Contour patches are created temporarily during training and detection
/// from each contour found in the distance transform image.
class FContourPatch
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FContourPatch();
	/// Creates a patch with the given size.
	FContourPatch(const QSize& patchSize);
	/// Virtual destructor.
	virtual ~FContourPatch();

private:
	FContourPatch(const FContourPatch& other);
	FContourPatch& operator=(const FContourPatch& other);

	//  Public commands --------------------------------------------------------

public:
	/// Creates the warped patch from the given image using the patch coordinates.
	/// Only the DT area corresponding to the given contour index is used, everything
	/// else is masked. If index is set to -1, masking is skipped
	void warpImage(const FDTPixel* pImage, int nx, int ny, const FContour* pContour);

	/// Sets or changes the patch size.
	void setPatchSize(const QSize& patchSize);

	//  Public queries ---------------------------------------------------------

	/// Runs the binary tests of the given FFernTest object on the distance map
	/// and returns an array of unsigned integers holding the descriptor for each fern.
	void getDescriptor(const FFernTest& test, quint32* pResult) const;

	/// Returns the width and height of this patch.
	const QSize& patchSize() const { return m_patchSize; }
	/// Returns the contour this patch is based on.
	const FContour* contour() const { return m_pContour; }

	/// Draws the warped distance transform map to the given texture.
	void drawToTexture(FGLTextureRect& texture);

	//  Internal functions -----------------------------------------------------

private:
	void _fillArea(int x, int y, float minDist);

	//  Simple stack implementation for 2d coordinates
	inline bool push(const FVector2i& p) {
		if (m_stackPointer + 1 < STACK_SIZE) {
			m_pointStack[++m_stackPointer] = p;
			return true;
		}
		else {
			F_ASSERT(false);
			return false;
		}
	}

	inline bool pop(FVector2i& p) {
		if (m_stackPointer >= 0) {
			p = m_pointStack[m_stackPointer--];
			return true;
		}
		else {
			return false;
		}
	}

	inline void emptyStack() {
		m_stackPointer = -1;
	}

	//  Internal data members --------------------------------------------------

private:
	QSize m_patchSize;
	float* m_pPatch;

	// Stack for flood fill
	static const size_t STACK_SIZE = 256;
	FVector2i m_pointStack[STACK_SIZE];
	int m_stackPointer;

	const FContour* m_pContour;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FCONTOURPATCH_H