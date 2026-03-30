#ifndef ANIMATION_H
#define ANIMATION_H


#include "Bone.h"

#include <string>
#include <unordered_map>

class SD_API Animation
{
public:
	Animation(const char* animationFilepath);
	~Animation();
	float GetDuration() const;
	int GetTicksPerSecond() const;
	const Bone& GetBone(const std::string& name) const;
	bool ContainsBone(const std::string& name) const;
private:
	float mDuration;
	int mTicksPerSecond;
	std::unordered_map<std::string, Bone> mBones;
};

#endif