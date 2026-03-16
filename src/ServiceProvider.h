#ifndef SERVICE_PROVIDER_H
#define SERVICE_PROVIDER_H

#include <cassert>
#include <unordered_map>

typedef int ServiceID;

class IServiceBase
{
public:
	virtual ~IServiceBase() = default;
	virtual bool IsPersistance() = 0;
	static ServiceID sCounter;
};

template<typename ServiceType>
class IService : public IServiceBase
{
public:
	static ServiceID ID()
	{
		static ServiceID id = ++sCounter;
		return id;
	}
};

class ServiceProvider
{
public:
	static ServiceProvider* Instance();

	template <typename ServiceType>
	void AddService(IServiceBase* service);
	
	template <typename ServiceType>
	bool RemoveService();

	template <typename ServiceType>
	bool ContainsService();

	template <typename ServiceType>
	ServiceType* GetService();

	void ClearAllServices();
	void ClearAllNonPersitanceServices();
private:
	ServiceProvider() = default;
	static ServiceProvider* sInstance;
	std::unordered_map<ServiceID, IServiceBase*> mServices;
};

template <typename ServiceType>
void ServiceProvider::AddService(IServiceBase* service)
{
	if(mServices.find(ServiceType::ID()) != mServices.end())
	{
		assert(!"Error service already on the map");
	}
	mServices.emplace(ServiceType::ID(), service);
}

template <typename ServiceType>
bool ServiceProvider::RemoveService()
{
	auto it = mServices.find(ServiceType::ID());
	if(it != mServices.end())
	{
		ServiceType *service = reinterpret_cast<ServiceType*>(it->second);
		delete service;
		mServices.erase(it);
		return true;
	}
	else
	{
		return false;
	}
}

template <typename ServiceType>
bool ServiceProvider::ContainsService()
{
	return mServices.find(ServiceType::ID()) != mServices.end();
}

template <typename ServiceType>
ServiceType* ServiceProvider::GetService()
{
	auto it = mServices.find(ServiceType::ID());
	ServiceType *service = reinterpret_cast<ServiceType*>(it->second);
	return service;
}

#endif