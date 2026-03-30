#include "Bone.h"

#include <assimp/scene.h>

static Vector3 FromAssimp(const aiVector3D& v)
{
    return Vector3(v.x, v.y, v.z);
}

static Quaternion FromAssimp(const aiQuaternion& q)
{
    return Quaternion(q.w, q.x, q.y, q.z);
}

static float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) 
{
	return (animationTime - lastTimeStamp) / (nextTimeStamp - lastTimeStamp);;
}

Bone::Bone(aiNodeAnim* channel)
	: mName(channel->mNodeName.C_Str())
{
	mPositions.reserve(channel->mNumPositionKeys);
	mScales.reserve(channel->mNumRotationKeys);
	mRotations.reserve(channel->mNumScalingKeys);
    for(int i = 0; i < channel->mNumPositionKeys; ++i)
    {
        aiVector3D position = channel->mPositionKeys[i].mValue;
        float timeStamp = channel->mPositionKeys[i].mTime;
        KeyVector3 keyPosition;
        keyPosition.Value = FromAssimp(position);
        keyPosition.TimeStamp = timeStamp;
        mPositions.push_back(keyPosition);
    }
    for(int i = 0; i < channel->mNumRotationKeys; ++i)
    {
        aiQuaternion rotation = channel->mRotationKeys[i].mValue;
        float timeStamp = channel->mRotationKeys[i].mTime;
        KeyQuaternion keyRotation;
        keyRotation.Value = FromAssimp(rotation);
        keyRotation.TimeStamp = timeStamp;
        mRotations.push_back(keyRotation);
    }
    for(int i = 0; i < channel->mNumScalingKeys; ++i)
    {
        aiVector3D scale = channel->mScalingKeys[i].mValue;
        float timeStamp = channel->mScalingKeys[i].mTime;
        KeyVector3 keyScale;
        keyScale.Value = FromAssimp(scale);
        keyScale.TimeStamp = timeStamp;
        mScales.push_back(keyScale);
    }
}

Bone::~Bone()
{
	mName.clear();
	mPositions.clear();
	mRotations.clear();
	mScales.clear();
}

const std::string& Bone::GetName() const
{
	return mName;
}

Matrix4x4 Bone::GetMatrix(float animationTime) const
{
	Matrix4x4 translation = Matrix4x4::Translate(Interpolate(mPositions, animationTime));
	Matrix4x4 rotation = Interpolate(mRotations, animationTime).ToMatrix();
	Matrix4x4 scale = Matrix4x4::Scale(Interpolate(mScales, animationTime));
	return scale * rotation * translation;
}

Vector3 Bone::Interpolate(const std::vector<KeyVector3>& array, float animationTime) const
{
	if(array.size() == 1)
	{
		return array[0].Value;
	}
	int index0 = GetIndex<KeyVector3>(array, animationTime);
	int index1 = index0 + 1;
	float scaleFactor = GetScaleFactor(array[index0].TimeStamp, array[index1].TimeStamp, animationTime);
	return array[index0].Value.Lerp(array[index1].Value, scaleFactor);
}

Quaternion Bone::Interpolate(const std::vector<KeyQuaternion>& array, float animationTime) const
{
	if(array.size() == 1) 
	{
		return array[0].Value.Normalized();
	}
	int index0 = GetIndex<KeyQuaternion>(array, animationTime);
	int index1 = index0 + 1;
	float scaleFactor = GetScaleFactor(array[index0].TimeStamp, array[index1].TimeStamp, animationTime);
	Quaternion rot = Quaternion::Slerp(array[index0].Value, array[index1].Value, scaleFactor);
	return rot.Normalized();
}