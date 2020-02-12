// ----------------------------------------------------------------------------------------------------
//  Title			FContourDatabase.cpp
//  Description		Implementation of class FContourDatabase
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-25 21:02:54 +0200 (Do, 25 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FContourDatabase.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FContourDatabase
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FContourDatabase::FContourDatabase()
: m_numFerns(0),
  m_numBits(0),
  m_templateSize(0, 0),
  m_patchSize(0, 0),
  m_pLastClass(NULL),
  m_warpErrorThreshold(2.0f),
  m_isValid(false)
{
}

FContourDatabase::~FContourDatabase()
{
	_clear();
}

// Public commands ------------------------------------------------------------------------------------

bool FContourDatabase::insertContourPose(const FContourPatch& contourPatch,
										 const FContour* pContour,
										 const FCameraPose& cameraPose)
{
	F_ASSERT(isValid());
	if (!isValid())
		return false;

	// calculate NKP transformation: Normalize * CameraIntrinsics * ObjectPose
	FMatrix3f matNKP;
	_buildMatrixNKP(cameraPose, m_cameraMetrics, pContour, matNKP);

	// get the ferns descriptor from the contour patch
	size_t contourId = pContour->index();
	quint32 descriptor[MAX_FERNS];
	contourPatch.getDescriptor(m_test, &descriptor[0]);

	// match the contour against all classes of same contour type
	FContourClass* pClass = _matchClass(pContour);
	m_pLastClass = pClass;

	// scale mse to compensate for patch scaling
	// scaled mse is used only to compare against threshold
	float templateArea = (float)(m_templateSize.width() * m_templateSize.height() * 0.25f);
	float areaScale = fMax(1.0f, pContour->scale().x() * pContour->scale().y() * templateArea);
	m_lastScaledMSE = m_lastMSE / areaScale;

	// found matching class
	if (m_lastScaledMSE < m_warpErrorThreshold)
	{
		// calculate difference to template pose of class
		/*
		FMatrix3f remainder;
		remainder.makeIdentity();
		remainder -= (pClass->matrixNKP().inverted() * m_lastHomography * matNKP);
		float tDiff = remainder.frobeniusNorm();
		*/
		FMatrix3f A = m_lastHomography * matNKP;
		FMatrix3f B = pClass->matrixNKP();
		FVector3f p[4] = { FVector3f(-1.0f, -1.0f, 1.0f), FVector3f(1.0f, -1.0f, 1.0f),
			FVector3f(1.0f, 1.0f, 1.0f), FVector3f(-1.0f, 1.0f, 1.0f) };
		float tDiff = 0.0f;
		for (int i = 0; i < 4; i++)
		{
			FVector3f pa = A * p[i]; pa.homogenize();
			FVector3f pb = B * p[i]; pb.homogenize();
			FVector3f d = pa - pb;
			tDiff += d.x() * d.x() + d.y() * d.y();
		}
		tDiff = sqrtf(tDiff) * 12.5f; // 100 / 4 (measures) / 2 (width of unit area) -> percentage

		// update posterior probability
		pClass->increment(&descriptor[0], m_lastMSE, tDiff);
		return false;
	}

	// no match found, insert new class
	pClass = new FContourClass(pContour, m_numFerns, m_numBits, m_templateSize);
	pClass->setTransform(matNKP);
	pClass->increment(&descriptor[0], 0.0f, 0.0f);
	m_data[contourId].push_back(pClass);

	return true;
}

bool FContourDatabase::create(const QString& contourFilePath,
							  const QSize& templateSize,
							  const QSize& patchSize,
							  const FCameraMetrics& cameraIntrinsics,
							  size_t numFerns, size_t numBits)
{
	F_ASSERT(!templateSize.isEmpty());
	F_ASSERT(!patchSize.isEmpty());
	F_ASSERT(numFerns > 0 && numFerns < MAX_FERNS);
	F_ASSERT(numBits > 0 && numBits < MAX_BITS);

	m_isValid = false;

	bool result = m_model.import(contourFilePath);
	if (!result)
		return false;

	m_templateSize = templateSize;
	m_patchSize = patchSize;
	m_cameraMetrics = cameraIntrinsics;
	m_numFerns = numFerns;
	m_numBits = numBits;

	_clear();
	m_data.resize(m_model.contourCount());
	m_test.create(numFerns, numBits, patchSize);

	m_pLastClass = NULL;
	m_lastMSE = 0.0f;
	m_lastHomography.makeZero();

	m_isValid = true;
	return true;
}

void FContourDatabase::serialize(FArchive& ar)
{
	if (ar.isReading())
	{
		_clear();

		QString magic; ar >> magic;
		F_ASSERT(magic == "TRACKMEdatabase5Start");

		m_model.serialize(ar);

		ar >> m_templateSize;
		ar >> m_patchSize;
		ar >> m_cameraMetrics;
		ar >> m_numFerns;
		ar >> m_numBits;
		ar >> m_warpErrorThreshold;
		ar >> m_trainingParameter;

		size_t numContours;
		ar >> numContours;
		m_data.resize(numContours);

		for (size_t i = 0; i < numContours; i++)
		{
			size_t numClasses;
			ar >> numClasses;
			for (size_t j = 0; j < numClasses; j++)
			{
				FContourClass* pClass = new FContourClass();
				pClass->serialize(ar);
				m_data[i].push_back(pClass);
			}
		}

		m_test.serialize(ar);

		ar >> magic;
		m_isValid = (magic == "TRACKMEdatabase5End");
		F_ASSERT(m_isValid);
	}
	else
	{
		ar << QString("TRACKMEdatabase5Start");
		
		m_model.serialize(ar);

		ar << m_templateSize;
		ar << m_patchSize;
		ar << m_cameraMetrics;
		ar << m_numFerns;
		ar << m_numBits;
		ar << m_warpErrorThreshold;
		ar << m_trainingParameter;

		size_t numContours = m_data.size();
		ar << numContours;

		for (size_t i = 0; i < numContours; i++)
		{
			size_t numClasses = m_data[i].size();
			ar << numClasses;
			for (classList_t::iterator it = m_data[i].begin(); it != m_data[i].end(); ++it)
				(*it)->serialize(ar);
		}

		m_test.serialize(ar);

		ar << QString("TRACKMEdatabase5End");
	}
}

// Public queries -------------------------------------------------------------------------------------

void FContourDatabase::getBestClassCandidates(const FContourPatch& contourPatch,
											  FContourClass** ppClassList) const
{
	F_ASSERT(isValid());
	if (!isValid())
		return;

	quint32 descriptor[MAX_FERNS];
	contourPatch.getDescriptor(m_test, &descriptor[0]);
	return _getBestClassCandidates(&descriptor[0], ppClassList);
}

size_t FContourDatabase::sampleCount(size_t index) const
{
	F_ASSERT(index < m_data.size());
	size_t count = 0;
	for (classList_t::const_iterator it = m_data[index].begin(); it != m_data[index].end(); ++it)
		count += (*it)->sampleCount();

	return count;
}

size_t FContourDatabase::totalClassCount() const
{
	size_t count = 0;
	for (size_t c = 0; c < m_data.size(); c++)
		count += m_data[c].size();
	return count;
}

size_t FContourDatabase::totalSampleCount() const
{
	size_t count = 0;
	for (size_t c = 0; c < m_data.size(); c++) {
		for (classList_t::const_iterator it = m_data[c].begin(); it != m_data[c].end(); ++it)
			count += (*it)->sampleCount();
	}
	return count;
}

float FContourDatabase::fittingAccuracy(size_t typeIndex) const
{
	F_ASSERT(typeIndex < m_data.size());

	double sum = 0.0;
	size_t freq = 0;
	for (classList_t::const_iterator it = m_data[typeIndex].begin(); it != m_data[typeIndex].end(); ++it)
	{
		freq += (*it)->sampleCount();
		sum += (*it)->cumulatedFittingAccuracy();
	}
	return (float)(sum / (double)freq);
}

float FContourDatabase::poseAmbiguity(size_t typeIndex) const
{
	F_ASSERT(typeIndex < m_data.size());

	double sum = 0.0;
	size_t freq = 0;
	for (classList_t::const_iterator it = m_data[typeIndex].begin(); it != m_data[typeIndex].end(); ++it)
	{
		freq += (*it)->sampleCount();
		sum += (*it)->cumulatedPoseAmbiguity();
	}
	return (float)(sum / (double)freq);
}

void FContourDatabase::dump(QDebug& debug) const
{
	debug.nospace();
	debug << "\n----- FContourDatrabase -----";
	debug << "\n   Template size: " << m_templateSize;
	debug << "\n   Patch size:    " << m_patchSize;
	debug << "\n   #Ferns:        " << m_numFerns;
	debug << "\n   #Bits:         " << m_numBits;
	debug << "\n";
	debug << "\n   Contours:      " << m_data.size();
	for (size_t i = 0; i < m_data.size(); i++)
		debug << "\n   #" << i << ": " << m_data[i].size() << " classes";
	debug << "\n";

	m_model.dump(debug.nospace());
	m_cameraMetrics.dump(debug.nospace());
	m_trainingParameter.dump(debug.nospace());
	
	for (size_t i = 0; i < m_data.size(); i++)
	{
		debug << "\n\n   Contour Type " << i << ": " << m_data[i].size() << " Classes, Accuracy: "
			<< fittingAccuracy(i) << ", Ambiguity: " << poseAmbiguity(i) << "\n";
		size_t j = 0;
		for (classList_t::const_iterator it = m_data[i].begin(); it != m_data[i].end(); ++it, ++j)
		{
			debug.nospace() << "\n   Class " << j << " - ";
			FContourClass* pClass = *it;
			pClass->dump(debug);
		}
	}
}

// Internal functions ---------------------------------------------------------------------------------

void FContourDatabase::_clear()
{
	for (size_t c = 0; c < m_data.size(); c++)
	{
		for (classList_t::iterator it = m_data[c].begin(); it != m_data[c].end(); ++it)
			F_SAFE_DELETE(*it);
		m_data[c].clear();
	}
}

FContourClass* FContourDatabase::_matchClass(const FContour* pContour)
{
	float minMSE = FLT_MAX;
	FContourClass* pMinClass = NULL;

	quint32 cId = pContour->index();
	F_ASSERT(cId < m_data.size());

	for (classList_t::const_iterator it = m_data[cId].begin(); it != m_data[cId].end(); ++it)
	{
		float info[3];
		FMatrix3f homography;
		(*it)->matchContour(pContour, homography, info);
		float mse = info[0];

		// homography nearly singular
		if (info[2] < 0.25f)
			mse = 1000.0f;

		if (mse < minMSE)
		{
			minMSE = mse;
			m_lastHomography = homography;
			pMinClass = *it;
		}
	}

	if (minMSE < 1000.0f)
	{
		m_lastMSE= minMSE;
	}
	else
	{
		m_lastMSE = 1000.0f;
		m_lastHomography.makeZero();
	}

	return pMinClass;
}

void FContourDatabase::_getBestClassCandidates(quint32* pDescriptor, OUT FContourClass** ppClassList) const
{
	F_ASSERT(ppClassList);

	for (int i = 0; i < 3; i++)
		ppClassList[i] = NULL;

	double p1 = 0.0;
	double p2 = 0.0;
	double p3 = 0.0;

	for (size_t c = 0; c < m_data.size(); c++)
	{
		for (classList_t::const_iterator it = m_data[c].begin(); it != m_data[c].end(); ++it)
		{
			double p = (*it)->probability(pDescriptor);
			if (p > p1)
			{
				ppClassList[2] = ppClassList[1];
				ppClassList[1] = ppClassList[0];
				ppClassList[0] = *it;
				p3 = p2; p2 = p1; p1 = p;
			}
			else if (p > p2)
			{
				ppClassList[2] = ppClassList[1];
				ppClassList[1] = *it;
				p3 = p2; p2 = p;
			}
			else if (p > p3)
			{
				ppClassList[2] = *it;
				p3 = p;
			}
		}
	}
}

void FContourDatabase::_buildMatrixNKP(const FCameraPose& cameraPose,
									   const FCameraMetrics& metrics,
									   const FContour* pContour,
									   OUT FMatrix3f& matNKP)
{
	FMatrix3f matPose;
	cameraPose.getProjectivePoseMatrix(matPose);

	// extract rotation and translation columns from pose and go from RH to LH system
	float pr11 = -matPose(0, 0);
	float pr12 = -matPose(0, 1);
	float pr21 = -matPose(1, 0);
	float pr22 = -matPose(1, 1);
	float pr31 = matPose(2, 0);
	float pr32 = matPose(2, 1);
	float pt1 = -matPose(0, 2);
	float pt2 = -matPose(1, 2);
	float pt3 = matPose(2, 2);

	// camera, scale and center
	float kax = cameraPose.focalLength() * metrics.resolution().x() / metrics.apertureSize().x();
	float kay = cameraPose.focalLength() * metrics.resolution().y() / metrics.apertureSize().y();
	float kcx = metrics.pixelShift().x();
	float kcy = metrics.pixelShift().y();

	// normalization, rotation
	float sin_a = sinf(pContour->rotationAngle());
	float cos_a = cosf(pContour->rotationAngle());
	float nr11 = cos_a;
	float nr12 = -sin_a;
	float nr21 = sin_a;
	float nr22 = cos_a;

	// normalization, scale
	float nsx = pContour->scale().x();
	float nsy = pContour->scale().y();
	
	// normalization, translation
	float ntx = pContour->translation().x();
	float nty = pContour->translation().y();

	// 3x3 Matrix NKP = N * K * P
	matNKP(0, 0) = nsx * (nr11 * kax * pr11 + nr12 * kay * pr21
		+ pr31 * (nr11 * kcx + nr12 * kcy + nr11 * ntx + nr12 * nty));
	matNKP(0, 1) = nsx * (nr11 * kax * pr12 + nr12 * kay * pr22
		+ pr32 * (nr11 * kcx + nr12 * kcy + nr11 * ntx + nr12 * nty));
	matNKP(0, 2) = nsx * (nr11 * kax * pt1 + nr12 * kay * pt2
		+ pt3 * (nr11 * kcx + nr12 * kcy + nr11 * ntx + nr12 * nty));
	matNKP(1, 0) = nsy * (nr21 * kax * pr11 + nr22 * kay * pr21
		+ pr31 * (nr21 * kcx + nr22 * kcy + nr21 * ntx + nr22 * nty));
	matNKP(1, 1) = nsy * (nr21 * kax * pr12 + nr22 * kay * pr22
		+ pr32 * (nr21 * kcx + nr22 * kcy + nr21 * ntx + nr22 * nty));
	matNKP(1, 2) = nsy * (nr21 * kax * pt1 + nr22 * kay * pt2
		+ pt3 * (nr21 * kcx + nr22 * kcy + nr21 * ntx + nr22 * nty));
	matNKP(2, 0) = pr31;
	matNKP(2, 1) = pr32;
	matNKP(2, 2) = pt3;
}

// ----------------------------------------------------------------------------------------------------