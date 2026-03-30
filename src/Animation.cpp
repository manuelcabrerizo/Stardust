#include "Animation.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Animation::Animation(const char* animationFilepath)
{
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(animationFilepath, aiProcess_Triangulate);
	aiAnimation *animation = scene->mAnimations[0];
	mDuration = animation->mDuration;
	mTicksPerSecond = static_cast<int>(animation->mTicksPerSecond);
	for(int i = 0; i < animation->mNumChannels; i++)
	{
		std::string name(animation->mChannels[i]->mNodeName.C_Str());
		mBones.emplace(name, Bone{animation->mChannels[i]});
	}
}

Animation::~Animation()
{
	mDuration = 0.0f;
	mTicksPerSecond = 0;
	mBones.clear();
}

float Animation::GetDuration() const
{
	return mDuration;
}

int Animation::GetTicksPerSecond() const
{
	return mTicksPerSecond;
}

const Bone& Animation::GetBone(const std::string& name) const
{
	auto it = mBones.find(name);
	return it->second;
}

bool Animation::ContainsBone(const std::string& name) const
{
	return mBones.find(name) != mBones.end();
}
