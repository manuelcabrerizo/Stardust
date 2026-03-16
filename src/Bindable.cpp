#include "Bindable.h"

Bindable::Bindable()
{
	mInfo = nullptr;
}

Bindable::~Bindable()
{
}

ResourceIdentifier* Bindable::GetIdentifier(Renderer* user) const
{
	if(mInfo && mInfo->User == user)
	{
		return mInfo->ID;
	}
	return nullptr;
}

void Bindable::Release()
{
	if(mInfo)
	{
		(mInfo->User->*mInfo->Release)(this);
	}
}

void Bindable::OnLoad(Renderer* user, Renderer::ReleaseFunction onRelease, ResourceIdentifier* id)
{
	if(mInfo == nullptr)
	{
		mInfo = new Info();
		mInfo->User = user;
		mInfo->Release = onRelease;
		mInfo->ID = id;
	}
}

void Bindable::OnRelease(Renderer* user, ResourceIdentifier* id)
{
	if(mInfo)
	{
		delete mInfo;
		mInfo = nullptr;
	}
}