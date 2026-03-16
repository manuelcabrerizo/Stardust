#ifndef STARDUST_ENGINE_H
#define STARDUST_ENGINE_H

#include "Config.h"

class Platform;
class Renderer;

class StardustEngine
{
public:
	StardustEngine(const Config& config);
	virtual ~StardustEngine();
	void Run();

	StardustEngine(const StardustEngine&) = delete;
	StardustEngine& operator=(const StardustEngine&) = delete;
private:
	virtual void OnInit() = 0;
	virtual void OnLateInit() = 0;
	virtual void OnTick() = 0;
	virtual void OnDestroy() = 0;
protected:
	Platform* mPlatform;
	Renderer* mRenderer;
};

#endif