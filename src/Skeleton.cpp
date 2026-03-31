#include "Skeleton.h"
#include "Animation.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <cassert>

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

const SkeletonNode* SkeletonNode::GetNodeByName(const std::string& name) const
{
	if(mName == name)
	{
		return this;
	}
	for(const SkeletonNode& child : mChilds)
	{
		if(const SkeletonNode* result = child.GetNodeByName(name))
		{
			return result;
		}
	}
	return nullptr;
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


void Skeleton::CalculateBoneTransform(
	const SkeletonNode& node, Matrix4x4 parentTransform,
	const Animation& a, const Animation& b, float t,
	Matrix4x4 finalBoneMatrices[], float aTime, float bTime) const
{
	Matrix4x4 nodeTransform = node.GetTransformation();
	if(a.ContainsBone(node.GetName()) && b.ContainsBone(node.GetName()))
	{
		const Bone& aBone = a.GetBone(node.GetName());
		const Bone& bBone = b.GetBone(node.GetName());
		nodeTransform = Bone::Interpolate(aBone, bBone, aTime, bTime, t);
	}

	Matrix4x4 globalTransform = nodeTransform * parentTransform;
	if(mBoneInfos.find(node.GetName()) != mBoneInfos.end())
	{
		const BoneInfo& boneInfo = mBoneInfos.at(node.GetName());
		finalBoneMatrices[boneInfo.ID] = boneInfo.Offset * globalTransform;
	}
	for(int i = 0; i < node.GetChildCount(); i++)
	{
		CalculateBoneTransform(node.GetChild(i), globalTransform, 
			a, b, t, finalBoneMatrices, aTime, bTime);
	}
}

void Skeleton::CalculateBoneTransform(
	const SkeletonNode& node, Matrix4x4 parentTransform,
	const Animation& a, const Animation& b, const Animation& c, const Animation& d,
	float t0, float t1, float t2, Matrix4x4 finalBoneMatrices[],
	float aTime, float bTime, float cTime, float dTime) const
{
	Matrix4x4 nodeTransform = node.GetTransformation();
	if(a.ContainsBone(node.GetName()) && b.ContainsBone(node.GetName()) &&
	   c.ContainsBone(node.GetName()) && d.ContainsBone(node.GetName()))
	{
		const Bone& aBone = a.GetBone(node.GetName());
		const Bone& bBone = b.GetBone(node.GetName());
		const Bone& cBone = c.GetBone(node.GetName());
		const Bone& dBone = d.GetBone(node.GetName());
		nodeTransform = Bone::Interpolate(
			aBone, bBone, cBone, dBone,
			aTime, bTime, cTime, dTime, t0, t1, t2);
	}

	Matrix4x4 globalTransform = nodeTransform * parentTransform;
	if(mBoneInfos.find(node.GetName()) != mBoneInfos.end())
	{
		const BoneInfo& boneInfo = mBoneInfos.at(node.GetName());
		finalBoneMatrices[boneInfo.ID] = boneInfo.Offset * globalTransform;
	}
	for(int i = 0; i < node.GetChildCount(); i++)
	{
		CalculateBoneTransform(node.GetChild(i), globalTransform, 
			a, b, c, d, t0, t1, t2, finalBoneMatrices, aTime, bTime, cTime, dTime);
	}
}

void Skeleton::CalculateBoneTransform(
	const SkeletonNode& node, Matrix4x4 parentTransform,
	Animation* animations[], float timeStamps[], float animationTimes[], float b,
	Matrix4x4 finalBoneMatrices[])
{
	float interval = std::fabs(timeStamps[1] - timeStamps[0]);
	int index0 = std::floor((b - timeStamps[0]) / interval);
	int index1 = std::ceil((b - timeStamps[0]) / interval);
	float t = 0.0f;
	float numerator = (b - timeStamps[index0]);
	float  denomnator = (timeStamps[index1] - timeStamps[index0]);
	if(denomnator > FLT_EPSILON)
	{
		t = numerator / denomnator;
	} 
	CalculateBoneTransform(node, parentTransform,
		*animations[index0], *animations[index1], t,
		finalBoneMatrices, animationTimes[index0], animationTimes[index1]);
}

void Skeleton::CalculateBoneTransform(
	const SkeletonNode& node, Matrix4x4 parentTransform,
	Animation* animations0[], float timeStamps0[], float animationTimes0[], float b0,
	Animation* animations1[], float timeStamps1[], float animationTimes1[], float b1,
	float b2, Matrix4x4 finalBoneMatrices[])
{
	float interval0 = std::fabs(timeStamps0[1] - timeStamps0[0]);
	int indexStart0 = std::floor((b0 - timeStamps0[0]) / interval0);
	int indexEnd0 = std::ceil((b0 - timeStamps0[0]) / interval0);
	float t0 = 0.0f;
	float numerator0 = (b0 - timeStamps0[indexStart0]);
	float denomnator0 = (timeStamps0[indexEnd0] - timeStamps0[indexStart0]);
	if(denomnator0 > FLT_EPSILON)
	{
		t0 = numerator0 / denomnator0;
	}

	float interval1 = std::fabs(timeStamps1[1] - timeStamps1[0]);
	int indexStart1 = std::floor((b1 - timeStamps1[0]) / interval1);
	int indexEnd1 = std::ceil((b1 - timeStamps1[0]) / interval1);
	float t1 = 0.0f;
	float numerator1 = (b1 - timeStamps1[indexStart1]);
	float denomnator1 = (timeStamps1[indexEnd1] - timeStamps1[indexStart1]);
	if(denomnator1 > FLT_EPSILON)
	{
		t1 = numerator1 / denomnator1;
	}

	CalculateBoneTransform(node, parentTransform,
		*animations0[indexStart0], *animations0[indexEnd0], *animations1[indexStart1], *animations1[indexEnd1],
		t0, t1, b2, finalBoneMatrices,
		animationTimes0[indexStart0], animationTimes0[indexEnd0], animationTimes1[indexStart1], animationTimes1[indexEnd1]);
}

const SkeletonNode& Skeleton::GetRoot() const
{
	return *mRoot;
}

const SkeletonNode& Skeleton::GetNode(const std::string& name) const
{
	return *mRoot->GetNodeByName(name);
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