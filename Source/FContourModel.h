// ----------------------------------------------------------------------------------------------------
//  Title			FContourModel.h
//  Description		Header file for FContourModel.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-25 21:02:54 +0200 (Do, 25 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FCONTOURMODEL_H
#define FCONTOURMODEL_H

#include "FTrackMe.h"
#include "FVectorT.h"
#include "FBoxT.h"
#include "FArchive.h"
#include "FlowGL.h"

// ----------------------------------------------------------------------------------------------------
//  Class FContourModel
// ----------------------------------------------------------------------------------------------------

/// Imported line model for DT contour classifier training.
/// Supports OpenGL drawing of line contours.
class  FContourModel
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FContourModel();
	/// Virtual destructor.
	virtual ~FContourModel();

	//  Public commands --------------------------------------------------------

public:
	bool import(const QString& filePath);
	
	void draw() const;
	void draw(size_t contourIndex) const;

	/// Serialization.
	void serialize(FArchive& ar);

	//  Public queries ---------------------------------------------------------

	/// Returns true if the model has been loaded and initialized properly.
	bool isValid() const { return m_vertexArray.isValid(); }

	/// Returns the file path of the contour model.
	const QString& filePath() const { return m_filePath; }

	/// Returns the number of extracted contours.
	size_t contourCount() const { return m_contours.size(); }

	/// Returns the center of the contour with the given index.
	const FVector3f& contourCenter(size_t index) const {
		F_ASSERT(index < m_contours.size());
		return m_contours[index].center;
	}
	/// Returns the bounding box of the entire model.
	const FBox3f& boundingBox() const { return m_boundingBox; }

	/// Returns the name of the contour with the given index.
	const QString& contourName(size_t index) const {
		F_ASSERT(index < m_contours.size());
		return m_contours[index].name;
	}

	/// Writes information about the internal state to the given debug object.
	void dump(QDebug& debug) const;

	//  Internal functions -----------------------------------------------------

private:
	void _createGL();
	void _releaseGL();
	bool _importModel(const QString& filePath);
	void _getEdgesFromNode(const aiScene* pScene, const aiNode* pNode, FMatrix4f trafo);
	void _calculateBoundingBox();
	
	//  Internal data members --------------------------------------------------

private:
	struct vertex_t
	{
		FVector3f pos;
		float index;
	};

	struct line_t
	{
		line_t() { }

		line_t(const FVector3f& p0, const FVector3f& p1, float index) {
			v0.pos = p0; v0.index = index;
			v1.pos = p1; v1.index = index;
		}

		vertex_t v0;
		vertex_t v1;
	};

	struct contour_t
	{
		size_t firstVertex;
		size_t vertexCount;
		FVector3f center;
		QString name;
	};

	typedef std::vector<line_t> lineVec_t;
	typedef std::vector<contour_t> contourVec_t;
	lineVec_t m_lines;
	contourVec_t m_contours;
	QString m_filePath;
	FBox3f m_boundingBox;

	mutable FGLBuffer m_vertexBuffer;
	mutable FGLVertexArray m_vertexArray;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FCONTOURMODEL_H