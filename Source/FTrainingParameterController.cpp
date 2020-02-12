// ----------------------------------------------------------------------------------------------------
//  Title			FTrainingParameterController.cpp
//  Description		Implementation of class FTrainingParameterController
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-04 18:33:19 +0200 (So, 04 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "FTrainingEngine.h"
#include "FPatchSizePreset.h"
#include "FTrainingParameterController.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingParameterController
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FTrainingParameterController::FTrainingParameterController(QWidget* pParent /* = NULL */)
: FTreeWidget(pParent),
  m_pEngine(NULL),
  m_isInitialized(false)
{
}

FTrainingParameterController::~FTrainingParameterController()
{
}

// Public commands ------------------------------------------------------------------------------------

void FTrainingParameterController::setEngine(FTrainingEngine* pEngine)
{
	F_ASSERT(pEngine && !m_isInitialized);

	m_pEngine = pEngine;
	_initialize();
	m_isInitialized = true;
}

// Internal functions ---------------------------------------------------------------------------------

void FTrainingParameterController::_initialize()
{
	FPTItemBase* pRootItem = new FPTItemBase(this, FTreeFormat::createDefaultFormat(0));
	setRootItem(pRootItem);

	FPTItemGroup* pGroupPoseRange = new FPTItemGroup("Camera Pose", pRootItem, true);

	FPTItemGroup* pGroupPoseMin = new FPTItemGroup("Lower Bounds", pGroupPoseRange, true);

	FPTItemVector3* pMinPosition = new FPTItemVector3("Position", pGroupPoseMin);
	pMinPosition->setValues(m_pEngine->parameters().posMin);
	pMinPosition->setOptions(4, false, false);
	pMinPosition->setDragSpeed(0.1);
	connect(pMinPosition, SIGNAL(vectorValueChanged(FVector3d)), m_pEngine,
		SLOT(setParamCamPositionMin(FVector3d)), Qt::DirectConnection);

	FPTItemVector3* pMinRotation = new FPTItemVector3("Rotation", pGroupPoseMin);
	pMinRotation->setValues(m_pEngine->parameters().yprMin);
	pMinRotation->setOptions(4, false, false);
	pMinRotation->setDragSpeed(0.1);
	connect(pMinRotation, SIGNAL(vectorValueChanged(FVector3d)), m_pEngine,
		SLOT(setParamCamOrbitMin(FVector3d)), Qt::DirectConnection);

	FPTItemGroup* pGroupPoseMax = new FPTItemGroup("Upper Bounds", pGroupPoseRange, true);

	FPTItemVector3* pMaxPosition = new FPTItemVector3("Position", pGroupPoseMax);
	pMaxPosition->setValues(m_pEngine->parameters().posMax);
	pMaxPosition->setOptions(4, false, false);
	pMaxPosition->setDragSpeed(0.1);
	connect(pMaxPosition, SIGNAL(vectorValueChanged(FVector3d)), m_pEngine,
		SLOT(setParamCamPositionMax(FVector3d)), Qt::DirectConnection);

	FPTItemVector3* pMaxRotation = new FPTItemVector3("Rotation", pGroupPoseMax);
	pMaxRotation->setValues(m_pEngine->parameters().yprMax);
	pMaxRotation->setOptions(4, false, false);
	pMaxRotation->setDragSpeed(0.1);
	connect(pMaxRotation, SIGNAL(vectorValueChanged(FVector3d)), m_pEngine,
		SLOT(setParamCamOrbitMax(FVector3d)), Qt::DirectConnection);

	FPTItemGroup* pGroupPoseFix = new FPTItemGroup("Constants", pGroupPoseRange, true);

	FPTItemNumeric* pFocalLength = new FPTItemNumeric("Focal Length", QStringList(), pGroupPoseFix);
	pFocalLength->setValue(m_pEngine->parameters().focalLength);
	connect(pFocalLength, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setParamCamFocalLength(double)), Qt::DirectConnection);

	FPTItemSwitch* pRotOrder = new FPTItemSwitch("Inverse Axis Order", pGroupPoseFix);
	pRotOrder->setState(m_pEngine->parameters().inverseRotationOrder);
	connect(pRotOrder, SIGNAL(stateChanged(bool, int)), m_pEngine,
		SLOT(setParamInverseRotationOrder(bool)), Qt::DirectConnection);

	FPTItemGroup* pGroupTraining = new FPTItemGroup("Training", pRootItem, true);

	FPTItemGroup* pGroupDatabase = new FPTItemGroup("Classifier", pGroupTraining, true);

	QStringList sizeOpts;
	for (size_t i = 0; i < FPatchSizePreset::size(); ++i)
		sizeOpts << FPatchSizePreset(i).toString();

	FPTItemOption* pPatchSize = new FPTItemOption("Patch Size", QStringList(), pGroupDatabase);
	pPatchSize->setOptions(sizeOpts);
	pPatchSize->selectOption((int)FPatchSizePreset::PatchSize_64x64);
	connect(pPatchSize, SIGNAL(optionChanged(int, int)), m_pEngine, SLOT(setParamPatchSize(int)));

	FPTItemOption* pTemplSize = new FPTItemOption("Template Size", QStringList(), pGroupDatabase);
	pTemplSize->setOptions(sizeOpts);
	pTemplSize->selectOption((int)FPatchSizePreset::PatchSize_128x128);
	connect(pTemplSize, SIGNAL(optionChanged(int, int)), m_pEngine, SLOT(setParamTemplateSize(int)));

	FPTItemNumeric* pErrorThres = new FPTItemNumeric("Fitting Threshold", QStringList(), pGroupDatabase);
	pErrorThres->setValue(2.0);
	pErrorThres->setOptions(2, true, false);
	pErrorThres->setBounds(0.5f, 16.0f);
	pErrorThres->setDragSpeed(0.02f);
	connect(pErrorThres, SIGNAL(scalarValueChanged(double, int)), m_pEngine,
		SLOT(setParamWarpErrorThreshold(double)), Qt::DirectConnection);
}

// ----------------------------------------------------------------------------------------------------