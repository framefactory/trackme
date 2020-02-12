// ----------------------------------------------------------------------------------------------------
//  Title			FParameterController.cpp
//  Description		Implementation of class FParameterController
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "FStreamEngine.h"
#include "FStreamProcessor.h"
#include "FParameterController.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FParameterController
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FParameterController::FParameterController(QWidget* pParent /* = NULL */)
: FTreeWidget(pParent),
  m_pEngine(NULL),
  m_pProcessor(NULL),
  m_initialized(false),
  m_isReading(false),
  m_modelIndex(0)
{
}

FParameterController::~FParameterController()
{
}

// Public commands ------------------------------------------------------------------------------------

void FParameterController::setStreamEngine(FStreamEngine* pEngine)
{
	F_ASSERT(pEngine && !m_initialized);
	m_pEngine = pEngine;

	if (m_pEngine && m_pProcessor && !m_initialized)
		_initialize();
}

void FParameterController::setStreamProcessor(FStreamProcessor* pProcessor)
{
	F_ASSERT(pProcessor && !m_initialized);
	m_pProcessor = pProcessor;

	if (m_pEngine && m_pProcessor && !m_initialized)
		_initialize();
}

void FParameterController::activatePreset(int index)
{
	selectModel(index);
}

void FParameterController::selectModel(int index)
{
	m_modelIndex = index;

	switch(index)
	{
	case 0: // Simple Cube I
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(0.0, 0.0, -225.0));
			m_pCamRotation->setValues(FVector3d(27.0, 0.0, 0.0));
			m_pCamFocalDist->setValue(35.0);
			m_pCamAperture->setValues(FVector2d(36.0, 27.0));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(0.0, 7.1, 0.0));
			m_pAugRotation->setValues(FVector3d(35.0, 0.0, 0.0));
			m_pAugScale->setValue(1.0);
		}
		emit modelChanged(0);
		emit mediaFileChanged("../../../Design/Models/Simple Cube/Rendered/Simple Cube C/Simple Cube C0000.tif");
		break;

	case 1: // Simple Cube II
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(0.0, 0.0, -225.0));
			m_pCamRotation->setValues(FVector3d(27.0, 0.0, 0.0));
			m_pCamFocalDist->setValue(35.0);
			m_pCamAperture->setValues(FVector2d(36.0, 27.0));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(0.0, 7.1, 0.0));
			m_pAugRotation->setValues(FVector3d(35.0, 0.0, 0.0));
			m_pAugScale->setValue(1.0);
		}
		emit modelChanged(0);
		emit mediaFileChanged("../../../Design/Models/ShapeR/TestSequence A/TestSequence A0000.tif");
		//emit mediaFileChanged("../../../Design/Models/SF Cube/Rendered/Detection Tests/Detection 01.tif");
		//emit mediaFileChanged("../../../Design/Models/Simple Cube/Rendered/Simple Cube E/Simple Cube E0000.tif");
		break;

	case 2: // SF Cube
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(0.0, 0.0, -225.0));
			m_pCamRotation->setValues(FVector3d(27.0, 0.0, 0.0));
			m_pCamFocalDist->setValue(35.0);
			m_pCamAperture->setValues(FVector2d(36.0, 27.0));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(0.0, 7.1, 0.0));
			m_pAugRotation->setValues(FVector3d(35.0, 0.0, 0.0));
			m_pAugScale->setValue(1.0);
		}
		emit modelChanged(1);
		emit mediaFileChanged("../../../Design/Models/SF Cube/Rendered/SF Cube Simple/Simple Cube B0000.tif");
		break;

	case 3: // SF Cube / Zoom
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(0.0, 0.0, -225.0));
			m_pCamRotation->setValues(FVector3d(27.0, 0.0, 0.0));
			m_pCamFocalDist->setValue(18.0);
			m_pCamAperture->setValues(FVector2d(36.0, 27.0));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(0.0, 7.1, 0.0));
			m_pAugRotation->setValues(FVector3d(35.0, 0.0, 0.0));
			m_pAugScale->setValue(1.0);
		}
		emit modelChanged(1);
		emit mediaFileChanged("../../../Design/Models/SF Cube/Rendered/SF Cube Zoom A/SF Cube Zoom A0000.tif");
		break;

	case 4: // Noisy Cube
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(0.0, 0.0, -225.0));
			m_pCamRotation->setValues(FVector3d(27.0, 0.0, 0.0));
			m_pCamFocalDist->setValue(35.0);
			m_pCamAperture->setValues(FVector2d(36.0, 27.0));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(0.0, 7.1, 0.0));
			m_pAugRotation->setValues(FVector3d(35.0, 0.0, 0.0));
			m_pAugScale->setValue(1.0);
		}
		emit modelChanged(0);
		emit mediaFileChanged("../../../Design/Models/Simple Cube/Rendered/Simple Cube D Noisy/Simple Cube D Noisy 0000.tif");
		break;

	case 5: // Masked Cube
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(0.0, 0.0, -225.0));
			m_pCamRotation->setValues(FVector3d(27.0, 0.0, 0.0));
			m_pCamFocalDist->setValue(35.0);
			m_pCamAperture->setValues(FVector2d(36.0, 27.0));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(0.0, 7.1, 0.0));
			m_pAugRotation->setValues(FVector3d(35.0, 0.0, 0.0));
			m_pAugScale->setValue(1.0);
		}
		emit modelChanged(0);
		emit mediaFileChanged("../../../Design/Models/Simple Cube/Rendered/Simple Cube D Mask/Simple Cube D Mask 0000.tif");
		break;

	case 6: // SF Anchor Object
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(0.0, 0.0, -35.0));
			m_pCamRotation->setValues(FVector3d(30.0, 0.0, 0.0));
			m_pCamFocalDist->setValue(27.0);
			m_pCamAperture->setValues(FVector2d(36.0, 27.0));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(0.0, 1.4, 0.0));
			m_pAugRotation->setValues(FVector3d(0.0, 0.0, 0.0));
			m_pAugScale->setValue(0.2);
		}
		emit modelChanged(2);
		break;

	case 7: // Anchor Bench Object
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(0.0, 0.0, -35.0));
			m_pCamRotation->setValues(FVector3d(30.0, 0.0, 0.0));
			m_pCamFocalDist->setValue(27.0);
			m_pCamAperture->setValues(FVector2d(36.0, 27.0));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(0.0, 1.4, 0.0));
			m_pAugRotation->setValues(FVector3d(0.0, 0.0, 0.0));
			m_pAugScale->setValue(0.2);
		}
		emit modelChanged(8);
		break;

	case 8: // ECO Studio A
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(-9.1, -11.4, -6.0));
			m_pCamRotation->setValues(FVector3d(15.7, -11.0, 0.4));
			m_pCamFocalDist->setValue(18.5);
			m_pCamAperture->setValues(FVector2d(36.0, 20.35));
			m_pCamRadialDist->setValues(FVector2d(-12.0, -5.0));
			m_pAugPosition->setValues(FVector3d(5.3, -0.2, -27.7));
			m_pAugRotation->setValues(FVector3d(145.0, 0.0, 0.0));
			m_pAugScale->setValue(0.2);
		}
		emit modelChanged(3);
		emit mediaFileChanged("../../../Design/Models/Real Studio ECO/StudioTest A/StudioTest A 0000.tif");
		break;

	case 9: // ECO Studio B w/Anchor
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(5.8, -7.7, -5.1));
			m_pCamRotation->setValues(FVector3d(-0.2, 16.4, 1.6));
			m_pCamFocalDist->setValue(18.2);
			m_pCamAperture->setValues(FVector2d(36.0, 20.45));
			m_pCamRadialDist->setValues(FVector2d(-16.0, -4.0));
			m_pAugPosition->setValues(FVector3d(5.3, -0.2, -27.7));
			m_pAugRotation->setValues(FVector3d(145.0, 0.0, 0.0));
			m_pAugScale->setValue(0.2);
		}
		emit modelChanged(5);
		emit mediaFileChanged("../../../Design/Models/Real Studio ECO/StudioTest B ECO/StudioTest B ECO 0000.tif");
		break;

	case 10: // ECO Studio/Table Pan
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(12.07, -14.06, -6.54));
			m_pCamRotation->setValues(FVector3d(20.26, -8.12, -0.17));
			m_pCamFocalDist->setValue(18.4);
			m_pCamAperture->setValues(FVector2d(36.0, 20.3));
			m_pCamRadialDist->setValues(FVector2d(-12.0, -4.0));
			m_pAugPosition->setValues(FVector3d(-2.8, -0.2, -27.7));
			m_pAugRotation->setValues(FVector3d(145.0, 0.0, 0.0));
			m_pAugScale->setValue(0.21);
		}
		emit modelChanged(4);
		emit mediaFileChanged("../../../Design/Models/Real Studio ECO/TableTest A/TableTest A 0000.tif");
		break;

	case 11: // ECO Studio/Table Zoom w/Anchor
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(15.03, -16.21, -10.90));
			m_pCamRotation->setValues(FVector3d(16.27, -8.65, -0.83));
			m_pCamFocalDist->setValue(18.4);
			m_pCamAperture->setValues(FVector2d(36.0, 20.35));
			m_pCamRadialDist->setValues(FVector2d(-15.0, 0.0));
			m_pAugPosition->setValues(FVector3d(-17.43, 4.26, -16.93));
			m_pAugRotation->setValues(FVector3d(145.0, 0.0, 0.0));
			m_pAugScale->setValue(0.1050);
			m_pEnableFocalDist->setState(true);
		}
		emit modelChanged(6);
		emit mediaFileChanged("../../../Design/Models/Real Studio ECO/TableTest B ECO/TableTest B ECO 0000.tif");
		break;

	case 12: // ECO Studio/Table
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(7.78, -13.56, 3.86));
			m_pCamRotation->setValues(FVector3d(3.22, -31.79, 0.42));
			m_pCamFocalDist->setValue(29.85);
			m_pCamAperture->setValues(FVector2d(36.0, 20.25));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(-10.2, -0.5, -31.6));
			m_pAugRotation->setValues(FVector3d(145.0, 0.0, 0.0));
			m_pAugScale->setValue(0.2);
		}
		emit modelChanged(4);
		emit mediaFileChanged("../../../Design/Models/Real Studio ECO/Table Test C/Table Test C 0000.tif");
		break;

	case 13: // ECO Studio/Table Zoom B
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(-2.87, -16.71, -17.90));
			m_pCamRotation->setValues(FVector3d(4.97, -40.75, -0.83));
			m_pCamFocalDist->setValue(80.0);
			m_pCamAperture->setValues(FVector2d(36.0, 20.25));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(-2.6, -0.2, -27.7));
			m_pAugRotation->setValues(FVector3d(145.0, 0.0, 0.0));
			m_pAugScale->setValue(0.2);
			m_pEnableFocalDist->setState(true);
		}
		emit modelChanged(4);
		emit mediaFileChanged("../../../Design/Models/Real Studio ECO/Table Zoom A/Table Zoom A 0000.tif");
		break;

	case 14: // ECO Virtual Studio
		if (!m_isReading)
		{
			m_pCamPosition->setValues(FVector3d(-127.5, -117.0, -37.0));
			m_pCamRotation->setValues(FVector3d(4.7, -18.6, -0.8));
			m_pCamFocalDist->setValue(27.0);
			m_pCamAperture->setValues(FVector2d(36.0, 20.25));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(0.0, 0.0, -460.0));
			m_pAugRotation->setValues(FVector3d(0.0, 0.0, 0.0));
			m_pAugScale->setValue(1.8);
		}
		emit modelChanged(7);
		emit mediaFileChanged("../../../Design/Models/Studio Model Eco/Rendered/ECO Studio 04 Move 1a/ECO 04 Move 1a0000.tif");
		break;

	case 15: // ECO Virtual Studio
		if (!m_isReading)
		{
			/*
			m_pCamPosition->setValues(FVector3d(-127.5, -117.0, -37.0));
			m_pCamRotation->setValues(FVector3d(4.7, -18.6, -0.8));
			m_pCamFocalDist->setValue(27.0);
			m_pCamAperture->setValues(FVector2d(36.0, 20.25));
			m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
			m_pAugPosition->setValues(FVector3d(0.0, 0.0, -460.0));
			m_pAugRotation->setValues(FVector3d(0.0, 0.0, 0.0));
			m_pAugScale->setValue(1.8);
			*/
		}
		emit modelChanged(9);
		emit mediaFileChanged("../../../Design/Sequences/Meteo Pan A/Meteo Pan A 0000.tif");
		break;
	}
}

void FParameterController::modelReset()
{
	selectModel(m_modelIndex);
}

void FParameterController::write(FArchive& ar)
{
	F_ASSERT(ar.isWriting());
	rootItem()->serialize(ar);
}

void FParameterController::read(FArchive& ar)
{
	F_ASSERT(ar.isReading());
	m_isReading = true,
	rootItem()->serialize(ar);
	m_isReading = false;
}

// Public queries -------------------------------------------------------------------------------------

// Internal functions ---------------------------------------------------------------------------------

void FParameterController::_initialize()
{
	FPTItemBase* pRootItem = new FPTItemBase(this, FTreeFormat::createDefaultFormat(0));
	setRootItem(pRootItem);

	FPTItemGroup* pGroupAugmentation = new FPTItemGroup("Augmentation", pRootItem, true);

	FPTItemGroup* pGroupAugModel = new FPTItemGroup("Model", pGroupAugmentation, true);

	FPTItemFileSelect* pAugModelFile = new FPTItemFileSelect("Model File", pGroupAugModel);
	connect(pAugModelFile, SIGNAL(textChanged(QString, int)), m_pEngine,
		SLOT(loadAugmentationModel(QString)));
	
	m_pAugPosition = new FPTItemVector3("Position", pGroupAugModel);
	m_pAugPosition->setValues(FVector3d(0.0, 0.0, 0.0));
	m_pAugPosition->setOptions(4, false, false);
	m_pAugPosition->setDragSpeed(0.1);
	connect(m_pAugPosition, SIGNAL(vectorValueChanged(FVector3d)), m_pEngine,
		SLOT(setAugmentedPosition(FVector3d)), Qt::DirectConnection);

	m_pAugRotation = new FPTItemVector3("Rotation", pGroupAugModel);
	m_pAugRotation->setValues(FVector3d(0.0, 0.0, 0.0));
	m_pAugRotation->setOptions(4, false, false);
	m_pAugRotation->setDragSpeed(0.1);
	connect(m_pAugRotation, SIGNAL(vectorValueChanged(FVector3d)), m_pEngine,
		SLOT(setAugmentedRotation(FVector3d)), Qt::DirectConnection);

	m_pAugScale = new FPTItemNumeric("Scale", QStringList(), pGroupAugModel);
	m_pAugScale->setValue(1.0);
	m_pAugScale->setOptions(4, false, false);
	m_pAugScale->setDragSpeed(0.01);
	connect(m_pAugScale, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setAugmentedScale(double)), Qt::DirectConnection);


	FPTItemGroup* pGroupLineTracker = new FPTItemGroup("Line Tracking", pRootItem, true);

	FPTItemGroup* pGroupLineModel = new FPTItemGroup("Model", pGroupLineTracker, true);

	FPTItemFileSelect* pLineModelFile = new FPTItemFileSelect("Model File", pGroupLineModel);
	connect(pLineModelFile, SIGNAL(textChanged(QString, int)), m_pEngine,
		SLOT(loadLineModel(QString)));

	//connect(this, SIGNAL(modelChanged(int)), m_pProcessor, SLOT(selectModel(int)));
	//connect(this, SIGNAL(mediaFileChanged(QString)), m_pProcessor, SLOT(openMediaFile(QString)));

	FPTItemGroup* pGroupCalibration = new FPTItemGroup("Calibration", pGroupLineTracker, true);

	m_pCamRadialDist = new FPTItemVector2("Radial Distortion", pGroupCalibration);
	m_pCamRadialDist->setOptions(3, false, false);
	m_pCamRadialDist->setDragSpeed(0.01);
	m_pCamRadialDist->setValues(FVector2d(0.0, 0.0));
	connect(m_pCamRadialDist, SIGNAL(vectorValueChanged(FVector2d)), m_pEngine,
		SLOT(setCameraRadialDistortion(FVector2d)), Qt::DirectConnection);

	m_pCamAperture = new FPTItemVector2("Aperture Size", pGroupCalibration);
	m_pCamAperture->setValues(FVector2d(36.0, 20.25));
	connect(m_pCamAperture, SIGNAL(vectorValueChanged(FVector2d)), m_pEngine,
		SLOT(setCameraAperture(FVector2d)), Qt::DirectConnection);

	m_pCamFocalDist = new FPTItemNumeric("Focal Length", QStringList(), pGroupCalibration);
	m_pCamFocalDist->setValue(35.0);
	connect(m_pCamFocalDist, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setCameraFocalDistance(double)), Qt::DirectConnection);

	FPTItemGroup* pGroupManual = new FPTItemGroup("Manual Control", pGroupLineTracker, true);

	FPTItemSwitch* pCamOverride = new FPTItemSwitch("Override Tracking", pGroupManual);
	connect(pCamOverride, SIGNAL(stateChanged(bool, int)), m_pEngine,
		SLOT(setCameraOverride(bool)), Qt::DirectConnection);

	m_pCamPosition = new FPTItemVector3("Position", pGroupManual);
	m_pCamPosition->setValues(FVector3d(0.0, 0.0, -225.0));
	connect(m_pCamPosition, SIGNAL(vectorValueChanged(FVector3d)), m_pEngine,
		SLOT(setCameraTranslation(FVector3d)), Qt::DirectConnection);

	m_pCamRotation = new FPTItemVector3("Rotation", pGroupManual);
	m_pCamRotation->setValues(FVector3d(0.0, 27.0, 0.0));
	connect(m_pCamRotation, SIGNAL(vectorValueChanged(FVector3d)), m_pEngine,
		SLOT(setCameraRotation(FVector3d)), Qt::DirectConnection);

	FPTItemGroup* pGroupSampling = new FPTItemGroup("Edge Sampling", pGroupLineTracker, true);

	FPTItemNumeric* pSearchRange = new FPTItemNumeric("Search Range", QStringList(), pGroupSampling);
	pSearchRange->setBounds(0.0, 100.0);
	pSearchRange->setOptions(1, true, false);
	pSearchRange->setDragSpeed(0.5);
	pSearchRange->setValue(25.0);
	connect(pSearchRange, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setSearchRange(double)), Qt::DirectConnection);

	FPTItemNumeric* pSampleDist = new FPTItemNumeric("Sampling Distance", QStringList(), pGroupSampling);
	pSampleDist->setBounds(0.1, 50.0);
	pSampleDist->setOptions(1, true, false);
	pSampleDist->setDragSpeed(0.1);
	pSampleDist->setValue(10.0);
	connect(pSampleDist, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setSamplingDistance(double)), Qt::DirectConnection);

	FPTItemNumeric* pSampleDens = new FPTItemNumeric("Adaptive Density", QStringList(), pGroupSampling);
	pSampleDens->setBounds(0.0, 5.0);
	pSampleDens->setOptions(1, true, false);
	pSampleDens->setDragSpeed(0.1);
	pSampleDens->setValue(0.0);
	connect(pSampleDens, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setSamplingAdaptiveDensity(double)), Qt::DirectConnection);

	FPTItemNumeric* pMotionComp = new FPTItemNumeric("Motion Compensation", QStringList(), pGroupSampling);
	pMotionComp->setBounds(0.0, 4.0);
	pMotionComp->setOptions(1, true, false);
	pMotionComp->setDragSpeed(0.1);
	pMotionComp->setValue(0.0);
	connect(pMotionComp, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setMotionCompensation(double)), Qt::DirectConnection);

	FPTItemNumeric* pSurfaceAngleLimit = new FPTItemNumeric("Surface Angle Limit", QStringList(), pGroupSampling);
	pSurfaceAngleLimit->setBounds(0.0, 10.0);
	pSurfaceAngleLimit->setOptions(1, true, false);
	pSurfaceAngleLimit->setDragSpeed(0.01);
	pSurfaceAngleLimit->setValue(3.0);
	connect(pSurfaceAngleLimit, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setTrackerSurfaceAngleLimit(double)), Qt::DirectConnection);

	FPTItemNumeric* pContourAngleLimit = new FPTItemNumeric("Contour Angle Limit", QStringList(), pGroupSampling);
	pContourAngleLimit->setBounds(0.0, 10.0);
	pContourAngleLimit->setOptions(1, true, false);
	pContourAngleLimit->setDragSpeed(0.01);
	pContourAngleLimit->setValue(1.0);
	connect(pContourAngleLimit, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setTrackerContourAngleLimit(double)), Qt::DirectConnection);
	

	FPTItemGroup* pGroupEdgeDetect = new FPTItemGroup("Edge Detection", pGroupLineTracker, true);

	FPTItemNumeric* pEdgeLuma = new FPTItemNumeric("Luma Weight", QStringList(), pGroupEdgeDetect);
	pEdgeLuma->setBounds(0.0, 2.0);
	pEdgeLuma->setOptions(3, true, false);
	pEdgeLuma->setDragSpeed(0.01);
	pEdgeLuma->setValue(1.0);
	connect(pEdgeLuma, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setEdgeLumaWeight(double)), Qt::DirectConnection);

	FPTItemNumeric* pEdgeChroma = new FPTItemNumeric("Chroma Weight", QStringList(), pGroupEdgeDetect);
	pEdgeChroma->setBounds(0.0, 2.0);
	pEdgeChroma->setOptions(3, true, false);
	pEdgeChroma->setDragSpeed(0.01);
	pEdgeChroma->setValue(1.0);
	connect(pEdgeChroma, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setEdgeChromaWeight(double)), Qt::DirectConnection);

	FPTItemNumeric* pEdgeThreshold = new FPTItemNumeric("Threshold", QStringList(), pGroupEdgeDetect);
	pEdgeThreshold->setBounds(0.0, 2.0);
	pEdgeThreshold->setOptions(3, true, false);
	pEdgeThreshold->setDragSpeed(0.001);
	pEdgeThreshold->setValue(0.5);
	connect(pEdgeThreshold, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setEdgeThreshold(double)), Qt::DirectConnection);

	
	FPTItemGroup* pGroupColorTolerance = new FPTItemGroup("Color Matching", pGroupLineTracker, true);

	FPTItemSwitch* pColorToleranceEnabled = new FPTItemSwitch("Enable", QStringList(), pGroupColorTolerance);
	pColorToleranceEnabled->setState(true);
	connect(pColorToleranceEnabled, SIGNAL(stateChanged(bool, int)), m_pEngine,
		SLOT(setColorToleranceEnabled(bool)), Qt::DirectConnection);

	FPTItemNumeric* pColorTol = new FPTItemNumeric("Tolerance", QStringList(), pGroupColorTolerance);
	pColorTol->setBounds(0.0, 1.0);
	pColorTol->setOptions(2, true, false);
	pColorTol->setDragSpeed(0.01);
	pColorTol->setValue(0.15);
	connect(pColorTol, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setColorTolerance(double)), Qt::DirectConnection);

	FPTItemNumeric* pColorAdapt = new FPTItemNumeric("Adaptability", QStringList(), pGroupColorTolerance);
	pColorAdapt->setBounds(0.0, 1.0);
	pColorAdapt->setOptions(2, true, false);
	pColorAdapt->setDragSpeed(0.01);
	pColorAdapt->setValue(0.5);
	connect(pColorAdapt, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setColorAdaptability(double)), Qt::DirectConnection);

	FPTItemNumeric* pColorPeekDistance = new FPTItemNumeric("Peek Distance", QStringList(), pGroupColorTolerance);
	pColorPeekDistance->setBounds(0.5, 10.0);
	pColorPeekDistance->setOptions(2, true, false);
	pColorPeekDistance->setDragSpeed(0.01);
	pColorPeekDistance->setValue(3.0);
	connect(pColorPeekDistance, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setColorPeekDistance(double)), Qt::DirectConnection);

	FPTItemNumeric* pColorFullEdge = new FPTItemNumeric("Full Edge Threshold", QStringList(), pGroupColorTolerance);
	pColorFullEdge->setBounds(0.0, 10.0);
	pColorFullEdge->setOptions(2, true, false);
	pColorFullEdge->setDragSpeed(0.01);
	pColorFullEdge->setValue(3.0);
	connect(pColorFullEdge, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setColorFullEdgeThreshold(double)), Qt::DirectConnection);

	FPTItemNumeric* pColorHalfEdge = new FPTItemNumeric("Half Edge Threshold", QStringList(), pGroupColorTolerance);
	pColorHalfEdge->setBounds(0.0, 10.0);
	pColorHalfEdge->setOptions(2, true, false);
	pColorHalfEdge->setDragSpeed(0.01);
	pColorHalfEdge->setValue(1.5);
	connect(pColorHalfEdge, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setColorHalfEdgeThreshold(double)), Qt::DirectConnection);


	FPTItemGroup* pGroupOptimization = new FPTItemGroup("Optimization", pGroupLineTracker, true);

	m_pEnableFocalDist = new FPTItemSwitch("Estimate Focal Length", pGroupOptimization);
	connect(m_pEnableFocalDist, SIGNAL(stateChanged(bool, int)), m_pEngine,
		SLOT(setFocalDistanceEnabled(bool)), Qt::DirectConnection);

	FPTItemNumeric* pInitThreshold = new FPTItemNumeric("Initialization Threshold", QStringList(), pGroupOptimization);
	pInitThreshold->setBounds(0.0, 10.0);
	pInitThreshold->setOptions(2, true, false);
	pInitThreshold->setDragSpeed(0.01);
	pInitThreshold->setValue(1.5);
	connect(pInitThreshold, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setTrackerInitializationThreshold(double)), Qt::DirectConnection);

	FPTItemNumeric* pFailureThreshold = new FPTItemNumeric("Failure Threshold", QStringList(), pGroupOptimization);
	pFailureThreshold->setBounds(0.0, 10.0);
	pFailureThreshold->setOptions(2, true, false);
	pFailureThreshold->setDragSpeed(0.01);
	pFailureThreshold->setValue(3.5);
	connect(pFailureThreshold, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setTrackerFailureThreshold(double)), Qt::DirectConnection);


	FPTItemGroup* pGroupMoreOpt = new FPTItemGroup("More Optimization", pGroupLineTracker, false);

	FPTItemSwitch* pMultiHyp = new FPTItemSwitch("Multiple Hypotheses", pGroupMoreOpt);
	pMultiHyp->setState(true);
	connect(pMultiHyp, SIGNAL(stateChanged(bool, int)), m_pEngine,
		SLOT(setMultipleHypothesesEnabled(bool)), Qt::DirectConnection);

	FPTItemNumeric* pPredictionFactor = new FPTItemNumeric("Motion Prediction", QStringList(), pGroupMoreOpt);
	pPredictionFactor->setBounds(0.0, 1.0);
	pPredictionFactor->setOptions(2, true, false);
	pPredictionFactor->setDragSpeed(0.01);
	pPredictionFactor->setValue(0.9);
	connect(pPredictionFactor, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setTrackerPredictionFactor(double)), Qt::DirectConnection);

	FPTItemVector2* pInterpolationRate = new FPTItemVector2("Interpolation Range", pGroupMoreOpt);
	pInterpolationRate->setValues(FVector2d(2.0, 6.0));
	pInterpolationRate->setOptions(2, false, false);
	pInterpolationRate->setDragSpeed(0.01);
	connect(pInterpolationRate, SIGNAL(vectorValueChanged(FVector2d)), m_pEngine,
		SLOT(setTrackerInterpolationRate(FVector2d)), Qt::DirectConnection);

	QStringList estimatorOpts;
	estimatorOpts << "Squared" << "Huber" << "Tukey";
	FPTItemOption* pOptEstimator = new FPTItemOption("Estimator Type", QStringList(), pGroupMoreOpt);
	pOptEstimator->setOptions(estimatorOpts);
	pOptEstimator->selectOption(0);
	connect(pOptEstimator, SIGNAL(optionChanged(int, int)), m_pEngine, SLOT(setTrackerEstimatorType(int)));

	FPTItemNumeric* pEstimatorLimit = new FPTItemNumeric("Estimator Limit", QStringList(), pGroupMoreOpt);
	pEstimatorLimit->setBounds(0.0, 100.0);
	pEstimatorLimit->setOptions(2, false, false);
	pEstimatorLimit->setDragSpeed(0.01);
	pEstimatorLimit->setValue(8.0);
	connect(pEstimatorLimit, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setTrackerEstimatorLimit(double)), Qt::DirectConnection);

	FPTItemNumeric* pRejectionLimitA = new FPTItemNumeric("Rejection Factor A", QStringList(), pGroupMoreOpt);
	pRejectionLimitA->setBounds(0.0, 10.0);
	pRejectionLimitA->setOptions(2, true, false);
	pRejectionLimitA->setDragSpeed(0.01);
	pRejectionLimitA->setValue(3.0);
	connect(pRejectionLimitA, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setTrackerRejectionFactorA(double)), Qt::DirectConnection);

	FPTItemNumeric* pRejectionLimitB = new FPTItemNumeric("Rejection Factor B", QStringList(), pGroupMoreOpt);
	pRejectionLimitB->setBounds(0.0, 10.0);
	pRejectionLimitB->setOptions(2, true, false);
	pRejectionLimitB->setDragSpeed(0.01);
	pRejectionLimitB->setValue(1.0);
	connect(pRejectionLimitB, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setTrackerRejectionFactorB(double)), Qt::DirectConnection);

	FPTItemGroup* pGroupDetection = new FPTItemGroup("Contour Detection", pRootItem, true);

	FPTItemGroup* pGroupGeneral = new FPTItemGroup("General", pGroupDetection, true);

	FPTItemSwitch* pDetectionEnabled = new FPTItemSwitch("Enable Detection", pGroupGeneral);
	pDetectionEnabled->setState(true);
	connect(pDetectionEnabled, SIGNAL(stateChanged(bool, int)), m_pEngine,
		SLOT(setDetectionEnabled(bool)), Qt::DirectConnection);

	FPTItemSwitch* pDetectionAlways = new FPTItemSwitch("Always On", pGroupGeneral);
	connect(pDetectionAlways, SIGNAL(stateChanged(bool, int)), m_pEngine,
		SLOT(setDetectionAlwaysOn(bool)), Qt::DirectConnection);

	m_pClassifierDataFile = new FPTItemFileSelect("Database File", pGroupGeneral);
	connect(m_pClassifierDataFile, SIGNAL(textChanged(QString, int)), m_pEngine,
		SLOT(loadClassifierDatabase(QString)));

	FPTItemGroup* pGroupCanny = new FPTItemGroup("Canny Edges", pGroupDetection, true);

	FPTItemNumeric* pEdgeThresholdLow = new FPTItemNumeric("Low Threshold", QStringList(), pGroupCanny);
	pEdgeThresholdLow->setBounds(0.0, 1.0);
	pEdgeThresholdLow->setOptions(3, false, false);
	pEdgeThresholdLow->setDragSpeed(0.001);
	pEdgeThresholdLow->setValue(0.02);
	connect(pEdgeThresholdLow, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setEdgeThresholdLow(double)), Qt::DirectConnection);

	FPTItemNumeric* pEdgeThresholdHigh = new FPTItemNumeric("High Threshold", QStringList(), pGroupCanny);
	pEdgeThresholdHigh->setBounds(0.0, 1.0);
	pEdgeThresholdHigh->setOptions(3, false, false);
	pEdgeThresholdHigh->setDragSpeed(0.001);
	pEdgeThresholdHigh->setValue(0.07);
	connect(pEdgeThresholdHigh, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setEdgeThresholdHigh(double)), Qt::DirectConnection);
	

	FPTItemGroup* pGroupPoseDetection = new FPTItemGroup("Template Fitting", pGroupDetection, true);


	FPTItemNumeric* pMSEThres = new FPTItemNumeric("MSE Threshold", QStringList(), pGroupPoseDetection);
	pMSEThres->setValue(3.0);
	pMSEThres->setOptions(2, true, false);
	pMSEThres->setBounds(0.5f, 16.0f);
	pMSEThres->setDragSpeed(0.02f);
	connect(pMSEThres, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setDetectionMSEThreshold(double)), Qt::DirectConnection);

	FPTItemOption* pFixedTypeId = new FPTItemOption("Fixed Contour", QStringList(), pGroupPoseDetection);
	QStringList options3;
	options3 << "Off" << "Type #0" << "Type #1" << "Type #2" << "Type #3" << "Type #4"
		<< "Type #5" << "Type #6" << "Type #7";
	pFixedTypeId->setOptions(options3);
	connect(pFixedTypeId, SIGNAL(optionChanged(int, int)), m_pEngine,
		SLOT(setDetectionFixedTypeId(int)), Qt::DirectConnection);


	FPTItemGroup* pGroupContourModel = new FPTItemGroup("Contour Model Pose", pGroupDetection, true);


	FPTItemVector3* pContourPosition = new FPTItemVector3("Position", pGroupContourModel);
	pContourPosition->setValues(FVector3d(0.0, 0.0, 0.0));
	pContourPosition->setOptions(4, false, false);
	pContourPosition->setDragSpeed(0.1);
	connect(pContourPosition, SIGNAL(vectorValueChanged(FVector3d)), m_pEngine,
		SLOT(setDetectionContourPosition(FVector3d)), Qt::DirectConnection);

	FPTItemVector3* pContourRotation = new FPTItemVector3("Rotation", pGroupContourModel);
	pContourRotation->setValues(FVector3d(0.0, 0.0, 0.0));
	pContourRotation->setOptions(4, false, false);
	pContourRotation->setDragSpeed(0.1);
	connect(pContourRotation, SIGNAL(vectorValueChanged(FVector3d)), m_pEngine,
		SLOT(setDetectionContourRotation(FVector3d)), Qt::DirectConnection);

	FPTItemNumeric* pContourScale = new FPTItemNumeric("Scale", QStringList(), pGroupContourModel);
	pContourScale->setValue(1.0);
	pContourScale->setOptions(4, false, false);
	pContourScale->setDragSpeed(0.01);
	connect(pContourScale, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setDetectionContourScale(double)), Qt::DirectConnection);


	m_initialized = true;
}

// ----------------------------------------------------------------------------------------------------