#include "StardustEngine.h"
#include "ServiceProvider.h"
#include "EventBus.h"
#include "Input.h"
#include "Platform.h"
#include "Renderer.h"

StardustEngine::StardustEngine(const Config& config)
{
	ServiceProvider::Instance()->AddService<EventBus>(new EventBus());
	ServiceProvider::Instance()->AddService<Input>(new Input());

	mPlatform = Platform::Create(config);
	mRenderer = Renderer::Create(config, mPlatform);
}

StardustEngine::~StardustEngine()
{
	delete mRenderer;
	delete mPlatform;

	ServiceProvider::Instance()->RemoveService<Input>();
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

		GetInput()->Process();
		mPlatform->ProcessEvents();
		if(!mPlatform->IsPaused())
		{
			OnTick(deltaTime);
		}
	}
	OnDestroy();
}