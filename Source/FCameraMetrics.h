// ----------------------------------------------------------------------------------------------------
//  Title			FCameraMetrics.h
//  Description		Header file for FCameraMetrics.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-17 00:16:08 +0200 (Mi, 17 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FCAMERAMETRICS_H
#define FCAMERAMETRICS_H

#include "FTrackMe.h"
#include "FVectorT.h"
#include "FMatrix3T.h"
#include "FMatrix4T.h"
#include "FArchive.h"

// ----------------------------------------------------------------------------------------------------
//  Class FCameraMetrics
// ----------------------------------------------------------------------------------------------------

class FCameraMetrics
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Creates a camera metrics object with default aperture size (36 x 27)
	/// and default image resolution (640 x 480).
	FCameraMetrics();

	//  Public commands --------------------------------------------------------

	/// Sets the aperture size of the camera.
	void setApertureSize(const FVector2f& apertureSize) {
		m_apertureSize = apertureSize;
	}
	/// Sets the resolution of the camera in pixels.
	void setResolution(const FVector2f& resolution) {
		m_resolution = resolution;
	}
	/// Sets the offset of the camera center in pixels.
	void setPixelShift(const FVector2f& pixelShift) {
		m_pixelShift = pixelShift;
	}
	/// Sets the image resolution and offset of the camera center from the given image size.
	void setImageSize(const QSize& imageSize) {
		m_resolution.set(imageSize.width(), imageSize.height());
		m_pixelShift.set(m_resolution * 0.5f);
	}

	/// Sets the focal length of the camera.
	void setFocalLength(float focalLength) {
		m_focalLength = focalLength;
	}

	//  Serialization operators ------------------------------------------------

	/// Serialization: write operator.
	friend FArchive& operator<<(FArchive& ar, const FCameraMetrics& obj);
	/// Serialization: read operator.
	friend FArchive& operator>>(FArchive& ar, FCameraMetrics& obj);

	//  Public queries ---------------------------------------------------------

	/// Returns the size of the camera's aperture.
	const FVector2f& apertureSize() const { return m_apertureSize; }
	/// Returns the resolution of the camera image in pixels.
	const FVector2f& resolution() const { return m_resolution; }
	/// Returns the offset of the camera center in pixels.
	const FVector2f& pixelShift() const { return m_pixelShift; }
	/// Returns the focal length of the camera.
	float focalLength() const { return m_focalLength; }

	/// Creates an OpenGL projection matrix mapping from camera to device space.
	void getGLProjectionMatrix(FMatrix4f& matGLProjection) const;
	/// Creates a 4x4 projection matrix mapping from camera to image space.
	void getProjectionMatrix(FMatrix4f& matProjection) const;
	/// Creates a 3x3 camera matrix in 2D projective space.
	void getProjectiveCameraMatrix(FMatrix3f& matCamera) const;
	/// Creates the 3x3 inverse camera matrix in 2D projective space.
	void getInverseProjectiveCameraMatrix(FMatrix3f& matInverseCamera) const;

	/// Writes information about the internal state to the given debug object.
	void dump(QDebug& debug) const;

	//  Internal data members --------------------------------------------------

private:
	FVector2f m_apertureSize;
	FVector2f m_resolution;
	FVector2f m_pixelShift;
	float m_focalLength;
};

// Serialization operators ----------------------------------------------------------------------------

inline FArchive& operator<<(FArchive& ar, const FCameraMetrics& obj)
{
	ar << obj.m_apertureSize;
	ar << obj.m_resolution;
	ar << obj.m_pixelShift;
	ar << obj.m_focalLength;
	return ar;
}

inline FArchive& operator>>(FArchive& ar, FCameraMetrics& obj)
{
	ar >> obj.m_apertureSize;
	ar >> obj.m_resolution;
	ar >> obj.m_pixelShift;
	ar >> obj.m_focalLength;
	return ar;
}

#endif // FCAMERAMETRICS_H