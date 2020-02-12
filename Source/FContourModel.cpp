// ----------------------------------------------------------------------------------------------------
//  Title			FContourModel.cpp
//  Description		Implementation of class FContourModel
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-25 21:02:54 +0200 (Do, 25 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include <QMessageBox>

#include <assimp.hpp>
#include <aiScene.h>
#include <aiPostProcess.h>

#include "FContourModel.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FContourModel
// ----------------------------------------------------------------------------------------------------

#define CMP_EPS 0.01f

inline bool aiCmp(const aiVector3D& v0, const aiVector3D& v1)
{
	return (v0.x < v1.x + CMP_EPS && v0.x > v1.x - CMP_EPS
		&& v0.y < v1.y + CMP_EPS && v0.y > v1.y - CMP_EPS
		&& v0.z < v1.z + CMP_EPS && v0.z > v1.z - CMP_EPS);
}

// Constructors and destructor ------------------------------------------------------------------------

FContourModel::FContourModel()
{
}

FContourModel::~FContourModel()
{
}

// Public commands ------------------------------------------------------------------------------------

bool FContourModel::import(const QString& filePath)
{
	if (!_importModel(filePath))
	{
		_releaseGL();
		return false;
	}

	m_filePath = filePath;
	_createGL();
	return true;
}

void FContourModel::draw() const
{
	F_ASSERT(isValid());
	if (!isValid())
		return;

	m_vertexArray.draw(FGLPrimitiveType::Lines, 0, m_lines.size() * 2);
	F_GLERROR_ASSERT;
}

void FContourModel::draw(size_t contourIndex) const
{
	F_ASSERT(contourIndex < m_contours.size());
	F_ASSERT(isValid());
	if (!isValid())
		return;

	m_vertexArray.draw(FGLPrimitiveType::Lines,
		m_contours[contourIndex].firstVertex, m_contours[contourIndex].vertexCount);
	F_GLERROR_ASSERT;
}

void FContourModel::serialize(FArchive& ar)
{
	if (ar.isReading())
	{
		size_t n;
		ar >> n;
		m_lines.resize(n);
		for (size_t i = 0; i < n; ++i)
		{
			ar >> m_lines[i].v0.pos;
			ar >> m_lines[i].v0.index;
			ar >> m_lines[i].v1.pos;
			ar >> m_lines[i].v1.index;
		}

		ar >> n;
		m_contours.resize(n);
		for (size_t i = 0; i < n; ++i)
		{
			ar >> m_contours[i].firstVertex;
			ar >> m_contours[i].vertexCount;
			ar >> m_contours[i].center;
			ar >> m_contours[i].name;
		}

		ar >> m_filePath;
		_calculateBoundingBox();
		_createGL();
	}
	else
	{
		ar << m_lines.size();
		for (size_t i = 0; i < m_lines.size(); ++i)
		{
			ar << m_lines[i].v0.pos;
			ar << m_lines[i].v0.index;
			ar << m_lines[i].v1.pos;
			ar << m_lines[i].v1.index;
		}

		ar << m_contours.size();
		for (size_t i = 0; i < m_contours.size(); ++i)
		{
			ar << m_contours[i].firstVertex;
			ar << m_contours[i].vertexCount;
			ar << m_contours[i].center;
			ar << m_contours[i].name;
		}

		ar << m_filePath;
	}
}

// Public queries -------------------------------------------------------------------------------------

void FContourModel::dump(QDebug& debug) const
{
	debug << "\n--- FContourModel ---";
	debug << "\n   Bounding box:  " << m_boundingBox.toString();
	debug << "\n   Contour count: " << m_contours.size();
	for (size_t i = 0; i < m_contours.size(); i++)
	{
		debug << "\n      Contour #" << i << " (" << m_contours[i].name
			<< "): " << m_contours[i].vertexCount << " vertices, center: "
			<< m_contours[i].center.toString();
	}
}

// Internal functions ---------------------------------------------------------------------------------

void FContourModel::_createGL()
{
	size_t numVertices = m_lines.size() * 2;
	size_t numBytes = m_lines.size() * sizeof(line_t);

	m_vertexBuffer.createInitialize(&m_lines.front(), numBytes, FGLUsage::StaticDraw);

	m_vertexArray.create();
	m_vertexArray.bindVertexBuffer(m_vertexBuffer);
	m_vertexArray.setVertexAttribute(0, 3, FGLDataType::Float, false, 4 * sizeof(float), (void*)0);
	m_vertexArray.setVertexAttribute(1, 1, FGLDataType::Float, false, 4 * sizeof(float), (void*)(3 * sizeof(float)));
	
	F_GLERROR_ASSERT;
}

void FContourModel::_releaseGL()
{
	m_vertexArray.release();
	m_vertexBuffer.release();
}

bool FContourModel::_importModel(const QString& filePath)
{
	F_TRACE(QString("FContourModel::_importModel - Importing %1").arg(filePath));
	QByteArray fnAscii = filePath.toAscii();

	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT);
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_COLORS /*| aiComponent_MATERIALS */);
	quint32 flags = aiProcess_JoinIdenticalVertices | aiProcess_RemoveComponent
		| aiProcess_PreTransformVertices;

	const aiScene* pScene = importer.ReadFile(fnAscii, flags);
	if (pScene == NULL)
		return false;

	// check if model is on the xy-plane (all z-coordinates must be zero)
	bool pass = true;
	float maxAbsZ = 0.0f;
	for (quint32 m = 0; m < pScene->mNumMeshes; m++)
	{
		aiMesh* pMesh = pScene->mMeshes[m];
		for (quint32 v = 0; v < pMesh->mNumVertices; v++)
		{
			aiVector3D& vertex = pMesh->mVertices[v];
			pass = pass && (vertex.z > -CMP_EPS && vertex.z < CMP_EPS);
			if (!pass)
				maxAbsZ = fMax(maxAbsZ, fabsf(vertex.z));

			vertex.z = 0.0f;
		}
	}

	if (!pass)
	{
		QMessageBox::warning(NULL, "Contour Model Import",
			QString("Warning: Model is not planar in xy-plane (max z-value: %1).\r\n"
			"All z-values will be forced to zero which may lead to unexpected results.").arg(maxAbsZ));
	}
	else
		F_TRACE("FContourModel::_importModel - xy-plane test passed");


	m_lines.clear();
	FMatrix4f trafo;
	trafo.makeIdentity();

	_getEdgesFromNode(pScene, pScene->mRootNode, trafo);
	if (m_lines.empty())
		return false;

	F_CONSOLE("FContourModel::_importModel - Imported " << m_contours.size()
		<< " contours with " << m_lines.size() << " edge lines");

	_calculateBoundingBox();
	return true;
}

void FContourModel::_getEdgesFromNode(const aiScene* pScene,
									  const aiNode* pNode,
									  FMatrix4f trafo)
{
	FMatrix4f localTrafo(&pNode->mTransformation.a1, FMatrix4f::RowWise);
	trafo *= localTrafo;
	const float faceEps = 0.1f;

	for (size_t m = 0; m < pNode->mNumMeshes; m++)
	{
		int meshId = pNode->mMeshes[m];
		const aiMesh* pMesh = pScene->mMeshes[meshId];
		float contourIndex = (float)pMesh->mMaterialIndex + 1.0f;

		F_CONSOLE("   importing contour #" << contourIndex
			<< "(" << pMesh->mName.data << "): " << pMesh->mNumVertices << " vertices");

		contour_t contourSubset;
		contourSubset.firstVertex = 2 * m_lines.size();
		contourSubset.name = pMesh->mName.data;

		aiVector3D* pFaceNormals = new aiVector3D[pMesh->mNumFaces];
		bool* pUseEdges = new bool[pMesh->mNumFaces * 3];

		for (int f = 0; f < pMesh->mNumFaces; f++)
		{
			const aiFace& face = pMesh->mFaces[f];
			F_ASSERT(face.mNumIndices == 3);

			// calculate face normal
			const aiVector3D& v0 = pMesh->mVertices[face.mIndices[0]];
			const aiVector3D& v1 = pMesh->mVertices[face.mIndices[1]];
			const aiVector3D& v2 = pMesh->mVertices[face.mIndices[2]];
			pFaceNormals[f] = (v2 - v0) ^ (v1 - v0);
			pFaceNormals[f].Normalize();

			bool bUseE01 = true;
			bool bUseE12 = true;
			bool bUseE20 = true;

			for (int ff = 0; ff < f; ff++)
			{
				const aiFace& face1 = pMesh->mFaces[ff];
				const aiVector3D& vv0 = pMesh->mVertices[face1.mIndices[0]];
				const aiVector3D& vv1 = pMesh->mVertices[face1.mIndices[1]];
				const aiVector3D& vv2 = pMesh->mVertices[face1.mIndices[2]];

				bool e0 = ((aiCmp(v0, vv0) && aiCmp(v1, vv1)) || (aiCmp(v0, vv1) && aiCmp(v1, vv2)) || (aiCmp(v0, vv2) && aiCmp(v1, vv0))
					|| (aiCmp(v1, vv0) && aiCmp(v0, vv1)) || (aiCmp(v1, vv1) && aiCmp(v0, vv2)) || (aiCmp(v1, vv2) && aiCmp(v0, vv0)));
				bool e1 = ((aiCmp(v1, vv0) && aiCmp(v2, vv1)) || (aiCmp(v1, vv1) && aiCmp(v2, vv2)) || (aiCmp(v1, vv2) && aiCmp(v2, vv0))
					|| (aiCmp(v2, vv0) && aiCmp(v1, vv1)) || (aiCmp(v2, vv1) && aiCmp(v1, vv2)) || (aiCmp(v2, vv2) && aiCmp(v1, vv0)));
				bool e2 = ((aiCmp(v2, vv0) && aiCmp(v0, vv1)) || (aiCmp(v2, vv1) && aiCmp(v0, vv2)) || (aiCmp(v2, vv2) && aiCmp(v0, vv0))
					|| (aiCmp(v0, vv0) && aiCmp(v2, vv1)) || (aiCmp(v0, vv1) && aiCmp(v2, vv2)) || (aiCmp(v0, vv2) && aiCmp(v2, vv0)));

				if (e0 || e1 || e2)
				{
					if ((fabsf(pFaceNormals[f].x - pFaceNormals[ff].x) < faceEps)
						&& (fabsf(pFaceNormals[f].y - pFaceNormals[ff].y) < faceEps)
						&& (fabsf(pFaceNormals[f].z - pFaceNormals[ff].z) < faceEps))
					{
						if ((aiCmp(v0, vv0) && aiCmp(v1, vv1)) || (aiCmp(v1, vv0) && aiCmp(v2, vv1)) || (aiCmp(v2, vv0) && aiCmp(v0, vv1))
							|| (aiCmp(v0, vv1) && aiCmp(v1, vv0)) || (aiCmp(v1, vv1) && aiCmp(v2, vv0)) || (aiCmp(v2, vv1) && aiCmp(v0, vv0)))
							pUseEdges[ff*3 + 0] = false;
						if ((aiCmp(v0, vv1) && aiCmp(v1, vv2)) || (aiCmp(v1, vv1) && aiCmp(v2, vv2)) || (aiCmp(v2, vv1) && aiCmp(v0, vv2))
							|| (aiCmp(v0, vv2) && aiCmp(v1, vv1)) || (aiCmp(v1, vv2) && aiCmp(v2, vv1)) || (aiCmp(v2, vv2) && aiCmp(v0, vv1)))
							pUseEdges[ff*3 + 1] = false;
						if ((aiCmp(v0, vv2) && aiCmp(v1, vv0)) || (aiCmp(v1, vv2) && aiCmp(v2, vv0)) || (aiCmp(v2, vv2) && aiCmp(v0, vv0))
							|| (aiCmp(v0, vv0) && aiCmp(v1, vv2)) || (aiCmp(v1, vv0) && aiCmp(v2, vv2)) || (aiCmp(v2, vv0) && aiCmp(v0, vv2)))
							pUseEdges[ff*3 + 2] = false;
					}
				}

				bUseE01 = bUseE01 && !e0;
				bUseE12 = bUseE12 && !e1;
				bUseE20 = bUseE20 && !e2;
			}

			pUseEdges[f*3 + 0] = bUseE01;
			pUseEdges[f*3 + 1] = bUseE12;
			pUseEdges[f*3 + 2] = bUseE20;
		}

		for (int f = 0; f < pMesh->mNumFaces; f++)
		{
			const aiFace& face = pMesh->mFaces[f];
			const aiVector3D& v0 = pMesh->mVertices[face.mIndices[0]];
			const aiVector3D& v1 = pMesh->mVertices[face.mIndices[1]];
			const aiVector3D& v2 = pMesh->mVertices[face.mIndices[2]];

			FVector4f p0h(v0.x, v0.y, v0.z, 1.0f);
			p0h = trafo * p0h;
			p0h.homogenize();
			FVector3f p0(p0h.x(), p0h.y(), p0h.z());

			FVector4f p1h(v1.x, v1.y, v1.z, 1.0f);
			p1h = trafo * p1h;
			p1h.homogenize();
			FVector3f p1(p1h.x(), p1h.y(), p1h.z());

			FVector4f p2h(v2.x, v2.y, v2.z, 1.0f);
			p2h = trafo * p2h;
			p2h.homogenize();
			FVector3f p2(p2h.x(), p2h.y(), p2h.z());

			if (pUseEdges[f*3 + 0])
				m_lines.push_back(line_t(p0, p1, contourIndex));
			if (pUseEdges[f*3 + 1])
				m_lines.push_back(line_t(p1, p2, contourIndex));
			if (pUseEdges[f*3 + 2])			
				m_lines.push_back(line_t(p2, p0, contourIndex));
		}

		delete[] pFaceNormals;
		delete[] pUseEdges;

		contourSubset.vertexCount = 2 * m_lines.size() - contourSubset.firstVertex;
		F_CONSOLE("   subset - first vertex: " << contourSubset.firstVertex
			<< ", vertex count: " << contourSubset.vertexCount);

		contourSubset.center.makeZero();
		size_t a = contourSubset.firstVertex / 2;
		size_t n = (contourSubset.firstVertex + contourSubset.vertexCount) / 2;

		for (size_t i = a; i < n; ++i)
			contourSubset.center += m_lines[i].v0.pos;

		contourSubset.center /= (float)(n - a);
		m_contours.push_back(contourSubset);

		F_CONSOLE("   contour has " << n << " lines, center is " << contourSubset.center.toString());
		F_CONSOLE("");
	}

	for (size_t c = 0; c < pNode->mNumChildren; c++)
		_getEdgesFromNode(pScene, pNode->mChildren[c], trafo);
}

void FContourModel::_calculateBoundingBox()
{
	FVector3f m_minPos(FLT_MAX, FLT_MAX, FLT_MAX);
	FVector3f m_maxPos(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (size_t i = 0; i < m_lines.size(); ++i)
	{
		m_minPos = m_minPos.minimum(m_lines[i].v0.pos);
		m_maxPos = m_maxPos.maximum(m_lines[i].v0.pos);
	}

	m_boundingBox.set(m_minPos, m_maxPos);
}

// ----------------------------------------------------------------------------------------------------