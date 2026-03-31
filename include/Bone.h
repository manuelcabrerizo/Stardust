#ifndef BONE_H
#define BONE_H

#include <string>
#include <vector>

#include "Config.h"

#include "math/Vector3.h"
#include "math/Matrix4x4.h"
#include "math/Quaternion.h"

struct KeyVector3
{
	Vector3 Value;
	float TimeStamp;
};

struct KeyQuaternion
{
	Quaternion Value;
	float TimeStamp;
};

struct aiNodeAnim;

class SD_API Bone
{
public:
	Bone(aiNodeAnim* channel);
	~Bone();
	const std::string& GetName() const;
	Matrix4x4 GetMatrix(float animationTime) const;

	static Matrix4x4 Interpolate(const Bone& a, const Bone& b, float aTime, float bTime, float t);
	static Matrix4x4 Interpolate(
		const Bone& a, const Bone& b, const Bone& c, const Bone& d,
		float aTime, float bTime, float cTime, float dTime,
		float t0, float t1, float t2);


private:
	template<typename T>
	int GetIndex(const std::vector<T>& array, float animationTime) const
	{
	    for(int i = 0; i < array.size() - 1; ++i) 
	    {
	        if(animationTime < array[i + 1].TimeStamp)
	        {
	            return i;
	        }
	    }
	    return array.size() - 2;
	}
	Vector3 Interpolate(const std::vector<KeyVector3>& array, float animationTime) const;
	Quaternion Interpolate(const std::vector<KeyQuaternion>& array, float animationTime) const;

	std::string mName;
	std::vector<KeyVector3> mPositions;
	std::vector<KeyVector3> mScales;
	std::vector<KeyQuaternion> mRotations;
};

#endif