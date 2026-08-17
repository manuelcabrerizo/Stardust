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
	Matrix4x4 tMat = Matrix4x4::Translate(Interpolate(mPositions, animationTime));
	Matrix4x4 rMat = Interpolate(mRotations, animationTime).ToMatrix();
	Matrix4x4 sMat = Matrix4x4::Scale(Interpolate(mScales, animationTime));
	return sMat * rMat * tMat;
}

Matrix4x4 Bone::GetMatrix(const InterpolationBone& interpolatedBone)
{
	Matrix4x4 tMat = Matrix4x4::Translate(interpolatedBone.Position);
	Matrix4x4 rMat = interpolatedBone.Rotation.ToMatrix();
	Matrix4x4 sMat = Matrix4x4::Scale(interpolatedBone.Scale);
	return sMat * rMat * tMat;
}

Matrix4x4 Bone::Interpolate(const Bone& a, const Bone& b, float aTime, float bTime, float t)
{
	Vector3 aPos = a.Interpolate(a.mPositions, aTime);
	Quaternion aRot = a.Interpolate(a.mRotations, aTime);
	Vector3 aSca = a.Interpolate(a.mScales, aTime);
	Vector3 bPos = b.Interpolate(b.mPositions, bTime);
	Quaternion bRot = b.Interpolate(b.mRotations, bTime);
	Vector3 bSca = b.Interpolate(b.mScales, bTime);

	Matrix4x4 tMat = Matrix4x4::Translate(Vector3::Lerp(aPos, bPos, t));
	Matrix4x4 rMat = Quaternion::Slerp(aRot, bRot, t).ToMatrix();
	Matrix4x4 sMat = Matrix4x4::Scale(Vector3::Lerp(aSca, bSca, t));	
	return sMat * rMat * tMat;
}

Matrix4x4 Bone::Interpolate(
	const Bone& a, const Bone& b, const Bone& c, const Bone& d,
	float aTime, float bTime, float cTime, float dTime,
	float t0, float t1, float t2)
{
	Vector3 aPos = a.Interpolate(a.mPositions, aTime);
	Quaternion aRot = a.Interpolate(a.mRotations, aTime);
	Vector3 aSca = a.Interpolate(a.mScales, aTime);
	Vector3 bPos = b.Interpolate(b.mPositions, bTime);
	Quaternion bRot = b.Interpolate(b.mRotations, bTime);
	Vector3 bSca = b.Interpolate(b.mScales, bTime);

	Vector3 abPos = Vector3::Lerp(aPos, bPos, t0);
	Quaternion abRot = Quaternion::Slerp(aRot, bRot, t0);
	Vector3 abSca = Vector3::Lerp(aSca, bSca, t0);

	Vector3 cPos = c.Interpolate(c.mPositions, cTime);
	Quaternion cRot = c.Interpolate(c.mRotations, cTime);
	Vector3 cSca = c.Interpolate(c.mScales, cTime);
	Vector3 dPos = d.Interpolate(d.mPositions, dTime);
	Quaternion dRot = d.Interpolate(d.mRotations, dTime);
	Vector3 dSca = d.Interpolate(d.mScales, dTime);

	Vector3 cdPos = Vector3::Lerp(cPos, dPos, t1);
	Quaternion cdRot = Quaternion::Slerp(cRot, dRot, t1);
	Vector3 cdSca = Vector3::Lerp(cSca, dSca, t1);

	Vector3 pos = Vector3::Lerp(abPos, cdPos, t2);
	Quaternion rot = Quaternion::Slerp(abRot, cdRot, t2);
	Vector3 sca = Vector3::Lerp(abSca, cdSca, t2);

	Matrix4x4 tMat = Matrix4x4::Translate(pos);
	Matrix4x4 rMat = rot.ToMatrix();
	Matrix4x4 sMat = Matrix4x4::Scale(sca);	
	return sMat * rMat * tMat;
}

InterpolationBone Bone::InterpolateBone(const Bone& a, float aTime)
{
	InterpolationBone result;
	result.Position = a.Interpolate(a.mPositions, aTime);
	result.Rotation = a.Interpolate(a.mRotations, aTime);
	result.Scale = a.Interpolate(a.mScales, aTime);	
	return result;
}

InterpolationBone Bone::InterpolateBone(
	const Bone& a, const Bone& b,
	float aTime, float bTime,
	float t)
{
	Vector3 aPos = a.Interpolate(a.mPositions, aTime);
	Quaternion aRot = a.Interpolate(a.mRotations, aTime);
	Vector3 aSca = a.Interpolate(a.mScales, aTime);
	Vector3 bPos = b.Interpolate(b.mPositions, bTime);
	Quaternion bRot = b.Interpolate(b.mRotations, bTime);
	Vector3 bSca = b.Interpolate(b.mScales, bTime);

	InterpolationBone result;
	result.Position = Vector3::Lerp(aPos, bPos, t);
	result.Rotation = Quaternion::Slerp(aRot, bRot, t);
	result.Scale = Vector3::Lerp(aSca, bSca, t);	
	return result;
}

InterpolationBone Bone::InterpolateBone(const Bone& a, const Bone& b, const Bone& c, const Bone& d,
	float aTime, float bTime, float cTime, float dTime,
	float t0, float t1, float t2)
{
	Vector3 aPos = a.Interpolate(a.mPositions, aTime);
	Quaternion aRot = a.Interpolate(a.mRotations, aTime);
	Vector3 aSca = a.Interpolate(a.mScales, aTime);
	Vector3 bPos = b.Interpolate(b.mPositions, bTime);
	Quaternion bRot = b.Interpolate(b.mRotations, bTime);
	Vector3 bSca = b.Interpolate(b.mScales, bTime);

	Vector3 abPos = Vector3::Lerp(aPos, bPos, t0);
	Quaternion abRot = Quaternion::Slerp(aRot, bRot, t0);
	Vector3 abSca = Vector3::Lerp(aSca, bSca, t0);

	Vector3 cPos = c.Interpolate(c.mPositions, cTime);
	Quaternion cRot = c.Interpolate(c.mRotations, cTime);
	Vector3 cSca = c.Interpolate(c.mScales, cTime);
	Vector3 dPos = d.Interpolate(d.mPositions, dTime);
	Quaternion dRot = d.Interpolate(d.mRotations, dTime);
	Vector3 dSca = d.Interpolate(d.mScales, dTime);

	Vector3 cdPos = Vector3::Lerp(cPos, dPos, t1);
	Quaternion cdRot = Quaternion::Slerp(cRot, dRot, t1);
	Vector3 cdSca = Vector3::Lerp(cSca, dSca, t1);

	InterpolationBone result;
	result.Position = Vector3::Lerp(abPos, cdPos, t2);
	result.Rotation = Quaternion::Slerp(abRot, cdRot, t2);
	result.Scale = Vector3::Lerp(abSca, cdSca, t2);
	return result;
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