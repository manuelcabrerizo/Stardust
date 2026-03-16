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
	while(!mPlatform->ShouldClose())
	{
		if(!mPlatform->ProcessEvents())
		{
			OnTick();
		}

	}
	OnDestroy();
}