#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "ServiceProvider.h"
#include <unordered_map>
#include <vector>

enum class EventType
{
	WindowResizeEvent,

	Count
};

struct Event 
{
	EventType Type;
};

struct WindowResizeEvent : public Event
{
	int Width;
	int Height;
};

class EventListener
{
public:
	virtual ~EventListener() = default;
	virtual void OnEvent(const Event& event) = 0;
};

class EventBus : public IService<EventBus>
{
public:
	EventBus();
	~EventBus();

	bool IsPersistance() override 
	{
		return false;
	};

	void AddListener(EventType type, EventListener* listener);
	void RemoveListener(EventType type, EventListener* listener);
	void RiseEvent(EventType type, Event& event) const;
private:
	std::unordered_map<EventType, std::vector<EventListener*>> mListenersRegistry;
};

#endif