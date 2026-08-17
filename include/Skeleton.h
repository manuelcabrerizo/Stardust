#ifndef SKELETON_H
#define SKELETON_H

#include "Config.h"

#include "math/Matrix4x4.h"

#include <string>
#include <vector>
#include <unordered_map>

#include "Bone.h"

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
	const SkeletonNode* GetNodeByName(const std::string& name) const;

	mutable bool HasInterpolatesValue;
	mutable InterpolationBone InterpolationBone;
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

	void CalculateBoneTransform(
		const SkeletonNode& node, Matrix4x4 parentTransform,
		const Animation& a, const Animation& b, float t,
		Matrix4x4 finalBoneMatrices[], float aTime, float bTime) const;

	void CalculateBoneTransform(
		const SkeletonNode& node, Matrix4x4 parentTransform,
		const Animation& a, const Animation& b, const Animation& c, const Animation& d,
		float t0, float t1, float t2, Matrix4x4 finalBoneMatrices[],
		float aTime, float bTime, float cTime, float dTime) const;

	void CalculateBoneTransform(
		const SkeletonNode& node, Matrix4x4 parentTransform,
		Animation* animations[], float timeStamps[], float animationTimes[], float b,
		Matrix4x4 finalBoneMatrices[]);

	void CalculateBoneTransform(
		const SkeletonNode& node, Matrix4x4 parentTransform,
		Animation* animations0[], float timeStamps0[], float animationTimes0[], float b0,
		Animation* animations1[], float timeStamps1[], float animationTimes1[], float b1,
		float b2, Matrix4x4 finalBoneMatrices[]);

	void StartInterpolation();
	void EndInterpolation(const SkeletonNode& node, Matrix4x4 parentTransform, Matrix4x4 finalBoneMatrices[]);
	
	void Interpolate(const SkeletonNode& node,
		const Animation& a, float aTime, float t);

	void Interpolate(
		const SkeletonNode& node,
		const Animation& a, const Animation& b, const Animation& c, const Animation& d,
		float t0, float t1, float t2,
		float aTime, float bTime, float cTime, float dTime, float t);
	
	void Interpolate(
		const SkeletonNode& node,
		Animation* animations0[], float timeStamps0[], float animationTimes0[], float b0,
		Animation* animations1[], float timeStamps1[], float animationTimes1[], float b1,
		float b2, float t);

	int GetBoneIndex(const std::string& bone) const;

	const SkeletonNode& GetRoot() const;
	const SkeletonNode& GetNode(const std::string& name) const;
	bool ContainsBoneInfo(const std::string& name) const;
	const BoneInfo& GetBoneInfo(const std::string& name) const;
private:
	SkeletonNode* mRoot;
	std::unordered_map<std::string, BoneInfo> mBoneInfos;
};

#endif