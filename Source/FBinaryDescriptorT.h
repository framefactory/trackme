// ----------------------------------------------------------------------------------------------------
//  Title			FBinaryDescriptorT.h
//  Description		Header file for FBinaryDescriptorT.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FBINARYDESCRIPTOR_T
#define FBINARYDESCRIPTOR_T

#include "FTrackMe.h"
#include "FPixelStruct.h"
#include "FVectorT.h"
#include "FRandom.h"

// ----------------------------------------------------------------------------------------------------
//  Class FBinaryDescriptorT
// ----------------------------------------------------------------------------------------------------

/// Test descriptor for the random ferns classifier. Holds 2D coordinates for binary features.
/// The total number of features is N = S*M where M is the number of ferns and S is the number of
/// binary features for each fern. The template parameter T represents the image pixel data type.
template <class PIXELTYPE>
class FBinaryDescriptorT
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FBinaryDescriptorT(size_t numFerns, size_t featuresPerFern);
	/// Virtual destructor.
	virtual ~FBinaryDescriptorT();

	//  Public commands --------------------------------------------------------

public:
	/// Sets the image to run the descriptor on.
	void setImage(const PIXELTYPE* pImage, int sizeX, int sizeY);

	/// (Re-)generates the binary features for the given patch size using
	/// the given random seed. A seed value of -1 indicates to use the current time.
	void createFeatures(int patchSize, int seed = -1);

	//  Public queries ---------------------------------------------------------

	/// Builds the feature descriptor at location (x, y) and writes the result
	/// to the given int array. Each array member receives the results of one fern,
	/// a total of M entries is written where M is the number of ferns.
	void getDescriptorAt(int x, int y, quint32* pDescriptor) const;

	/// Compares the given descriptor to the descriptor at location (x, y)
	/// and returns the total number of different bits.
	quint32 compareDescriptorAt(int x, int y, const quint32* pDescriptor) const;

	//  Internal functions -----------------------------------------------------

private:
	inline float _rand(FRandom& randGen, float sd, float vMin, float vMax)
	{
		float val = randGen.gaussianDouble() * sd;
		return fMax(vMin, fMin(vMax, val));
	}

	template<class PIXELTYPE>
	inline float _pixVal(int x, int y, PIXELTYPE* pData)
	{
		F_ASSERT(x >= 0 && x < m_imageSizeX);
		F_ASSERT(y >= 0 && y < m_imageSizeY);
		const PIXELTYPE& pix = pData[y * m_imageSizeX + x];
		return pix.r * 0.3f + pix.g * 0.59f  + pix.b * 0.11f;
	}

	template<>
	inline double _pixVal<float>(int x, int y, double* pData)
	{
		F_ASSERT(x >= 0 && x < m_imageSizeX);
		F_ASSERT(y >= 0 && y < m_imageSizeY);
		return pData[y * m_imageSizeX + x];
	}

	template<>
	inline float _pixVal<float>(int x, int y, float* pData)
	{
		F_ASSERT(x >= 0 && x < m_imageSizeX);
		F_ASSERT(y >= 0 && y < m_imageSizeY);
		return pData[y * m_imageSizeX + x];
	}

	template<>
	inline int _pixVal<float>(int x, int y, int* pData)
	{
		F_ASSERT(x >= 0 && x < m_imageSizeX);
		F_ASSERT(y >= 0 && y < m_imageSizeY);
		return pData[y * m_imageSizeX + x];
	}

	//  Internal data members --------------------------------------------------

private:
	struct feature_t
	{
		int x0, y0;
		int x1, y1;
	};

	typedef std::vector<feature_t> featureVec_t;
	featureVec_t m_features;
	size_t m_numFerns;
	size_t m_numFeatures;

	PIXELTYPE* m_pImage;
	int m_imageSizeX;
	int m_imageSizeY;
};

// ----------------------------------------------------------------------------------------------------

template<class PIXELTYPE>
void FBinaryDescriptorT<PIXELTYPE>::FBinaryDescriptorT(size_t numFerns, size_t featuresPerFern)
: m_numFerns(numFerns),
  m_numFeatures(featuresPerFern)
{
	F_ASSERT(numFerns > 0);
	F_ASSERT(featuresPerFern > 0 && featuresPerFern <= 32);
	m_features.resize(m_numFerns * m_numFeatures);
}

template<class PIXELTYPE>
void FBinaryDescriptorT<PIXELTYPE>::setImage(const PIXELTYPE* pImage, int sizeX, int sizeY)
{
	F_ASSERT(sizeX > 0 && sizeY > 0);

	m_pImage = pImage;
	m_imageSizeX = sizeX;
	m_imageSizeY = sizeY;
}

template<class PIXELTYPE>
void FBinaryDescriptorT<PIXELTYPE>::createFeatures(int patchSize, int seed /* = -1 */)
{
	FRandom randGen;
	if (seed > -1)
		randGen.setSeed(seed);

	size_t count = m_numFerns * m_numFeatures;
	
	float vMax = 0.5f * (float)patchSize;
	float vMin = -vMax;
	float sd = 0.2f * (float)patchSize;


	for (size_t i = 0; i < count; i++)
	{
		feature_t& feat = m_features[i];
		feat.x0 = _rand(randGen, sd, vMin, vMax);
		feat.y0 = _rand(randGen, sd, vMin, vMax);
		feat.x1 = _rand(randGen, sd, vMin, vMax);
		feat.y1 = _rand(randGen, sd, vMin, vMax);
	}
}

template<class PIXELTYPE>
void FBinaryDescriptorT<PIXELTYPE>::getDescriptorAt(int x, int y, quint32* pDescriptor)
{
	for (int f = 0; f < m_numFerns; f++)
	{
		int ff = f * m_numFeatures;
		quint32 d = 0;

		for (int s = 0; s < m_numFeatures; s++)
		{
			int i = ff + s;
			feature_t& feat = m_features[i];
			int px0 = (x + feat.x0) % m_imageSizeX;
			int py0 = (y + feat.y0) % m_imageSizeY;
			int px1 = (x + feat.x1) % m_imageSizeX;
			int py1 = (y + feat.y1) % m_imageSizeY;

			float v0 = _pixVal(px0, py0, m_pImage);
			float v1 = _pixVal(px1, py1, m_pImage);
			if (v1 > v0)
				d++;
			d << 1;
		}

		pDescriptor[f] = d;
	}
}

template<class PIXELTYPE>
quint32 FBinaryDescriptorT<PIXELTYPE>::compareDescriptorAt(int x, int y, const quint32* pDescriptor)
{
	int diffBits = 0;
	for (int f = 0; f < m_numFerns; f++)
	{
		int ff = f * m_numFeatures;
		quint32 d = 0;

		for (int s = 0; s < m_numFeatures; s++)
		{
			int i = ff + s;
			feature_t& feat = m_features[i];
			int px0 = (x + feat.x0) % m_imageSizeX;
			int py0 = (y + feat.y0) % m_imageSizeY;
			int px1 = (x + feat.x1) % m_imageSizeX;
			int py1 = (y + feat.y1) % m_imageSizeY;

			float v0 = _pixVal(px0, py0, m_pImage);
			float v1 = _pixVal(px1, py1, m_pImage);
			if (v1 > v0)
				d++;
			d << 1;
		}

		quint32 c = d ^ pDescriptor[f];
		for (quint32 bp = 1; bp != 0; bp <<= 1)
			diffBits += c & bp;
	}

	return diffBits;
}

// ----------------------------------------------------------------------------------------------------

#endif // FBINARYDESCRIPTOR_T