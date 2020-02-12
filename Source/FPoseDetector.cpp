// ----------------------------------------------------------------------------------------------------
//  Title			FPoseDetector.cpp
//  Description		Implementation of class FPoseDetector
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-09 13:00:10 +0200 (Fr, 09 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include <vector>
#include <algorithm>
#include "Eigen/Dense"
#include "levmar.h"
#include "FPoseDetector.h"
#include "FMemoryTracer.h"

using Eigen::Matrix;
using Eigen::Matrix3f;
using Eigen::Vector3f;

// ----------------------------------------------------------------------------------------------------
//  Class FPoseDetector
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FPoseDetector::FPoseDetector()
: m_isValid(false),
  m_ownDatabase(false),
  m_frameSize(0, 0),
  m_templateSize(48, 48),
  m_patchSize(128, 128),
  m_slotSize(64, 64),
  m_pClassifierData(NULL),
  m_pStatistics(NULL),
  m_edgeThresholdLow(0.02f),
  m_edgeThresholdHigh(0.07f),
  m_warpErrorThreshold(3.0f),
  m_fixedTypeId(-1),
  m_contourPosition(0.0f, 0.0f, 0.0f),
  m_contourRotation(0.0f, 0.0f, 0.0f),
  m_contourScale(1.0f),
  m_poseCount(0)
{
	F_VERIFY(_initGL());
	_changePatchSize(m_patchSize);
	_updateContourRelativePose();
}

FPoseDetector::~FPoseDetector()
{
	if (m_ownDatabase)
		F_SAFE_DELETE(m_pClassifierData);
}

// Public commands ------------------------------------------------------------------------------------

void FPoseDetector::preprocess(const FGLTextureRect& inputImage)
{
	_runCanny(inputImage);
	m_distanceTransform.calculateDt(m_texBuffer[0]);
}

void FPoseDetector::detect(const FGLTextureRect& inputImage, FDetectorStatistics* pStats /* = NULL */)
{
	if (!m_pClassifierData)
	{
		m_poseCount = 0;
		return;
	}

	m_pStatistics = pStats;
	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());

	_runCanny(inputImage);
	m_distanceTransform.calculateDt(m_texBuffer[0]);
	m_contourFinder.findContours(m_distanceTransform.result());
	
	if (m_pStatistics)
	{
		m_stopWatch.reset();
		m_stopWatch.start();
	}

	_matchContours();

	if (m_pStatistics)
	{
		m_pStatistics->timePoseReconstruction = m_stopWatch.stop();
		m_stopWatch.reset();
	}


	FGLFramebuffer::bindDefault();

	F_GLERROR_ASSERT;
}

void FPoseDetector::detect(FDTPixel* pDTImage, FDetectorStatistics* pStats /* = NULL */)
{
	if (!m_pClassifierData)
		return;

	m_pStatistics = pStats;

	m_contourFinder.findContours(pDTImage, pStats);

	if (m_pStatistics)
	{
		m_stopWatch.reset();
		m_stopWatch.start();
	}

	_matchContours();

	if (m_pStatistics)
	{
		m_pStatistics->timePoseReconstruction = m_stopWatch.stop();
		m_stopWatch.reset();
	}

}

void FPoseDetector::detectVerify(const FGLTextureRect& contourImage)
{
	if (!m_pClassifierData)
		return;

	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());
	m_distanceTransform.calculateDt(contourImage);
	m_contourFinder.findContours(m_distanceTransform.result());
	_matchContours();

	FGLFramebuffer::bindDefault();

	F_GLERROR_ASSERT;
}

bool FPoseDetector::reset(const QSize& frameSize)
{
	F_ASSERT(!frameSize.isEmpty());
	if (frameSize.isEmpty())
		return false;

	m_isValid = false;
	m_frameSize = frameSize;

	if (!_resetGL())
		return false;
	if (!m_distanceTransform.reset(m_frameSize))
		return false;
	if (!m_contourFinder.reset(m_frameSize))
		return false;

	float ax = m_cameraMetrics.apertureSize().x();
	float ay = ax / (float)m_frameSize.width() * (float)m_frameSize.height();
	m_cameraMetrics.setApertureSize(FVector2f(ax, ay));
	m_cameraMetrics.setImageSize(m_frameSize);

#ifdef QT_DEBUG
	F_CONSOLE("\n***** DETECTOR RESET *****");
	m_cameraMetrics.dump(qDebug().nospace());
#endif

	m_isValid = true;
	return true;
}

bool FPoseDetector::loadClassifierData(const QString& dataFilePath)
{
	QFile file(dataFilePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		fWarning("Pose Detector", QString("Failed to open classifier data file: %1").arg(dataFilePath));
		return false;
	}

	FArchive ar(&file, false);

	if (m_ownDatabase)
		F_SAFE_DELETE(m_pClassifierData);

	m_ownDatabase = true;
	m_pClassifierData = new FContourDatabase();
	m_pClassifierData->serialize(ar);

	fInfo("Pose Detector", QString("Classifier data loaded: %1").arg(dataFilePath));

	_initializeDatabase();
	return true;
}

void FPoseDetector::setClassifierData(FContourDatabase* pDatabase)
{
	F_ASSERT(pDatabase);

	if (m_ownDatabase)
		F_SAFE_DELETE(m_pClassifierData);
	m_ownDatabase = false;
	m_pClassifierData = pDatabase;

	fInfo("Pose Detector", "Classifier data set");

	_initializeDatabase();
}

// Public queries -------------------------------------------------------------------------------------

void FPoseDetector::getPreprocessingResult(FDTPixel* pData)
{
	m_distanceTransform.getResult(pData);
}

const FGLTextureRect& FPoseDetector::cannyEdges() const
{
	return m_texBuffer[0];
}

const FGLTextureRect& FPoseDetector::distanceTransform() const
{
	return m_distanceTransform.result();
}

const FGLTextureRect& FPoseDetector::contourView()
{
	return m_contourFinder.contourView();
}

void FPoseDetector::drawContourStatistics(FGLCanvas& canvas)
{
	m_contourFinder.drawContourStatistics(canvas);
}

void FPoseDetector::drawDetectionMap()
{
	m_prgDetectionMap.bind();

	size_t n = fMin(m_contourFinder.contourCount(), MAX_DISPLAY_SLOTS);
	for (size_t i = 0; i < n; i++)
	{
		m_patch[i].drawToTexture(m_texPatch[i]);
		m_texPatch[i].bind(0);
		m_overlayPatch[i].draw();
	}

	if (m_pClassifierData)
	{
		for (size_t i = 0; i < m_pClassifierData->contourCount(); ++i)
		{
			contourInfo_t& info = m_contour[i];
			if (info.pClass)
			{
				info.pClass->contourTemplate().drawToTexture(m_texPatch[i + MAX_DISPLAY_SLOTS]);
				m_texPatch[i + MAX_DISPLAY_SLOTS].bind(0);
				m_overlayPatch[i + MAX_DISPLAY_SLOTS].draw();
			}
		}
	}
}

void FPoseDetector::drawDetectedPose()
{
	if (!m_pClassifierData)
		return;

	// generate mvp matrix from pose
	FMatrix4f matMV;
	m_detectedPose[0].getModelViewMatrix(matMV);

	// copy model view matrix to buffer, 2nd position
	matMV.transpose();
	m_bufTransform.write(matMV.ptr(), 16 * sizeof(float), 16 * sizeof(float));

	// build projection matrix
	FMatrix4f matProjGL;
	m_cameraMetrics.getGLProjectionMatrix(matProjGL);
	matProjGL.transpose();

	matMV *= matProjGL;
	m_bufTransform.write(matMV.ptr(), 16 * sizeof(float), 0);

	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());

	m_prgContourModel.bind();
	m_prgContourModel.bindUniformBlock("Transform", 0);
	m_bufTransform.bindUniform(0);
	m_pClassifierData->contourModel()->draw();
	/*
	m_prgSolidModel.bind();
	m_prgSolidModel.bindUniformBlock("Transform", 0);
	m_bufTransform.bindUniform(0);
	
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	m_axisModel.draw();
	glDisable(GL_DEPTH_TEST);
	*/
}

// Internal functions ---------------------------------------------------------------------------------

void FPoseDetector::_initializeDatabase()
{
	F_ASSERT(m_pClassifierData);

	m_cameraMetrics = m_pClassifierData->cameraMetrics();
	float ax = m_cameraMetrics.apertureSize().x();
	float ay = ax / (float)m_frameSize.width() * (float)m_frameSize.height();
	m_cameraMetrics.setApertureSize(FVector2f(ax, ay));
	m_cameraMetrics.setImageSize(m_frameSize);

	m_templateSize = m_pClassifierData->templateSize();
	m_patchSize = m_pClassifierData->patchSize();
	_changePatchSize(m_patchSize);

	for (size_t i = 0; i < m_pClassifierData->contourCount(); ++i)
	{
		m_contour[i].clear();
		m_contour[i].typeId = i;
		m_contour[i].center = m_pClassifierData->contourModel()->contourCenter(i);
	}

	fInfo("Pose Detector", QString("#Contour types: %1, Patch size %2 x %3")
		.arg(m_pClassifierData->contourCount()).arg(m_patchSize.width()).arg(m_patchSize.height()));

#ifdef QT_DEBUG
	F_CONSOLE("\n***** DATABASE DUMP *****");
	m_pClassifierData->dump(qDebug().nospace());
	F_CONSOLE("\n***** CURRENT CAMERA METRICS *****");
	m_cameraMetrics.dump(qDebug().nospace());
#endif
}

void FPoseDetector::_runCanny(const FGLTextureRect& inputImage)
{
	// run 1d horizontal gaussian kernel (result in buffer 0)
	m_prgGaussianHorz.bind();
	m_fbBuffer[0].bind(1);
	inputImage.bind();
	m_overlay.draw();

	// run 1d vertical gaussian kernel (result in buffer 1)
	m_prgGaussianVert.bind();
	m_fbBuffer[1].bind(1);
	m_texBuffer[0].bind();
	m_overlay.draw();

	// run canny edge detection step (result in buffer 0)
	m_prgCannyDetect.bind();
	m_fbBuffer[0].bind(1);
	m_texBuffer[1].bind();
	m_overlay.draw();

	// run canny edge suppression step (result in buffer 1)
	m_prgCannySuppress.bind();
	m_fbBuffer[1].bind(1);
	m_texBuffer[0].bind();
	m_overlay.draw();

	// run canny edge threshold step (result in buffer 0)
	m_prgCannyThreshold.bind();
	glUniform1f(m_uEdgeThresholdLow, m_edgeThresholdLow);
	glUniform1f(m_uEdgeThresholdHigh, m_edgeThresholdHigh);
	m_fbBuffer[0].bind(1);
	m_prgCannyThreshold.setSamplerUniform(0, "sEdges");
	m_texBuffer[1].bind(0);
	m_overlay.draw();

	// run edge thin pass 1 (result in buffer 1)
	m_prgCannyReduce1.bind();
	m_fbBuffer[1].bind(1);
	m_texBuffer[0].bind();
	m_overlay.draw();

	// run edge thin pass 2 (result in buffer 0)
	m_prgCannyReduce2.bind();
	m_fbBuffer[0].bind(1);
	m_texBuffer[1].bind();
	m_overlay.draw();
}

void FPoseDetector::_matchContours()
{
	m_poseCount = 0;

	F_ASSERT(m_pClassifierData);
	size_t typeCount = m_pClassifierData->contourCount();

	for (size_t i = 0; i < typeCount; i++)
		m_contour[i].clear();

	// iterate over all detected contours, assign matches to contour types
	for (quint32 i = 0; i < m_contourFinder.contourCount(); i++)
	{
		const FContour* pContour = m_contourFinder.contourAt(i);
		F_ASSERT(pContour->isNormalized());
		_matchContour(pContour, i);
	}

	if (m_pStatistics)
	{
		m_pStatistics->timeContourAlignment = m_stopWatch.stop();
		m_stopWatch.reset();
		m_stopWatch.start();
	}

	// reconstruct pose for each contour type
	size_t activeTypes = 0;
	size_t lastTypeId = 0;
	for (size_t i = 0; i < typeCount; ++i)
	{
		contourInfo_t& info = m_contour[i];
		if (info.pClass)
		{
			_reconstructPose(info);
			activeTypes++;
			lastTypeId = i;
		}
	}

	if (activeTypes == 1)
	{
		F_CONSOLE("\nEXACTLY ONE CONTOUR POSE AVAILABLE - NO FURTHER OPTIMIZATION");
		contourInfo_t& info = m_contour[lastTypeId];

		m_poseCount = 1;
		FMatrix3f matRot = info.matRot * m_matContourRot;
		m_detectedPose[0].setRotationFromMatrix(matRot);
		m_detectedPose[0].setTranslation((info.translation / m_contourScale)
			+ matRot * m_contourPosition);
		m_detectedPoseInfo[0] = &info;

		return;
	}

	if (activeTypes == 0)
	{
		F_CONSOLE("\nNO MATCHING CONTOURS - POSE RECONSTRUCTION FAILED");
		return;
	}

	if (m_fixedTypeId >= 0 && m_fixedTypeId < typeCount)
	{
		contourInfo_t& info = m_contour[m_fixedTypeId];
		F_CONSOLE("\nPOSE FROM FIXED TYPE ID #" << m_fixedTypeId);

		if (info.pClass)
		{
			m_poseCount = 1;
			FMatrix3f matRot = info.matRot * m_matContourRot;
			m_detectedPose[0].setRotationFromMatrix(matRot);
			m_detectedPose[0].setTranslation((info.translation / m_contourScale)
				+ matRot * m_contourPosition);
			m_detectedPoseInfo[0] = &info;
		}

		return;
	}
	
	// sort poses by pose ambiguity
	std::vector<contourInfo_t*> sortedInfo;
	for (size_t i = 0; i < typeCount; i++)
		if (m_contour[i].pClass)
			sortedInfo.push_back(&m_contour[i]);

	std::sort(sortedInfo.begin(), sortedInfo.end(), contourInfoSortByAmbiguity);

	m_poseCount = sortedInfo.size();
	for (size_t i = 0; i < sortedInfo.size(); i++)
	{
		FMatrix3f matRot = sortedInfo[i]->matRot * m_matContourRot;
		m_detectedPose[i].setRotationFromMatrix(matRot);
		m_detectedPose[i].setTranslation((sortedInfo[i]->translation / m_contourScale)
			+ matRot * m_contourPosition);
		m_detectedPoseInfo[i] = sortedInfo[i];
	}
	
	/*
	// do not sort poses, order by type id
	m_poseCount = 0;
	for (size_t i = 0; i < typeCount; i++)
	{
		contourInfo_t& info = m_contour[i];
		if (info.pClass)
		{
			FMatrix3f matRot = info.matRot * m_matContourRot;
			m_detectedPose[m_poseCount].setRotationFromMatrix(matRot);
			m_detectedPose[m_poseCount].setTranslation((info.translation / m_contourScale)
				+ matRot * m_contourPosition);
			m_detectedPoseInfo[m_poseCount] = &info;
			m_poseCount++;
		}
	}
	*/
	// debug output
	F_CONSOLE("\nTOTAL POSE CANDIDATES: " << m_poseCount);
	for (size_t i = 0; i < m_poseCount; i++)
	{
		F_CONSOLE("   candidate #" << i << " - PA: " << m_detectedPoseInfo[i]->pClass->poseAmbiguity()
			<< ", FA: " << m_detectedPoseInfo[i]->pClass->fittingAccuracy()
			<< ", MSE: " << m_detectedPoseInfo[i]->meanSquareError
			<< ", A: " << m_detectedPoseInfo[i]->reconstructionAngle);
	}

	// optimize plane normal vectors (3rd rotation column)
	/*
	FVector3f normal;
	FVector3f translation;
	normal.makeZero();
	
	float bestAngle = FLT_MAX;
	for (size_t i = 0; i < typeCount; ++i)
		if (m_contour[i].pClass)
			bestAngle = fMin(bestAngle, m_contour[i].reconstructionAngle);

	for (size_t i = 0; i < typeCount; ++i)
	{
		contourInfo_t& info = m_contour[i];
		if (info.pClass)
		{
			float w = fMax(0.01f, 1.0f - (info.reconstructionAngle - bestAngle) * 0.2f);
			normal += info.matRot.col(2) * w;
		}
	}

	normal.normalize();

	for (size_t i = 0; i < typeCount; ++i)
	{
		contourInfo_t& info = m_contour[i];
		if (info.pClass)
		{
			FVector3f r0 = info.matRot.col(0);
			FVector3f r1 = info.matRot.col(1);
			FVector3f r2 = info.matRot.col(2);
			float angle = normal.angleTo(r2);
			if (fabsf(angle) > 0.1f)
			{
				FVector3f axis = normal.cross(r2);
				FQuaternion4f rq(axis.normalized(), -angle);
				info.matRot.setCol(0, rq.rotate(r0));
				info.matRot.setCol(1, rq.rotate(r1));
			}
		}
	}

	// CONTOUR ALIGNMENT
	F_CONSOLE("\n\nCONTOUR ALIGNMENT");

	// chose contours with best fitting accuracy and with lowest pose ambiguity
	size_t bestFA_id;
	float bestFA = FLT_MAX;
	size_t bestPA_id;
	float bestPA = FLT_MAX;


	for (size_t i = 0; i < typeCount; ++i)
	{
		contourInfo_t& info = m_contour[i];
		if (info.pClass)
		{
			float fa = info.pClass->fittingAccuracy();
			if (fa < bestFA)
			{
				bestFA = fa;
				bestFA_id = i;
			}
			float pa = info.pClass->poseAmbiguity();
			if (pa < bestPA)
			{
				bestPA = pa;
				bestPA_id = i;
			}
		}
	}

	// run alignment: align bestFA to bestPA
	m_optActiveContours = activeTypes;
	m_optAlignAnchor = bestPA_id;
	m_optAlignSubject = bestFA_id;

	int numData = m_optActiveContours * 3;
	float params[] = { 0.0f, 0.0f, 0.0f };
	float lmInfo[LM_INFO_SZ];
	float opts[] = { 1e-3, 1e-6, 1e-6, 1e-6, 1e-6 }; // tau, eps1, eps2, eps3, delta

	int iter = slevmar_dif(sLevmarUpdate, params, NULL, 1, numData, 50,
		NULL, lmInfo, NULL, NULL, (void*)this, 0, 0.0f);

	contourInfo_t& info = m_contour[bestFA_id];
	info.alignAngle = params[0];
	info.alignError = lmInfo[1] / (float)numData;

	F_CONSOLE("\nContour #" << bestFA_id << " -> #" << bestPA_id << " - Align Error: "
		<< info.alignError << ", Angle: " << info.alignAngle * (float)FMath::r2d << ", Iter: " << iter);

	FMatrix3f matRotAlign; matRotAlign.makeRotationZ(info.alignAngle);
	info.matRot = info.matRot * matRotAlign;

	m_poseCount = 1;
	FMatrix3f matRot = info.matRot * m_matContourRot;
	m_detectedPose[0].setRotationFromMatrix(matRot);

	// *****
	FVector3f tx = info.translation / m_contourScale;
	m_detectedPose[0].setTranslation(tx + matRot * m_contourPosition);
	// *****

	*/
}

void FPoseDetector::_matchContour(const FContour* pContour, size_t candidateIndex)
{
	F_ASSERT(m_pClassifierData);
	F_CONSOLE("\nCONTOUR MATCHING, CANDIDATE NO. " << candidateIndex);

	// create a patch for the contour
	m_patch[candidateIndex].warpImage(m_contourFinder.dtImage(), m_frameSize.width(),
		m_frameSize.height(), pContour);

	FContourClass* classList[MAX_CLASS_CANDIDATES];
	FMatrix3f homography[MAX_CLASS_CANDIDATES];
	float bestMSE = FLT_MAX;
	int bestIndex = -1;

	// get the 3 best candidates using the descriptor from the normalized patch
	m_pClassifierData->getBestClassCandidates(m_patch[candidateIndex], classList);

	// try to align to each of the candidate templates
	for (int i = 0; i < MAX_CLASS_CANDIDATES; i++)
	{
		if (classList[i])
		{
			float info[3];
			classList[i]->matchContour(pContour, homography[i], info);
			float mse = (info[2] < 0.25f) ? 1000.0f : info[0];
			
			F_CONSOLE("Match candidate " << i << "(template #"
				<< classList[i]->templateIndex() <<"): MSE = " << info[0] << " -> "
				<< (mse < m_warpErrorThreshold ? "ACCEPT" : "reject"));

			if (mse < bestMSE)
			{
				bestMSE = mse;
				bestIndex = i;
			}
		}
	}

	if (bestMSE < m_warpErrorThreshold)
	{
		F_CONSOLE("Best candidate: "<< bestIndex << ", MSE: " << bestMSE);

		size_t typeId = classList[bestIndex]->templateIndex();
		contourInfo_t& info = m_contour[typeId];
		if (info.pClass == NULL || bestMSE < info.meanSquareError)
		{
			info.pPatch = &m_patch[candidateIndex];
			info.pContour = pContour;
			info.pClass = classList[bestIndex];
			info.homography = homography[bestIndex];
			info.meanSquareError = bestMSE;
		}
	}
}

float FPoseDetector::_reconstructPose(contourInfo_t& contour)
{
	FMatrix3f tNKP = contour.pClass->matrixNKP();

	FMatrix3f coKI, coNI;
	m_cameraMetrics.getInverseProjectiveCameraMatrix(coKI);
	contour.pContour->getInverseNormalizationMatrix(coNI);

	FMatrix3f matWI = contour.homography;
	matWI.invert();

	FMatrix3f tpPScale;
	tpPScale.makeScale(-1.0f, -1.0f);
	FMatrix3f matT = tpPScale * coKI * coNI * matWI * tNKP;

	F_CONSOLE("\nPOSE RECONSTRUCTION FROM TYPE #" << contour.pClass->templateIndex() << endl
		<< "Raw pose:\n" << matT.toString());

	FMatrix3f matRotation;
	FVector3f vecTranslation;
	
	FVector3f r0 = matT.col(0);
	FVector3f r1 = matT.col(1);
	FVector3f t = matT.col(2);
	
	float len0 = r0.length();
	float len1 = r1.length();
	float angle = acosf(r0.dot(r1) / (len0 * len1));
	r0 *= (2.0f / (len0 + len1));
	r1 *= (2.0f / (len0 + len1));
	t  *= (2.0f / (len0 + len1));

	FVector3f c2 = r0.cross(r1);
	c2.normalize();
	FVector3f m = r0 + r1;
	m.normalize();
	FVector3f n = m.cross(c2);
	FVector3f c0 = n + m;
	FVector3f c1 = -n + m;
	c0.normalize();
	c1.normalize();
	c2.normalize();

	//F_CONSOLE("Right-Angle-Check: " << c0.dot(c1) << ", " << c1.dot(c2) << ", " << c2.dot(c0));

	contour.reconstructionAngle = fabsf(angle * (float)FMath::r2d - 90.0f);
	contour.matRot.setCols(c0, c1, c2);
	contour.translation = t;

	F_CONSOLE("Pose quality (reconstruction angle):\n" << contour.reconstructionAngle);
	return angle;
}

void FPoseDetector::_levmarUpdate(float* p, float* hx, int m, int n)
{
	F_ASSERT(m == 1);
	F_ASSERT(m_pClassifierData);
	F_ASSERT(m_optAlignSubject < m_pClassifierData->contourCount());
	F_ASSERT(m_optAlignAnchor < m_pClassifierData->contourCount());

	FMatrix3f matRotAlign; matRotAlign.makeRotationZ(p[0]);
	FMatrix3f matRotSubject = m_contour[m_optAlignSubject].matRot * matRotAlign;
	FVector3f transSubject = m_contour[m_optAlignSubject].translation;

	FMatrix3f matRotAnchor = m_contour[m_optAlignAnchor].matRot;
	FVector3f transAnchor = m_contour[m_optAlignAnchor].translation;

	size_t typeCount = m_pClassifierData->contourCount();
	size_t dataId = 0;

	for (size_t t = 0; t < typeCount; t++)
	{
		if (!m_contour[t].pClass)
			continue;

		FVector3f c1 = matRotSubject * m_contour[t].center + transSubject;
		FVector3f c2 = matRotAnchor * m_contour[t].center + transAnchor;
		FVector3f d = c1 - c2;
		hx[dataId++] = d.x();
		hx[dataId++] = d.y();
		hx[dataId++] = d.z();
	}

	F_ASSERT(dataId == m_optActiveContours * 3);
}

void FPoseDetector::_decomposeHomography(const FMatrix3f& homography,
										 FMatrix3f& rotation,
										 FVector3f& translation)
{
	Matrix3f He;
	for (int i = 0; i < 9; i++)
		He.data()[i] = homography.ptr()[i];

	Matrix<float, Eigen::Dynamic, Eigen::Dynamic> HTH = He.transpose() * He;
	Eigen::JacobiSVD< Matrix<float, Eigen::Dynamic, Eigen::Dynamic> > svd(HTH, Eigen::ComputeThinU);

	Vector3f s(svd.singularValues());
	float squaredScale = 1.0f / s(1);
	float scale = sqrtf(squaredScale);
	s *= squaredScale;

	Matrix3f U(svd.matrixU());
	Vector3f v1(U.col(0));
	Vector3f v2(U.col(1));
	Vector3f v3(U.col(2));

	Vector3f r1 = v1 * sqrtf(1.0f - s(2));
	Vector3f r2 = v3 * sqrtf(s(0) - 1.0f);
	float r3 = sqrtf(s(0) - s(2));

	Vector3f u1 = (r1 + r2) / r3;
	Vector3f u2 = (r1 - r2) / r3;

	Matrix3f U1;
	U1.col(0) = v2;
	U1.col(1) = u1;
	U1.col(2) = v2.cross(u1);

	Matrix3f U2;
	U2.col(0) = v2;
	U2.col(1) = u2;
	U2.col(2) = v2.cross(u2);

	He *= scale;
	Vector3f t1 = He * v2;
	Vector3f t2 = He * u1;
	Vector3f t3 = He * u2;

	Matrix3f W1;
	W1.col(0) = t1;
	W1.col(1) = t2;
	W1.col(2) = t1.cross(t2);

	Matrix3f W2;
	W2.col(0) = t1;
	W2.col(1) = t3;
	W2.col(2) = t1.cross(t3);

	// plane normals
	Vector3f n1 = v2.cross(u1);
	if (n1(2) < 0.0f)
		n1 = -n1;

	Vector3f n2 = v2.cross(u2);
	if (n2(2) < 0.0f)
		n2 = -n2;

	// rotations
	Matrix3f R1 = W1 * U1.transpose();
	Matrix3f R2 = W2 * U2.transpose();

	// translations
	t1 = (He - R1) * n1;
	t2 = (He - R2) * n2;

	
	qDebug() << "\nHOMOGRAPHY DECOMPOSITION";
	std::stringstream ss;
	ss << "R1: " << R1 << std::endl;
	ss << "t1: " << t1 << std::endl;
	ss << "n1: " << n1 << std::endl;
	ss << "R2: " << R2 << std::endl;
	ss << "t2: " << t2 << std::endl;
	ss << "n2: " << n2 << std::endl;
	qDebug() << ss.str().c_str();
	
	// decompose rotation (R^T) and set translation (-t)
	float a1 = n1.dot(Vector3f(0.0f, 0.0f, 1.0f));
	float a2 = n2.dot(Vector3f(0.0f, 0.0f, 1.0f));

	Matrix3f R = (a1 > a2) ? R1 : R2;
	Vector3f t = (a1 > a2) ? t1 : t2;

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
			rotation(i, j) = R(j, i);
		translation[i] = t[i];
	}
}

void FPoseDetector::_changePatchSize(const QSize& patchSize)
{
	for (size_t i = 0; i < FGlobalConstants::MAX_CONTOUR_CANDIDATES; i++)
		m_patch[i].setPatchSize(m_patchSize);

	if (m_frameSize.isEmpty())
		return;

	_resetGL();
}

void FPoseDetector::_updateContourRelativePose()
{
	m_matContourRot.makeRotationYPR(m_contourRotation * (float)FMath::d2r);
}

bool FPoseDetector::_initGL()
{
	for (int i = 0; i < 2; i++)
	{
		m_texBuffer[i].create();
		m_fbBuffer[i].create();
	}

	FGLShader shOverlay("Shader/overlay.vert");
	FGLShader shGaussian3x3("Shader/gaussian3x3.frag");
	FGLShader shGaussianHorz("Shader/gaussianHorz.frag");
	FGLShader shGaussianVert("Shader/gaussianVert.frag");
	FGLShader shBoxBlurHorz("Shader/boxBlurHorz.frag");
	FGLShader shBoxBlurVert("Shader/boxBlurVert.frag");
	FGLShader shCannyEdgeDetect("Shader/cannyEdgeDetect.frag");
	FGLShader shCannyEdgeSuppress("Shader/cannyEdgeSuppress.frag");
	FGLShader shCannyEdgeReduce1("Shader/cannyEdgeReduce1.frag");
	FGLShader shCannyEdgeReduce2("Shader/cannyEdgeReduce2.frag");
	FGLShader shCannyEdgeThreshold("Shader/cannyEdgeThreshold.frag");

	m_prgGaussian3x3.createLinkProgram(shOverlay, shGaussian3x3);
	m_prgGaussianHorz.createLinkProgram(shOverlay, shGaussianHorz);
	m_prgGaussianVert.createLinkProgram(shOverlay, shGaussianVert);
	m_prgBoxBlurHorz.createLinkProgram(shOverlay, shBoxBlurHorz);
	m_prgBoxBlurVert.createLinkProgram(shOverlay, shBoxBlurVert);
	m_prgCannyDetect.createLinkProgram(shOverlay, shCannyEdgeDetect);
	m_prgCannySuppress.createLinkProgram(shOverlay, shCannyEdgeSuppress);
	m_prgCannyReduce1.createLinkProgram(shOverlay, shCannyEdgeReduce1);
	m_prgCannyReduce2.createLinkProgram(shOverlay, shCannyEdgeReduce2);
	m_prgCannyThreshold.createLinkProgram(shOverlay, shCannyEdgeThreshold);

	m_uEdgeThresholdLow = m_prgCannyThreshold.getUniformLocation("edgeThresholdLow");
	m_uEdgeThresholdHigh = m_prgCannyThreshold.getUniformLocation("edgeThresholdHigh");

	// detection map
	FGLShader shDTMap("Shader/trainViewDTMap.frag");
	m_prgDetectionMap.createLinkProgram(shOverlay, shDTMap);
	m_texDetectionMap.create();
	m_fbDetectionMap.create();

	// pose draw
	m_axisModel.import("Models/axis.dae");
	m_prgSolidModel.createLinkProgram("Shader/solidModel.vert", "Shader/phong2LightsFixed.frag");
	F_ASSERT(m_prgSolidModel.isLinked());
	m_uTransformBlock1 = m_prgSolidModel.getUniformBlockIndex("Transform");
	m_prgSolidModel.bindAttribLocation(0, "vecPosition");
	m_prgSolidModel.bindAttribLocation(1, "vecNormal");
	m_prgSolidModel.bindAttribLocation(2, "vecTexCoord");

	m_prgContourModel.createLinkProgram("Shader/trainDrawContour.vert", "Shader/detectDrawContour.frag");
	F_ASSERT(m_prgContourModel.isLinked());
	m_uTransformBlock2 = m_prgContourModel.getUniformBlockIndex("Transform");
	m_prgContourModel.bindAttribLocation(0, "vertPosition");
	m_prgContourModel.bindAttribLocation(1, "vertIndex");

	m_bufTransform.createAllocate(32 * sizeof(float), FGLUsage::DynamicDraw);
	
	return F_GLNOERROR;
}

bool FPoseDetector::_resetGL()
{
	F_ASSERT(!m_frameSize.isEmpty());

	m_overlay.setTexCoords(m_frameSize);
	m_overlay.create();

	for (int i = 0; i < 2; i++)
	{
		m_texBuffer[i].allocate(FGLPixelFormat::R32G32B32A32_Float, m_frameSize);
		m_fbBuffer[i].attachColorTexture(m_texBuffer[i], 0);
		F_ASSERT(m_fbBuffer[i].checkStatus());
	}

	// detection map
	m_texDetectionMap.allocate(FGLPixelFormat::R8G8B8A8_UNorm, m_frameSize);
	m_fbDetectionMap.attachColorTexture(m_texDetectionMap, 0);
	F_ASSERT(m_fbDetectionMap.checkStatus());

	FRect2f templateRect(m_templateSize.width(), m_templateSize.height());
	FRect2f patchRect(m_patchSize.width(), m_patchSize.height());

	for (int i = 0; i < MAX_DISPLAY_SLOTS; i++)
	{
		float dx = (float)m_slotSize.width() / (float)m_frameSize.width() * 2.0f;
		float dy = (float)m_slotSize.height() / (float)m_frameSize.height() * 2.0f;

		FRect2f screenRect0(-1.0f + i * dx, 1.0f - dy, -1.0f + (i+1) * dx, 1.0f);
		FRect2f screenRect1(-1.0f + i * dx, 1.0f - 2.0f * dy, -1.0f + (i+1) * dx, 1.0f - dy);
		m_overlayPatch[i].create(screenRect0, patchRect);
		m_overlayPatch[i + MAX_DISPLAY_SLOTS].create(screenRect1, templateRect);
	}

	return F_GLNOERROR;
}

// ----------------------------------------------------------------------------------------------------