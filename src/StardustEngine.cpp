#include "StardustEngine.h"
#include "Platform.h"
#include "Renderer.h"


StardustEngine::StardustEngine(const Config& config)
{
	mPlatform = Platform::Create(config);
	mRenderer = Renderer::Create(config, mPlatform);
}

StardustEngine::~StardustEngine()
{
	delete mRenderer;
	delete mPlatform;
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