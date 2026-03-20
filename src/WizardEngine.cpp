#include "WizardEngine.h"
#include "Input.h"

WizardEngine::WizardEngine(const Config& config)
	: StardustEngine(config)
{
	mFPS = 0;
	mFrameCounter = 0;
}

void WizardEngine::OnInit()
{
	GetEventBus()->AddListener(EventType::WindowResizeEvent, this);

#if SD_D3D11
	const char* vertexShaderFilepath = "assets/shaders/vert.dxbc";
	const char* vertex2DShaderFilepath = "assets/shaders/vert2d.dxbc";
	const char* pixelShaderFilepath = "assets/shaders/frag.dxbc";
#elif SD_VULKAN
	const char* vertexShaderFilepath = "assets/shaders/vert.spv";
	const char* vertex2DShaderFilepath = "assets/shaders/vert2d.spv";
	const char* pixelShaderFilepath = "assets/shaders/frag.spv";
#endif

	m3DGraphicPipeline = new GraphicPipeline(vertexShaderFilepath, pixelShaderFilepath);
	m2DGraphicPipeline = new GraphicPipeline(vertex2DShaderFilepath, pixelShaderFilepath);
	mModel = new Model("assets/models/viking_room.obj", "assets/textures/viking_room.png");
	mText = new TextRenderer(mRenderer, "assets/fonts/atlas.rtpa");

	mWindowWidth = 1280.0f;
	mWindowHeight = 720.0f;
	float aspect = mWindowWidth / mWindowHeight;
	mPerspective = Matrix4x4::Perspective(45.0f*(SD_PI/180.0f), aspect, 0.05f, 200.0f);
	mOrthographic = Matrix4x4::Orthographic(-mWindowWidth*0.5f, mWindowWidth*0.5f, -mWindowHeight*0.5f, mWindowHeight*0.5f, 0, 10000);
	mView = Matrix4x4::LookAt(Vector3(0.0f, 2.0f, -2.0f), Vector3::ZeroVector, Vector3::YAxisVector);
}

void WizardEngine::OnLateInit()
{
	mRenderer->LoadGraphicPipeline(m3DGraphicPipeline);
	mRenderer->LoadGraphicPipeline(m2DGraphicPipeline);
	mModel->Load(mRenderer);
}

void WizardEngine::OnTick(float deltaTime)
{
	static float time = 0;
	time += deltaTime;

	mFrameCounter++;
	if(time >= 1.0f)
	{
		mFPS = mFrameCounter;
		mFrameCounter = 0;
		time -= 1.0f;
	}

	static float angle = 0;
	if(GetInput()->KeyDown(KEY_W))
	{
		angle += deltaTime;
		if(angle >= SD_PI*2.0f)
		{
			angle -= SD_PI*2.0f;
		}
	}

	mRenderer->BeginFrame(0.2f, 0.2f, 0.4f);

	mRenderer->SetPerFrameVariable<Matrix4x4>("View", mView);
	mRenderer->SetPerFrameVariable<Matrix4x4>("Perspective", mPerspective);
	mRenderer->SetPerFrameVariable<Matrix4x4>("Orthographic", mOrthographic);
	mRenderer->SetPerFrameVariable<float>("Time", 69.0f);
	mRenderer->PushPerFrameVariables();

	// 3D Rendering
	mRenderer->PushGraphicPipeline(m3DGraphicPipeline);

	mRenderer->SetPerDrawVariable<Matrix4x4>("World", Matrix4x4::RotateX(-SD_PI*0.5f) * Matrix4x4::RotateY(angle) * Matrix4x4::Translate(0.0f,  -0.25f, 0.0f));
	mRenderer->SetPerDrawVariable<Vector3>("Tint", Vector3{1.0f, 1.0f, 1.0f});
	mRenderer->PushPerDrawVariables();
	mModel->Draw(mRenderer);
	

	// 2D Batch Rendering
	mRenderer->PushGraphicPipeline(m2DGraphicPipeline);

	char buffer[256];
	sprintf(buffer, "FPS: %d", mFPS);
	Vector3 origin = Vector3(-mWindowWidth*0.5f, mWindowHeight*0.5f - 64, 0);
	mText->DrawString(buffer, origin, 1, Vector3(1,0,0));
	mText->DrawString("Hello Sailor!", origin + Vector3(0.0f, -64.0f, 0.0f), 1, Vector3(0,1,0));
	mText->DrawString("This is Wow Killer!", origin + Vector3(0.0f, -128.0f, 0.0f), 1, Vector3(1,1,0));
	mText->Present(mRenderer);


	mRenderer->EndFrame();
}

void WizardEngine::OnDestroy()
{
	delete mText;
	delete mModel;
	delete m2DGraphicPipeline;
	delete m3DGraphicPipeline;

	GetEventBus()->RemoveListener(EventType::WindowResizeEvent, this);
}

void WizardEngine::OnEvent(const Event& event)
{
	switch(event.Type)
	{
		case EventType::WindowResizeEvent:
		{
			OnWindowResizeEvent(reinterpret_cast<const WindowResizeEvent&>(event));
		}break;
		default:
		{
			assert(!"ERROR!");
		}
	};
}

void WizardEngine::OnWindowResizeEvent(const WindowResizeEvent& windowResizeEvent)
{
	mWindowWidth = windowResizeEvent.Width;
	mWindowHeight = windowResizeEvent.Height;
	float aspect = (float)windowResizeEvent.Width / (float)windowResizeEvent.Height;
	mPerspective = Matrix4x4::Perspective(45.0f*(SD_PI/180.0f), aspect, 0.05f, 200.0f);
	mOrthographic = Matrix4x4::Orthographic(
		(float)windowResizeEvent.Width*-0.5f, (float)windowResizeEvent.Width*0.5f, 
		(float)windowResizeEvent.Height*-0.5f, (float)windowResizeEvent.Height*0.5f, 
		0, 10000);
}