// ----------------------------------------------------------------------------------------------------
//  Title			FGenericModel.cpp
//  Description		Implementation of class FGenericModel
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-10 22:55:38 +0200 (Sa, 10 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include <assimp.hpp>
#include <aiScene.h>
#include <aiPostProcess.h>

#include "FGenericModel.h"
#include "FMemoryTracer.h"

#define CMP_EPS 0.01f

inline bool aiCmp(const aiVector3D& v0, const aiVector3D& v1)
{
	return (v0.x < v1.x + CMP_EPS && v0.x > v1.x - CMP_EPS
		 && v0.y < v1.y + CMP_EPS && v0.y > v1.y - CMP_EPS
		 && v0.z < v1.z + CMP_EPS && v0.z > v1.z - CMP_EPS);
}

// ----------------------------------------------------------------------------------------------------
//  Class FGenericModel
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FGenericModel::FGenericModel()
{
}

FGenericModel::~FGenericModel()
{
}

// Public commands ------------------------------------------------------------------------------------

void FGenericModel::createImportModel(const QString& fileName)
{
	QFileInfo info(fileName);
	if (!info.exists())
		return;

	QString baseName = info.baseName();
	if (baseName.endsWith("edges", Qt::CaseInsensitive))
	{
		m_edgeModelFile = info.absoluteFilePath();
		m_solidModelFile = m_edgeModelFile;
		m_solidModelFile.replace("edges", "solid", Qt::CaseInsensitive);

		QFileInfo solidFileInfo(m_solidModelFile);
		if (!solidFileInfo.exists())
			m_solidModelFile = m_edgeModelFile;
	}
	else if (baseName.endsWith("solid", Qt::CaseInsensitive))
	{
		m_solidModelFile = info.absoluteFilePath();
		m_edgeModelFile = m_solidModelFile;
		m_edgeModelFile.replace("solid", "edges", Qt::CaseInsensitive);

		QFileInfo edgeFileInfo(m_edgeModelFile);
		if (!edgeFileInfo.exists())
			m_edgeModelFile = m_solidModelFile;
	}
	else
	{
		m_solidModelFile = info.absoluteFilePath();
		m_edgeModelFile = info.absoluteFilePath();
	}

	create();
}

void FGenericModel::setSolidModelFile(const QString& fileName)
{
	m_solidModelFile = fileName;
}

void FGenericModel::setEdgeModelFile(const QString& fileName)
{
	m_edgeModelFile = fileName;
}

// Public queries -------------------------------------------------------------------------------------

// Overrides ------------------------------------------------------------------------------------------

void FGenericModel::onCreateSolidMesh(FGLMesh& solidMesh)
{
	solidMesh.import(m_solidModelFile);
}

void FGenericModel::onCreateEdges(edgeVec_t& edges)
{
	if (!_importLineModel(m_edgeModelFile, edges))
	{
		F_ASSERT(!"FGenericModel::onCreateEdges - Failed to import edge model");
		edges.push_back(edge_t(0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
	}
}

void FGenericModel::onCreateFaces(faceVec_t& faces)
{
}

// Internal functions ---------------------------------------------------------------------------------

bool FGenericModel::_importLineModel(const QString& fileName, edgeVec_t& edges)
{
	F_TRACE(QString("FGenericModel::_importLineModel - Importing %1").arg(fileName));
	QByteArray fnAscii = fileName.toAscii();

	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT);
	//importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_MATERIALS);
	quint32 flags = aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices;

	const aiScene* pScene = importer.ReadFile(fnAscii, flags);
	if (pScene == NULL)
		return false;

	edges.clear();
	FMatrix4f trafo;
	trafo.makeIdentity();

	_getEdgesFromNode(pScene, pScene->mRootNode, trafo, edges);
	if (edges.empty())
		return false;

	// check again for double edges
	/*
	bool* bDel = new bool[edges.size()];
	for (size_t e = 0; e < edges.size(); e++)
		bDel[e] = false;

	for (size_t e = 0; e < edges.size(); e++)
	{
		for (size_t ee = 0; ee < e; ee++)
			if (edges[e].isEqual(edges[ee]))
				bDel[ee] = true;
	}

	for (size_t e = 0; e < edges.size();)
	{
		if (bDel[e])
			edges.erase(edges.begin() + e);
		else
			e++;
	}
	*/

	F_TRACE(QString("FGenericModel::_importLineModel - Successfully imported %1 edges")
		.arg(edges.size()));

	for (size_t e = 0; e < edges.size(); e++)
	{
		edge_t& ed = edges[e];
		F_CONSOLE(e << ": " << ed.modelPoint[0].toString() << " - " << ed.modelPoint[1].toString()
			<< ", n0: " << ed.faceNormal[0].toString() << ", n1: " << ed.faceNormal[1].toString()
			<< ", density: " << ed.samplingDensity);
	}
	
	return true;
}

void FGenericModel::_getEdgesFromNode(const aiScene* pScene, const aiNode* pNode,
								 FMatrix4f trafo, edgeVec_t& edges)
{
	FMatrix4f localTrafo(&pNode->mTransformation.a1, FMatrix4f::RowWise);
	trafo *= localTrafo;
	const float faceEps = 0.1f;

	for (size_t m = 0; m < pNode->mNumMeshes; m++)
	{
		int meshId = pNode->mMeshes[m];
		const aiMesh* pMesh = pScene->mMeshes[meshId];

		aiVector3D* pFaceNormals = new aiVector3D[pMesh->mNumFaces];
		bool* pUseEdges = new bool[pMesh->mNumFaces * 3];
		aiVector3D* pAdjFaceNormals = new aiVector3D[pMesh->mNumFaces * 3];
		for (int i = 0; i < pMesh->mNumFaces * 3; i++)
			pAdjFaceNormals[i] = aiVector3D(0.0f, 0.0f, 0.0f);

		float meshSampleDensity = 1.0f;
		if (pScene->HasMaterials())
		{
			aiMaterial* pMat = pScene->mMaterials[pMesh->mMaterialIndex];
			aiColor3D color(0.0f, 0.0f, 0.0f);
			pMat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			meshSampleDensity = color.r * 0.99609375f * 2.0f;
		}

		for (int f = 0; f < pMesh->mNumFaces; f++)
		{
			const aiFace& face = pMesh->mFaces[f];
			F_ASSERT(face.mNumIndices == 3);

			// calculate face normal
			const aiVector3D& v0 = pMesh->mVertices[face.mIndices[0]];
			const aiVector3D& v1 = pMesh->mVertices[face.mIndices[1]];
			const aiVector3D& v2 = pMesh->mVertices[face.mIndices[2]];
			pFaceNormals[f] = (v1 - v0) ^ (v2 - v0);
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
				
				// e0 true > (v0, v1) already exists
				bool e0 = ((aiCmp(v0, vv0) && aiCmp(v1, vv1)) || (aiCmp(v0, vv1) && aiCmp(v1, vv2)) || (aiCmp(v0, vv2) && aiCmp(v1, vv0))
					    || (aiCmp(v1, vv0) && aiCmp(v0, vv1)) || (aiCmp(v1, vv1) && aiCmp(v0, vv2)) || (aiCmp(v1, vv2) && aiCmp(v0, vv0)));
				// e1 true > (v1, v2) already exists
				bool e1 = ((aiCmp(v1, vv0) && aiCmp(v2, vv1)) || (aiCmp(v1, vv1) && aiCmp(v2, vv2)) || (aiCmp(v1, vv2) && aiCmp(v2, vv0))
					    || (aiCmp(v2, vv0) && aiCmp(v1, vv1)) || (aiCmp(v2, vv1) && aiCmp(v1, vv2)) || (aiCmp(v2, vv2) && aiCmp(v1, vv0)));
				// e2 true > (v2, v0) already exists
				bool e2 = ((aiCmp(v2, vv0) && aiCmp(v0, vv1)) || (aiCmp(v2, vv1) && aiCmp(v0, vv2)) || (aiCmp(v2, vv2) && aiCmp(v0, vv0))
					    || (aiCmp(v0, vv0) && aiCmp(v2, vv1)) || (aiCmp(v0, vv1) && aiCmp(v2, vv2)) || (aiCmp(v0, vv2) && aiCmp(v2, vv0)));

				if (e0 || e1 || e2)
				{
					bool equalFaces = ((fabsf(pFaceNormals[f].x - pFaceNormals[ff].x) < faceEps)
						&& (fabsf(pFaceNormals[f].y - pFaceNormals[ff].y) < faceEps)
						&& (fabsf(pFaceNormals[f].z - pFaceNormals[ff].z) < faceEps));

					if ((aiCmp(v0, vv0) && aiCmp(v1, vv1)) || (aiCmp(v1, vv0) && aiCmp(v2, vv1)) || (aiCmp(v2, vv0) && aiCmp(v0, vv1))
					 || (aiCmp(v0, vv1) && aiCmp(v1, vv0)) || (aiCmp(v1, vv1) && aiCmp(v2, vv0)) || (aiCmp(v2, vv1) && aiCmp(v0, vv0)))
					{
						if (equalFaces)
							pUseEdges[ff*3 + 0] = false;
						else
							pAdjFaceNormals[ff*3 + 0] = pFaceNormals[f];
					} 
					if ((aiCmp(v0, vv1) && aiCmp(v1, vv2)) || (aiCmp(v1, vv1) && aiCmp(v2, vv2)) || (aiCmp(v2, vv1) && aiCmp(v0, vv2))
					 || (aiCmp(v0, vv2) && aiCmp(v1, vv1)) || (aiCmp(v1, vv2) && aiCmp(v2, vv1)) || (aiCmp(v2, vv2) && aiCmp(v0, vv1)))
					{
						if (equalFaces)
							pUseEdges[ff*3 + 1] = false;
						else
							pAdjFaceNormals[ff*3 + 1] = pFaceNormals[f];
					}
					if ((aiCmp(v0, vv2) && aiCmp(v1, vv0)) || (aiCmp(v1, vv2) && aiCmp(v2, vv0)) || (aiCmp(v2, vv2) && aiCmp(v0, vv0))
					 || (aiCmp(v0, vv0) && aiCmp(v1, vv2)) || (aiCmp(v1, vv0) && aiCmp(v2, vv2)) || (aiCmp(v2, vv0) && aiCmp(v0, vv2)))
					{
						if (equalFaces)
							pUseEdges[ff*3 + 2] = false;
						else
							pAdjFaceNormals[ff*3 + 2] = pFaceNormals[f];
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

			FVector4f p0(v0.x, v0.y, v0.z, 1.0f);
			p0 = trafo * p0;
			p0.homogenize();

			FVector4f p1(v1.x, v1.y, v1.z, 1.0f);
			p1 = trafo * p1;
			p1.homogenize();

			FVector4f p2(v2.x, v2.y, v2.z, 1.0f);
			p2 = trafo * p2;
			p2.homogenize();

			const aiVector3D& vn = pFaceNormals[f];
			const aiVector3D& avn0 = pAdjFaceNormals[f*3 + 0];
			const aiVector3D& avn1 = pAdjFaceNormals[f*3 + 1];
			const aiVector3D& avn2 = pAdjFaceNormals[f*3 + 2];

			FVector4f n(vn.x, vn.y, vn.z, 0.0f);
			FVector4f n0(avn0.x, avn0.y, avn0.z, 0.0f);
			FVector4f n1(avn1.x, avn1.y, avn1.z, 0.0f);
			FVector4f n2(avn2.x, avn2.y, avn2.z, 0.0f);

			n = trafo * n;
			n.normalize();

			if (n0.x() == 0.0f && n0.y() == 0.0f && n0.z() == 0.0f)
			{
				n0 = n;
			}
			else
			{
				n0 = trafo * n0;
				n0.normalize();
			}

			if (n1.x() == 0.0f && n1.y() == 0.0f && n1.z() == 0.0f)
			{
				n1 = n;
			}
			else
			{
				n1 = trafo * n1;
				n1.normalize();
			}

			if (n2.x() == 0.0f && n2.y() == 0.0f && n2.z() == 0.0f)
			{
				n2 = n;
			}
			else
			{
				n2 = trafo * n2;
				n2.normalize();
			}

			if (pUseEdges[f*3 + 0])
				edges.push_back(edge_t(p0, p1, n, n0, meshSampleDensity));
			if (pUseEdges[f*3 + 1])
				edges.push_back(edge_t(p1, p2, n, n1, meshSampleDensity));
			if (pUseEdges[f*3 + 2])			
				edges.push_back(edge_t(p2, p0, n, n2, meshSampleDensity));
		}

		delete[] pFaceNormals;
		delete[] pUseEdges;
		delete[] pAdjFaceNormals;
	}

	for (size_t c = 0; c < pNode->mNumChildren; c++)
		_getEdgesFromNode(pScene, pNode->mChildren[c], trafo, edges);
}

// ----------------------------------------------------------------------------------------------------