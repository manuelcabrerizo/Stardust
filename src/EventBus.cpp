#include "EventBus.h"

EventBus::EventBus()
{

}

EventBus::~EventBus()
{

}

void EventBus::AddListener(EventType type, EventListener* listener)
{
	if(mListenersRegistry.find(type) == mListenersRegistry.end())
	{
		mListenersRegistry.emplace(type, std::vector<EventListener*>{});
	}
	mListenersRegistry.at(type).push_back(listener);
}

void EventBus::RemoveListener(EventType type, EventListener* listener)
{
	if(mListenersRegistry.find(type) != mListenersRegistry.end())
	{
		auto& listeners = mListenersRegistry.at(type);
		const auto toRemove = std::remove(listeners.begin(), listeners.end(), listener);
		listeners.erase(toRemove, listeners.end());
	}
}

void EventBus::RiseEvent(EventType type, Event& event) const
{
	event.Type = type;
	if(mListenersRegistry.find(type) != mListenersRegistry.end())
	{
		for(EventListener* listener : mListenersRegistry.at(type))
		{
			listener->OnEvent(event);
		}
	}
}