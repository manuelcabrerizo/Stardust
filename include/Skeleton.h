#ifndef SKELETON_H
#define SKELETON_H

#include "Config.h"

#include "math/Matrix4x4.h"

#include <string>
#include <vector>
#include <unordered_map>

struct aiNode;
class Animation;

struct BoneInfo
{
	int ID;
	Matrix4x4 Offset;
};

class SD_API SkeletonNode
{
public:
	SkeletonNode(aiNode* node);
	~SkeletonNode();
	const Matrix4x4& GetTransformation() const;
	const std::string& GetName() const;
	int GetChildCount() const;
	const SkeletonNode& GetChild(int index) const;
private:
	Matrix4x4 mTransformation;
	std::string mName;
	std::vector<SkeletonNode> mChilds;
};

class SD_API Skeleton
{
public:
	Skeleton(const char* skeletonFilepath);
	~Skeleton();
	void CalculateBoneTransform(
		const Animation& animation, const SkeletonNode& node,
		Matrix4x4 parentTransform, Matrix4x4 finalBoneMatrices[], float t) const;
	const SkeletonNode& GetRoot() const;
	bool ContainsBoneInfo(const std::string& name) const;
	const BoneInfo& GetBoneInfo(const std::string& name) const;
private:
	SkeletonNode* mRoot;
	std::unordered_map<std::string, BoneInfo> mBoneInfos;
};

#endif