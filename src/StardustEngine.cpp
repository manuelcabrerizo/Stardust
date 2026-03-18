#include "StardustEngine.h"

#include "ServiceProvider.h"
#include "EventBus.h"
#include "Platform.h"
#include "Renderer.h"


StardustEngine::StardustEngine(const Config& config)
{
	ServiceProvider::Instance()->AddService<EventBus>(new EventBus());

	mPlatform = Platform::Create(config);
	mRenderer = Renderer::Create(config, mPlatform);
}

StardustEngine::~StardustEngine()
{
	delete mRenderer;
	delete mPlatform;

	ServiceProvider::Instance()->RemoveService<EventBus>();
}

void StardustEngine::Run()
{
	OnInit();
	OnLateInit();

	double lastTime = mPlatform->GetTime();

	while(!mPlatform->ShouldClose())
	{
		double currentTime = mPlatform->GetTime();
		float deltaTime = static_cast<float>(currentTime - lastTime);
		lastTime = currentTime;
		if(deltaTime > 0.033f)
		{
			deltaTime = 0.033f;
		}

		mPlatform->ProcessEvents();
		if(!mPlatform->IsPaused())
		{
			OnTick(deltaTime);
		}
	}
	OnDestroy();
}