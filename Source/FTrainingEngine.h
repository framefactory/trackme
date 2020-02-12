// ----------------------------------------------------------------------------------------------------
//  Title			FTrainingEngine.h
//  Description		Header file for FTrainingEngine.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-25 21:02:54 +0200 (Do, 25 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FTRAININGENGINE_H
#define FTRAININGENGINE_H

#include <QObject>
#include "FTrackMe.h"
#include "FlowGL.h"
#include "FCameraPose.h"
#include "FCameraMetrics.h"
#include "FRandom.h"
#include "FTrainingParameter.h"
#include "FContourPatch.h"
#include "FTrainingStatistics.h"
#include "FArchive.h"

class FContourModel;
class FDistanceTransform;
class FContourFinder;
class FContourDatabase;
class FContourClass;
class FPoseDetector;

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingEngine
// ----------------------------------------------------------------------------------------------------

class FTrainingEngine : public QObject
{
	Q_OBJECT;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FTrainingEngine(QWidget* pView, QObject* pParent = NULL);
	/// Virtual destructor.
	virtual ~FTrainingEngine();

	//  Public commands --------------------------------------------------------

public:
	/// Clears all training data and prepares for a new training session.
	void clear();
	/// Clears all training data and loads the given contour model.
	void loadContourModel(const QString& modelFilePath);
	
	/// Loads a previously created and saved training data.
	bool loadClassifierData(const QString& dataFilePath);
	/// Saves the current training data.
	bool saveClassifierData(const QString& dataFilePath);

	/// Single run of the training engine. Draws a random pose and
	/// incrementally trains the classifier.
	void runOnce();
	/// Single run of pose verifying test. Draws a random pose and
	/// tests the estimation quality of the database.
	void testOnce(QTextStream& protocol);

	/// Sets the parameters to be used for training.
	void setParameters(const FTrainingParameter& params);

	/// Redraws the OpenGL view area.
	void updateView();

	//  Public queries ---------------------------------------------------------

	/// Returns the current training parameters.
	const FTrainingParameter& parameters() const { return m_params; }
	/// Returns the statistics of the last run.
	const FTrainingStatistics* statistics() const { return m_pStatistics; }
	/// Returns an info dump about the database.
	QString databaseInfo() const;

	/// Returns the size of the training canvas.
	const QSize& canvasSize() const { return m_trainingCanvasSize; }
	/// Returns the size of a template.
	const QSize& templateSize() const { return m_templateSize; }
	/// Returns the size of view.
	const QSize& viewSize() const { return m_viewSize; }

	//  Public slots -----------------------------------------------------------

public slots:
	void setParamCamPositionMin(FVector3d val);
	void setParamCamPositionMax(FVector3d val);
	void setParamCamOrbitMin(FVector3d val);
	void setParamCamOrbitMax(FVector3d val);

	void setParamCamFocalLength(double val);
	void setParamInverseRotationOrder(bool val);

	void setParamTemplateSize(int index);
	void setParamPatchSize(int index);
	void setParamWarpErrorThreshold(double val);

	//  Signals ----------------------------------------------------------------

signals:
	void postMessage(QString text);
	void postStatistics(FTrainingStatistics statistics);
	void postDatabaseInfo(QString info);

	//  Internal functions -----------------------------------------------------

private:
	void _generatePose(FCameraPose& pose);
	void _drawContourModel(const FCameraPose& pose);
	void _processContours(const FCameraPose& pose);
	void _viewPose(int poseType);

	void _initGL();

	//  Internal data members --------------------------------------------------

private:
	QSize m_trainingCanvasSize;
	QSize m_templateSize;
	QSize m_patchSize;
	QSize m_viewSize;
	QWidget* m_pView;

	FRandom m_randGen;
	const FContourModel* m_pModel;
	FTrainingParameter m_params;
	FCameraMetrics m_cameraMetrics;
	FContourDatabase* m_pDatabase;
	quint32 m_trainingRun;
	float m_warpErrorThreshold;

	FDistanceTransform* m_pDistanceTransform;
	FContourFinder* m_pContourFinder;

	// Last used patch and class for viewing only
	FContourPatch m_patch[FGlobalConstants::MAX_TEMPLATES];

	static const size_t MAX_DISPLAY_SLOTS = 8;
	const FContourClass* m_pSlotClass[FGlobalConstants::MAX_TEMPLATES];
	const FContour* m_pSlotContour[MAX_DISPLAY_SLOTS];
	FMatrix3f m_slotHomography[MAX_DISPLAY_SLOTS];

	FTrainingStatistics* m_pStatistics;

	// Quality measurement
	FPoseDetector* m_pPoseDetector;
	FDTPixel* m_pDTImage;

	// OpenGL

	FGLContext m_glContext;

	FGLOverlayRect m_overlayCanvas;

	FGLBuffer m_bufTransform;

	FGLProgram m_prgContourDraw;
	int m_uTransformBlock;
	FGLFramebuffer m_fbContour;
	FGLTextureRect m_texContour;

	FGLProgram m_prgContourView;
	FGLProgram m_prgDistanceTransformView;
	FGLProgram m_prgDTMapView;
	FGLOverlayRect m_overlayViewLarge[2];
	FGLOverlayRect m_overlayViewSmall[16];
	FGLTextureRect m_texTemplateView[16];
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FTRAININGENGINE_H