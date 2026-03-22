#include "Model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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

Model::Model(const char* modelFilepath, const char* textureFilePath)
{
	/*
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelFilepath))
	{
	    throw std::runtime_error(err);
	}

	std::vector<ModelVertex> vertices;
	vertices.reserve(attrib.vertices.size());
	std::vector<int> indices;
	for (const auto& shape : shapes)
	{
	    for (const auto& index : shape.mesh.indices)
	    {
	        ModelVertex vertex{};

	        vertex.Position = Vector3{
			    attrib.vertices[3 * index.vertex_index + 0],
			    attrib.vertices[3 * index.vertex_index + 1],
			    attrib.vertices[3 * index.vertex_index + 2]
			};

	        vertex.Normal = Vector3{
			    attrib.normals[3 * index.normal_index + 0],
			    attrib.normals[3 * index.normal_index + 1],
			    attrib.normals[3 * index.normal_index + 2]
			};

			vertex.TCoord = Vector2{
			    attrib.texcoords[2 * index.texcoord_index + 0],
			    attrib.texcoords[2 * index.texcoord_index + 1]
			};

	        vertices.push_back(vertex);
	        indices.push_back(indices.size());
	    }
	}
	*/

	/*
		aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder |
		aiProcess_Triangulate | aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace
	*/

	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(modelFilepath, aiProcess_Triangulate);

	std::vector<ModelVertex> vertices;
	std::vector<int> indices;
	for(int k = 0; k < scene->mNumMeshes; ++k)
	{
		aiMesh *mesh = scene->mMeshes[k]; 

		for(int i = 0; i < mesh->mNumVertices; i++)
		{
		   ModelVertex vertex{};
		   vertex.Position = Vector3{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
		   vertex.Normal = Vector3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		   vertex.TCoord = Vector2{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		   vertices.push_back(vertex);
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