#include "ServiceProvider.h"

ServiceID IServiceBase::sCounter = 0;

ServiceProvider* ServiceProvider::sInstance = nullptr;

ServiceProvider* ServiceProvider::Instance()
{
	if(sInstance == nullptr)
	{
		sInstance = new ServiceProvider();
	}
	return sInstance;
}

void ServiceProvider::ClearAllServices()
{
	mServices.clear();
}

void ServiceProvider::ClearAllNonPersitanceServices()
{
	// TODO: ...
}