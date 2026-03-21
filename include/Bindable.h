#ifndef BINDABLE_H
#define BINDABLE_H

#include "Renderer.h"

class ResourceIdentifier
{
public:
	~ResourceIdentifier() {}
protected:
	ResourceIdentifier() {}
};

class Bindable
{
public:
	Bindable();
	~Bindable();

	ResourceIdentifier* GetIdentifier(Renderer* user) const;
	void Release();
private:
	friend class Renderer;

	void OnLoad(Renderer* user, Renderer::ReleaseFunction onRelease, ResourceIdentifier* id);
	void OnRelease(Renderer* user, ResourceIdentifier* id);

	struct Info
	{
		Renderer* User;
		Renderer::ReleaseFunction Release;
		ResourceIdentifier* ID;
	};
	Info* mInfo;
};

#endif