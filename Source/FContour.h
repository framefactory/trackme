// ----------------------------------------------------------------------------------------------------
//  Title			FContour.h
//  Description		Header file for FContour.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-09 13:00:10 +0200 (Fr, 09 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FCONTOUR_H
#define FCONTOUR_H

#include "FTrackMe.h"
#include "FlowMath.h"

class FGLTextureRect;

// ----------------------------------------------------------------------------------------------------
//  Class FContour 
// ----------------------------------------------------------------------------------------------------

/// Holds a list of edge pixels belonging to the contour of a shape. Provides methods for
/// fitting an ellipse to the contour and calculates various properties of the contour.
class FContour
{
	friend class FContourFinder;

	//  Static members ---------------------------------------------------------

public:
	static const size_t MAX_CONTOUR_LENGTH = 4096;

	inline static void sLevmarUpdate(float* p, float* hx, int m, int n, void* pData) {
		F_ASSERT(pData);
		((FContour*)pData)->_levmarUpdate(p, hx, m, n);
	}

	//  Public members ---------------------------------------------------------

	FVector2f pos[MAX_CONTOUR_LENGTH];
	int length;
	
	//  Constructors and destructor --------------------------------------------

	/// Default constructor.
	FContour();

	//  Public commands --------------------------------------------------------

	/// Clears the contour's pixels and state.
	void clear();
	/// Calculates the contour's moments and fits an ellipse.
	void process(int imWidth, int imHeight);
	/// Transforms the contour's pixel coordinates according to the ellipse fit.
	void normalize();
	/// Discards the contour because it is not closed.
	void discardNonClosed() { m_isClosed = false; }

	//  Public queries ---------------------------------------------------------

	/// Returns true if the contour has been initialized properly.
	bool isValid() const { return m_isValid; }
	/// Returns true if the contour's coordinates have been normalized.
	bool isNormalized() const { return m_isNormalized; }

	/// Returns the contour's id.
	quint32 index() const { return m_index; }
	/// Returns the barycenter of the contour.
	const FVector2f& barycenter() const { return m_barycenter; }
	/// Returns a unit vector pointing in the direction of the main axis.
	const FVector2f& orientation() const { return m_orientation; }
	/// Returns the axis aligned bounding box of the contour.
	const FRect2f boundingBox() const { return m_boundingBox; }

	/// Returns the contour's fitted ellipse. The ellipse is fitted and then
	/// scaled to include all contour pixels.
	const FEllipse2f& ellipse() const { return m_ellipse; }
	/// Returns the coordinates of the quad including the ellipse.
	/// The quad can be used as texture coordinates for image warping.
	const FQuad2f& quadCoords() const { return m_quadCoords; }

	/// Returns the scale transformation from image space to normalized patch space (-1, 1)
	const FVector2f& scale() const { return m_scale; }
	/// Returns the translation from image space to normalized patch space, i.e. to (0; 0).
	/// The center of the fitted ellipse translates to the origin.
	const FVector2f& translation() const { return m_translation; }
	/// Returns the rotation angle from image space to normalized patch space (in radians).
	float rotationAngle() const { return m_rotationAngle; }

	/// Returns the 3x3 normalization matrix for this contour.
	void getNormalizationMatrix(FMatrix3f& matNormalization) const;
	/// Returns the 3x3 inverse normalization matrix for this contour.
	void getInverseNormalizationMatrix(FMatrix3f& matInverseNormalization) const;

	/// Draws the normalized contour to the given texture.
	void drawToTexture(FGLTextureRect& texture, const QSize& textureSize);

	//  Internal functions -----------------------------------------------------

private:
	void _setIndex(quint32 id) { m_index = id; }
	void _fitEllipse2();
	void _fitEllipse();
	void _levmarUpdate(float* p, float* hx, int m, int n);
	void _dominantAngle(int nx, int ny);

	//  Internal data members --------------------------------------------------

	bool m_isValid;
	bool m_isNormalized;
	bool m_isClosed;

	quint32 m_index;

	FVector2f m_barycenter;
	FRect2f m_boundingBox;
	FVector2f m_orientation;

	FEllipse2f m_ellipse;
	FQuad2f m_quadCoords;
	FVector2f m_scale;
	FVector2f m_translation;
	float m_rotationAngle;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FCONTOUR_H