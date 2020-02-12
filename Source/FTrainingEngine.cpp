// ----------------------------------------------------------------------------------------------------
//  Title			FTrainingEngine.cpp
//  Description		Implementation of class FTrainingEngine
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-25 21:02:54 +0200 (Do, 25 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FContourModel.h"
#include "FDistanceTransform.h"
#include "FContourFinder.h"
#include "FContourDatabase.h"
#include "FContourClass.h"
#include "FPatchSizePreset.h"
#include "FPoseDetector.h"

#include "FTrainingEngine.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingEngine
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FTrainingEngine::FTrainingEngine(QWidget* pView, QObject* pParent /* = NULL */)
: QObject(pParent),
  m_pPoseDetector(NULL),
  m_pDTImage(NULL),
  m_trainingCanvasSize(1024, 1024),
  m_templateSize(128, 128),
  m_patchSize(64, 64),
  m_viewSize(1024, 768),
  m_pView(pView),
  m_pModel(NULL),
  m_pDatabase(NULL),
  m_pStatistics(NULL),
  m_trainingRun(0),
  m_warpErrorThreshold(2.0f)

{
	F_ASSERT(pView);
	_initGL();

	m_cameraMetrics.setApertureSize(FVector2f(36.0f, 36.0f));
	m_cameraMetrics.setFocalLength(35.0f);
	m_cameraMetrics.setImageSize(m_trainingCanvasSize);

	m_pDistanceTransform = new FDistanceTransform();
	m_pDistanceTransform->reset(m_trainingCanvasSize);

	m_pContourFinder = new FContourFinder();
	m_pContourFinder->reset(m_trainingCanvasSize);

	for (size_t i = 0; i < FGlobalConstants::MAX_TEMPLATES; i++)
	{
		m_patch[i].setPatchSize(m_patchSize);
		m_pSlotClass[i] = NULL;
	}

	m_pStatistics = new FTrainingStatistics();
}

FTrainingEngine::~FTrainingEngine()
{
	F_SAFE_DELETE(m_pStatistics);
	F_SAFE_DELETE(m_pContourFinder);
	F_SAFE_DELETE(m_pDistanceTransform);
	F_SAFE_DELETE(m_pDatabase);

	F_SAFE_DELETE(m_pPoseDetector);
	F_SAFE_DELETE_ARRAY(m_pDTImage);

	m_glContext.release();
}

// Public commands ------------------------------------------------------------------------------------

void FTrainingEngine::clear()
{
	m_randGen = FRandom();
}

void FTrainingEngine::loadContourModel(const QString& modelFilePath)
{
	m_randGen = FRandom();
	m_trainingRun = 0;

	F_SAFE_DELETE(m_pDatabase);
	m_pDatabase = new FContourDatabase();
	bool result = m_pDatabase->create(modelFilePath, m_templateSize, m_patchSize,
		m_cameraMetrics, FGlobalConstants::NUM_FERNS, FGlobalConstants::NUM_BITS);

	if (result)
	{
		m_pModel = m_pDatabase->contourModel();
		m_pDatabase->setWarpErrorThreshold(m_warpErrorThreshold);
		m_pDatabase->setTrainingParameter(m_params);
		
		// initial run with neutral pose
		runOnce();

		emit postMessage(QString("Successfully loaded %1 contours from model: %2")
			.arg(m_pModel->contourCount()).arg(modelFilePath));
		
		emit postMessage(QString("Database created, patch size: %1 x %2, template size: %3 x %4")
			.arg(m_patchSize.width()).arg(m_patchSize.height()).arg(m_templateSize.width()).arg(m_templateSize.height()));

		emit postDatabaseInfo(QString("Database created\nContour model: %1").arg(m_pModel->filePath()));

		F_CONSOLE("\nDATABASE CREATED\nPatch size: " << m_patchSize << "\nTemplate size: " << m_templateSize);
	}
	else
	{
		m_pModel = NULL;
		F_SAFE_DELETE(m_pDatabase);
		emit postMessage(QString("Failed to load contour model: %1").arg(modelFilePath));
	}
}

bool FTrainingEngine::loadClassifierData(const QString& dataFilePath)
{
	QFile file(dataFilePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		emit postMessage(QString("Failed to open classifier data file: %1").arg(dataFilePath));
		return false;
	}

	FArchive ar(&file, false);
	F_SAFE_DELETE(m_pDatabase);
	m_pDatabase = new FContourDatabase();
	m_pDatabase->serialize(ar);
	m_pModel = m_pDatabase->contourModel();
	m_params = m_pDatabase->trainingParameter();
	m_warpErrorThreshold = m_pDatabase->warpErrorThreshold();

	emit postDatabaseInfo(QString("Database loaded: %1\nContour model: %2")
		.arg(dataFilePath).arg(m_pModel->filePath()));

#ifdef QT_DEBUG
	m_pDatabase->dump(qDebug().nospace());
#endif

	emit postMessage(QString("Classifier data loaded: %1").arg(dataFilePath));
	return true;
}

bool FTrainingEngine::saveClassifierData(const QString& dataFilePath)
{
	if (!m_pDatabase)
		return false;

	QFile file(dataFilePath);
	if (!file.open(QIODevice::WriteOnly))
	{
		emit postMessage(QString("Failed to create classifier data file: %1").arg(dataFilePath));
		return false;
	}

	m_pDatabase->setTrainingParameter(m_params);

	FArchive ar(&file, false);
	m_pDatabase->serialize(ar);

	emit postDatabaseInfo(QString("Database saved: %1\nContour model: %2")
		.arg(dataFilePath).arg(m_pModel->filePath()));

	emit postMessage(QString("Classifier data saved: %1").arg(dataFilePath));
	return true;
}

void FTrainingEngine::runOnce()
{
	if (!m_pDatabase || !m_pDatabase->isValid())
		return;

	m_trainingRun++;

	if (m_pStatistics)
	{
		m_pStatistics->clear();
		m_pStatistics->runCount = m_trainingRun;
	}

	FCameraPose randomPose;
	_generatePose(randomPose);
	_drawContourModel(randomPose);
	_processContours(randomPose);

	if (m_pStatistics)
	{
		size_t n = m_pDatabase->contourCount();
		for (size_t i = 0; i < n; i++)
		{
			m_pStatistics->contour[i].classCount = m_pDatabase->classCount(i);
			m_pStatistics->contour[i].sampleCount = m_pDatabase->sampleCount(i);
		}

		m_pStatistics->totalClassCount = m_pDatabase->totalClassCount();
		m_pStatistics->totalSampleCount = m_pDatabase->totalSampleCount();

		emit postStatistics(*m_pStatistics);
	}
}

void FTrainingEngine::testOnce(QTextStream& protocol)
{
	if (!m_pDatabase || !m_pDatabase->isValid())
		return;

	if (!m_pPoseDetector)
	{
		m_pPoseDetector = new FPoseDetector();
		m_pPoseDetector->reset(m_trainingCanvasSize);
		m_pPoseDetector->setClassifierData(m_pDatabase);

		m_pDTImage = new FDTPixel[m_trainingCanvasSize.width() * m_trainingCanvasSize.height()];
	}

	F_ASSERT(m_pPoseDetector);

	m_trainingRun++;
	FCameraPose randomPose;
	_generatePose(randomPose);

	// transform the model's bounding box by the generated pose
	FMatrix4f matMV;
	randomPose.getModelViewMatrix(matMV);
	FVector4f p[4];
	FVector4f pt1[4];
	p[0] = FVector4f(m_pModel->boundingBox().p0());
	p[1] = FVector4f(m_pModel->boundingBox().p1());
	p[2].set(p[0].x(), p[1].y(), 0.0f, 1.0f);
	p[3].set(p[1].x(), p[0].y(), 0.0f, 1.0f);
	float bbArea = m_pModel->boundingBox().width() * m_pModel->boundingBox().height();
	for (int j = 0; j < 4; j++)
	{
		pt1[j] = matMV * p[j];
		pt1[j].homogenize();
	}

	// run pose estimation
	_drawContourModel(randomPose);
	m_pDistanceTransform->result().read(FGLDataFormat::RGBA, FGLDataType::Float, m_pDTImage);
	size_t n = m_trainingCanvasSize.width() * m_trainingCanvasSize.height();
	for (size_t i = 0; i < n; i++)
		m_pDTImage[i].index = 0.0f;

	m_pPoseDetector->detect(m_pDTImage);
	size_t pc = m_pPoseDetector->poseCount();

	protocol << pc << "\t";
	for (size_t i = 0; i < m_pPoseDetector->poseCount(); i++)
	{
		// transform bounding box by detected pose
		FMatrix4f matMV;
		m_pPoseDetector->detectedPose(i).getModelViewMatrix(matMV);
		FVector4f pt2[4];
		for (int j = 0; j < 4; j++)
		{
			pt2[j] = matMV * p[j];
			pt2[j].homogenize();
		}

		// calculate error
		float squaredError = 0.0f;
		for (int j = 0; j < 4; j++)
		{
			FVector4f d = pt1[j] - pt2[j];
			squaredError += d.x() * d.x() + d.y() * d.y() + d.z() * d.z();
		}
		squaredError = sqrtf(squaredError) / sqrtf(bbArea) * 25.0f; // -> percentage

		// write protocol
		protocol << m_pPoseDetector->poseTypeId(i) << "\t";
		protocol << m_pPoseDetector->poseClassAmbiguity(i) << "\t";
		protocol << m_pPoseDetector->poseClassAccuracy(i) << "\t";
		protocol << m_pPoseDetector->poseFittingError(i) << "\t";
		protocol << m_pPoseDetector->poseReconstructionAngle(i) << "\t";
		protocol << sqrtf(squaredError) * 0.25f << "\t";
	}

	protocol << "\n";
}

void FTrainingEngine::setParameters(const FTrainingParameter& params)
{
	m_params = params;
	updateView();
}

void FTrainingEngine::updateView()
{
	if (!m_pDatabase || !m_pDatabase->isValid())
		return;

	m_glContext.makeCurrent();
	FGLFramebuffer::bindDefault();

	glViewport(0, 100, m_viewSize.width(), m_viewSize.height());
	glClearColor(0.05f, 0.1f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// large viewports
	m_prgContourView.bind();
	m_texContour.bind(0);
	m_overlayViewLarge[0].draw();

	m_prgDistanceTransformView.bind();
	m_pDistanceTransform->result().bind(0);
	m_overlayViewLarge[1].draw();

	// small viewports
	m_prgDTMapView.bind();
	for (size_t i = 0; i < 8; i++)
	{
		if (m_pSlotClass[i])
		{
			m_patch[i].drawToTexture(m_texTemplateView[i]);
			m_pSlotClass[i]->contourTemplate().drawToTexture(m_texTemplateView[i+8],
				m_pSlotContour[i], &m_slotHomography[i]);

			m_texTemplateView[i].bind(0);
			m_overlayViewSmall[i].draw();

			m_texTemplateView[i+8].bind(0);
			m_overlayViewSmall[i+8].draw();
		}
	}

	m_glContext.swapBuffers();
}

// Public queries -------------------------------------------------------------------------------------

QString FTrainingEngine::databaseInfo() const
{
	QString info;
	if (m_pDatabase)
	{
		QDebug debug(&info);
		m_pDatabase->dump(debug);
	}
	else
		info = "No Database loaded";

	return info;
}

// Public slots ---------------------------------------------------------------------------------------

void FTrainingEngine::setParamCamPositionMin(FVector3d val)
{
	m_params.posMin = val;
	_viewPose(-1);
}

void FTrainingEngine::setParamCamPositionMax(FVector3d val)
{
	m_params.posMax = val;
	_viewPose(1);
}

void FTrainingEngine::setParamCamOrbitMin(FVector3d val)
{
	m_params.yprMin = val;
	_viewPose(-1);
}

void FTrainingEngine::setParamCamOrbitMax(FVector3d val)
{
	m_params.yprMax = val;
	_viewPose(1);
}

void FTrainingEngine::setParamCamFocalLength(double val)
{
	m_params.focalLength = val;
	m_cameraMetrics.setFocalLength(val);
	_viewPose(0);
}

void FTrainingEngine::setParamInverseRotationOrder(bool val)
{
	m_params.inverseRotationOrder = val;
	_viewPose(0);
}

void FTrainingEngine::setParamTemplateSize(int index)
{
	if (!m_pDatabase)
	{
		m_templateSize = FPatchSizePreset(index).toSize();
		F_CONSOLE("Template size changed to " << m_templateSize);
	}
}

void FTrainingEngine::setParamPatchSize(int index)
{
	if (!m_pDatabase)
	{
		m_patchSize = FPatchSizePreset(index).toSize();
		F_CONSOLE("Patch size changed to " << m_patchSize);
	}
}

void FTrainingEngine::setParamWarpErrorThreshold(double val)
{
	m_warpErrorThreshold = val;
	if (m_pDatabase)
		m_pDatabase->setWarpErrorThreshold(val);

	F_CONSOLE("Warp mean square error threshold: " << val);
}

// Internal functions ---------------------------------------------------------------------------------

void FTrainingEngine::_generatePose(FCameraPose& cameraPose)
{
	if (m_trainingRun <= 1)
	{
		cameraPose.setTranslation((m_params.posMax + m_params.posMin) * 0.5f);
		cameraPose.setRotationEuler(FVector3f(0.0f, 0.0f, 0.0f));
		cameraPose.setFocalLength(m_params.focalLength);
	}
	else
	{
		FVector3f pos(m_randGen.uniformFloat(), m_randGen.uniformFloat(), m_randGen.uniformFloat());
		FVector3f ypr(m_randGen.uniformFloat(), m_randGen.uniformFloat(), m_randGen.uniformFloat());

		pos *= (m_params.posMax - m_params.posMin);
		pos = m_params.posMin + pos;

		ypr *= (m_params.yprMax - m_params.yprMin);
		ypr = m_params.yprMin + ypr;

		cameraPose.setTranslation(pos);
		cameraPose.setRotationEuler(ypr * (float)FMath::d2r, m_params.inverseRotationOrder);
		cameraPose.setFocalLength(m_params.focalLength);
	}

	if (m_pStatistics)
		m_pStatistics->currentPose = cameraPose;
}

void FTrainingEngine::_drawContourModel(const FCameraPose& cameraPose)
{
	if (!m_pModel)
		return;

	// generate mvp matrix from pose
	FMatrix4f matMV, matProjGL;
	cameraPose.getModelViewMatrix(matMV);
	m_cameraMetrics.getGLProjectionMatrix(matProjGL);

	matProjGL *= matMV;
	matProjGL.transpose(); // OpenGL needs a column-major matrix layout
	m_bufTransform.write(matProjGL.ptr(), 16 * sizeof(float));

	m_fbContour.bind(1);
	glViewport(0, 0, m_trainingCanvasSize.width(), m_trainingCanvasSize.height());
	glClearColor(-1.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	m_prgContourDraw.bind();
	m_prgContourDraw.bindUniformBlock("Transform", 0);
	m_bufTransform.bindUniform(0);
	m_pModel->draw();

	m_pDistanceTransform->calculateDt(m_texContour);
}

void FTrainingEngine::_processContours(const FCameraPose& cameraPose)
{
	for (size_t i = 0; i < FGlobalConstants::MAX_TEMPLATES; i++)
		m_pSlotClass[i] = NULL;

	m_pContourFinder->findContoursTraining(m_pDistanceTransform->result(), *m_pModel);
	quint32 numContours = m_pContourFinder->contourCount();

	for (quint32 i = 0; i < numContours; i++)
	{
		const FContour* pContour = m_pContourFinder->contourAt(i);

		if (pContour->isNormalized())
		{
			size_t tId = pContour->index();
			m_patch[tId].warpImage(m_pContourFinder->dtImage(), m_trainingCanvasSize.width(),
				m_trainingCanvasSize.height(), pContour);

			bool inserted = m_pDatabase->insertContourPose(m_patch[tId], pContour, cameraPose);

			if (tId < MAX_DISPLAY_SLOTS)
			{
				m_pSlotClass[tId] = m_pDatabase->lastClass();
				m_slotHomography[tId] = m_pDatabase->lastHomography();
				m_pSlotContour[tId] = pContour;
			}

			if (m_pStatistics)
			{
				m_pStatistics->contour[tId].isPresent = true;
				m_pStatistics->contour[tId].normalizationScale = pContour->scale();
				m_pStatistics->contour[tId].normalizationAngle = pContour->rotationAngle();
				m_pStatistics->contour[tId].isMatch = !inserted;
				m_pStatistics->contour[tId].meanSquaredError = m_pDatabase->lastMeanSquaredError();
				m_pStatistics->contour[tId].scaledMSE = m_pDatabase->lastScaledMSE();
				m_pStatistics->contour[tId].fittingAccuracy = m_pDatabase->fittingAccuracy(tId);
				m_pStatistics->contour[tId].poseAmbiguity = m_pDatabase->poseAmbiguity(tId);
			}
		}
	}
}

void FTrainingEngine::_viewPose(int poseType)
{
	if (m_pDatabase)
		m_pDatabase->setTrainingParameter(m_params);

	FCameraPose pose;
	
	if (poseType == 1)
		m_params.getMaxPose(pose);
	else if (poseType == -1)
		m_params.getMinPose(pose);
	else
		m_params.getMeanPose(pose);

	_drawContourModel(pose);
	updateView();
}

void FTrainingEngine::_initGL()
{
	FGLContextSettings settings;
	settings.setDepthEnabled(false);
	settings.setStencilEnabled(false);
	settings.setDoubleBufferingEnabled(true);
	settings.setVersion(3, 3);
	m_glContext.create(settings, m_pView);
	m_glContext.makeCurrent();

	m_overlayCanvas.setTexCoords(m_trainingCanvasSize);
	m_overlayCanvas.create();

	FGLShader shadOverlay("Shader/overlay.vert");
	m_prgContourDraw.createLinkProgram("Shader/trainDrawContour.vert", "Shader/trainDrawContour.frag");
	F_ASSERT(m_prgContourDraw.isLinked());
	m_uTransformBlock = m_prgContourDraw.getUniformBlockIndex("Transform");
	m_prgContourDraw.bindAttribLocation(0, "vertPosition");
	m_prgContourDraw.bindAttribLocation(1, "vertIndex");

	m_bufTransform.createAllocate(16 * sizeof(float), FGLUsage::DynamicDraw);

	m_texContour.createAllocate(FGLPixelFormat::R8_UNorm, m_trainingCanvasSize);
	m_fbContour.create();
	m_fbContour.attachColorTexture(m_texContour, 0);
	F_ASSERT(m_fbContour.checkStatus());

	// programs for engine view
	FGLShader shadViewContour("Shader/trainViewContour.frag");
	m_prgContourView.createLinkProgram(shadOverlay, shadViewContour);
	F_ASSERT(m_prgContourView.isLinked());

	FGLShader shadViewDT("Shader/trainViewDistanceTransform.frag");
	m_prgDistanceTransformView.createLinkProgram(shadOverlay, shadViewDT);
	F_ASSERT(m_prgDistanceTransformView.isLinked());

	FGLShader shadViewDTMap("Shader/trainViewDTMap.frag");
	m_prgDTMapView.createLinkProgram(shadOverlay, shadViewDTMap);
	F_ASSERT(m_prgDTMapView.isLinked());

	// init viewports for engine view, 2 large, 2 * 4 small
	FRect2f trainingCanvasRect(0.0f, 0.0f, m_trainingCanvasSize.width(), m_trainingCanvasSize.height());
	FRect2f templateRect(0.0f, 0.0f, m_templateSize.width(), m_templateSize.height());
	FRect2f patchRect(0.0f, 0.0f, m_patchSize.width(), m_patchSize.height());

	float y13 = 1.0f / 3.0f;

	m_overlayViewLarge[0].create(FRect2f(-1.0f, -y13, 0.0f, 1.0f), trainingCanvasRect);
	m_overlayViewLarge[1].create(FRect2f( 0.0f, -y13, 1.0f, 1.0f), trainingCanvasRect);

	for (int i = 0; i < 16; i++)
	{
		int x = i % 8;
		int y = 1 - (i / 8);
		FRect2f coords(-1.0f + x * 0.25f, -1.0f + y * y13, -0.75f + x * 0.25f, -1.0f + (y+1) * y13);
		
		if (i < 8)
		{
			m_overlayViewSmall[i].create(coords, patchRect);
			m_texTemplateView[i].createAllocate(FGLPixelFormat::R32_Float, m_patchSize);
		}
		else
		{
			m_overlayViewSmall[i].create(coords, templateRect);
			m_texTemplateView[i].createAllocate(FGLPixelFormat::R32_Float, m_templateSize);
		}
	}

	F_GLERROR_ASSERT;
}

// ----------------------------------------------------------------------------------------------------