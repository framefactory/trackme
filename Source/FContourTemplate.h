// ----------------------------------------------------------------------------------------------------
//  Title			FContourTemplate.h
//  Description		Header file for FContourTemplate.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-11 20:14:11 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FCONTOURTEMPLATE_H
#define FCONTOURTEMPLATE_H

#include "FTrackMe.h"
#include "FVectorT.h"
#include "FArchive.h"
#include "FPixelStruct.h"

class FContour;
class FGLTextureRect;

// ----------------------------------------------------------------------------------------------------
//  Class FContourTemplate
// ----------------------------------------------------------------------------------------------------

class FContourTemplate
{
	//  Static callbacks -------------------------------------------------------

private:
	inline static void sLevmarUpdate(float* p, float* hx, int m, int n, void* pData) {
		F_ASSERT(pData);
		((FContourTemplate*)pData)->_levmarUpdate(p, hx, m, n);
	}
	inline static void sLevmarJacobian(float* p, float* j, int m, int n, void* pData) {
		F_ASSERT(pData);
		((FContourTemplate*)pData)->_levmarJacobian(p, j, m, n);
	}

	//  Constructors and destructor --------------------------------------------

public:
	/// Default constructor. To be used only during de-serialization.
	FContourTemplate();
	/// Creates a new contour template with the given size.
	FContourTemplate(const QSize& patchSize);
	/// Virtual destructor.
	~FContourTemplate();

private:
	FContourTemplate(const FContourTemplate& other);
	FContourTemplate& operator=(const FContourTemplate& other);

	//  Public commands --------------------------------------------------------

public:
	/// From the given contour, create the distance map and the distance gradient map.
	/// The contour positions must be normalized to (0, 0) - (1, 1).
	void createMaps(const FContour* pContour);

	/// Serialization.
	void serialize(FArchive& ar);

	//  Public queries ---------------------------------------------------------

	/// Matches the given normalized contour pixels to the distance map of
	/// this class by optimizing a homography warp. The function returns the
	/// parameters of the homography. pInfo[0] contains the mean squared error.
	/// If the MSE is < 10.0, pInfo[1] contains the 1st PCA component of the covariance matrix.
	void matchContour(const FContour* pNormalizedContour,
		OUT FMatrix3f& homography, OUT float* pInfo = NULL) const;

	/// Returns the size of the template patch.
	const QSize& patchSize() const { return m_patchSize; }

	/// Draws the warped distance transform map to the given texture.
	/// If a contour and homography are given, the contour is drawn over the distance transform map.
	void drawToTexture(OUT FGLTextureRect& texture,
		const FContour* pContour = NULL, const FMatrix3f* pHomography = NULL) const;

	//  Internal functions -----------------------------------------------------

private:
	void _signedDistanceTransform();
	void _levmarUpdate(float* p, float* hx, int m, int n);
	void _levmarJacobian(float* p, float* j, int m, int n);

	//  Internal data members --------------------------------------------------

private:
	QSize m_patchSize;
	float* m_pDistanceMap;
	FVector2f* m_pGradientMap;

	mutable const FContour* m_pCurrentContour;
};

// ----------------------------------------------------------------------------------------------------

#endif // FCONTOURTEMPLATE_H