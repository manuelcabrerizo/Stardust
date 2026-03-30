#include "Skeleton.h"
#include "Animation.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static Matrix4x4 FromAssimp(const aiMatrix4x4& m)
{
	Matrix4x4 mat = {{
		{m.a1, m.b1, m.c1, m.d1},
		{m.a2, m.b2, m.c2, m.d2},
		{m.a3, m.b3, m.c3, m.d3},
		{m.a4, m.b4, m.c4, m.d4}
	}};
	return mat;
}

SkeletonNode::SkeletonNode(aiNode* node)
	: mName(node->mName.C_Str())
{
	mTransformation = FromAssimp(node->mTransformation);
	if(node->mNumChildren <= 0)
	{
		return;
	}
	mChilds.reserve(node->mNumChildren);
	for(int i = 0; i < node->mNumChildren; i++)
	{
		mChilds.emplace_back(node->mChildren[i]);
	}
}

SkeletonNode::~SkeletonNode()
{
	mTransformation = Matrix4x4::Identity;
	mName.clear();
	mChilds.clear();
}

const Matrix4x4& SkeletonNode::GetTransformation() const
{
	return mTransformation;
}

const std::string& SkeletonNode::GetName() const
{
	return mName;
}

int SkeletonNode::GetChildCount() const
{
	return mChilds.size();
}

const SkeletonNode& SkeletonNode::GetChild(int index) const
{
	return mChilds[index];
}

Skeleton::Skeleton(const char* skeletonFilepath)
{
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(skeletonFilepath, aiProcess_Triangulate);
	mRoot = new SkeletonNode(scene->mRootNode);

	int boneCounter = 0;
	for(int k = 0; k < scene->mNumMeshes; ++k)
	{
		aiMesh *mesh = scene->mMeshes[k]; 
		for(int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) 
		{
			std::string boneName(mesh->mBones[boneIndex]->mName.C_Str());
			if(mBoneInfos.find(boneName) == mBoneInfos.end())
			{
				BoneInfo boneInfo;
				boneInfo.ID = boneCounter;
				boneInfo.Offset = FromAssimp(mesh->mBones[boneIndex]->mOffsetMatrix);
				mBoneInfos.emplace(boneName, boneInfo);
				boneCounter++;
			}
		}
	}
}

Skeleton::~Skeleton()
{
	delete mRoot;
	mBoneInfos.clear();
}

void Skeleton::CalculateBoneTransform(
	const Animation& animation, const SkeletonNode& node,
	Matrix4x4 parentTransform, Matrix4x4 finalBoneMatrices[], float t) const
{
	Matrix4x4 nodeTransform = node.GetTransformation();
	if(animation.ContainsBone(node.GetName()))
	{
		const Bone& bone = animation.GetBone(node.GetName());
		nodeTransform = bone.GetMatrix(t);
	}
	Matrix4x4 globalTransform = nodeTransform * parentTransform;
	if(mBoneInfos.find(node.GetName()) != mBoneInfos.end())
	{
		const BoneInfo& boneInfo = mBoneInfos.at(node.GetName());
		finalBoneMatrices[boneInfo.ID] = boneInfo.Offset * globalTransform;
	}
	for(int i = 0; i < node.GetChildCount(); i++)
	{
		CalculateBoneTransform(animation, node.GetChild(i), globalTransform, finalBoneMatrices, t);
	}
}

const SkeletonNode& Skeleton::GetRoot() const
{
	return *mRoot;
}

bool Skeleton::ContainsBoneInfo(const std::string& name) const
{
	return mBoneInfos.find(name) != mBoneInfos.end();
}

const BoneInfo& Skeleton::GetBoneInfo(const std::string& name) const
{
	auto it = mBoneInfos.find(name);
	return it->second;
}