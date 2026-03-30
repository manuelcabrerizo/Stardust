#include "Model.h"

#include "Renderer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Texture2D.h"

#include <vector>
#include <exception>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <unordered_map>

Model::Model(const char* modelFilepath, const char* textureFilePath)
{
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(modelFilepath, aiProcess_Triangulate);

	std::vector<ModelVertex> vertices;
	std::vector<int> indices;
	std::unordered_map<std::string, int> boneIds;
	for(int k = 0; k < scene->mNumMeshes; ++k)
	{
		aiMesh *mesh = scene->mMeshes[k]; 

		for(int i = 0; i < mesh->mNumVertices; i++)
		{
			ModelVertex vertex{};
			vertex.Position = Vector3{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
			vertex.Normal = Vector3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
			vertex.TCoord = Vector2{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
			for(int j = 0; j < MAX_BONE_INFLUENCE; j++)
			{
				vertex.BoneId[j] = -1;
				vertex.Weights[j] = 0.0f;
			}
			vertices.push_back(vertex);
		}

		for(int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) 
		{
			int boneId = -1;
			std::string boneName(mesh->mBones[boneIndex]->mName.C_Str());
			if(boneIds.find(boneName) == boneIds.end())
			{
				boneId = boneIds.size();
				boneIds.emplace(boneName, boneId);
			}
			else
			{
				boneId = boneIds.at(boneName);
			}
			auto weights = mesh->mBones[boneIndex]->mWeights;
			int weightsCount = mesh->mBones[boneIndex]->mNumWeights;
			for(int weightIndex = 0; weightIndex < weightsCount; ++weightIndex)
			{
				int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;
				for(int j = 0; j < MAX_BONE_INFLUENCE; ++j) 
				{
					if(vertices[vertexId].BoneId[j] < 0)
					{
						vertices[vertexId].Weights[j] = weight;
						vertices[vertexId].BoneId[j] = boneId;
						break;
					}
				}
			}
		}

		for(int i = 0; i < mesh->mNumFaces; i++)
		{
		   aiFace *face = mesh->mFaces + i;
		   for(unsigned int j = 0; j < face->mNumIndices; j++)
		   {
		        indices.push_back(face->mIndices[j]);
		   }
		}
	}

	mVertexBuffer = new VertexBuffer(vertices.data(), vertices.size(), sizeof(ModelVertex));
	mIndexBuffer = new IndexBuffer(indices.data(), indices.size());
	mTexture = new Texture2D(textureFilePath, false);
}

Model::~Model()
{
	delete mTexture;
	delete mVertexBuffer;
	delete mIndexBuffer;
}

void Model::Load(Renderer* renderer)
{
	renderer->LoadVertexBuffer(mVertexBuffer);
	renderer->LoadIndexBuffer(mIndexBuffer);
	renderer->LoadTexture2D(mTexture);
}

void Model::Release(Renderer* renderer)
{
	renderer->ReleaseVertexBuffer(mVertexBuffer);
	renderer->ReleaseIndexBuffer(mIndexBuffer);
	renderer->ReleaseTexture2D(mTexture);
}

void Model::Draw(Renderer* renderer)
{
	renderer->PushTexture(mTexture, 0);
	renderer->PushIndexBuffer(mIndexBuffer, mVertexBuffer);
}