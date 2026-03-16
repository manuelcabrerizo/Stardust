#include "WizardEngine.h"

struct VertexQuad
{
	Vector3 Position;
	Vector2 TCoord;
};

VertexQuad vertices[] =
{
    VertexQuad{ Vector3{-0.5, -0.5, 0}, Vector2{1, 0} },
    VertexQuad{ Vector3{-0.5,  0.5, 0}, Vector2{1, 1} },
    VertexQuad{ Vector3{0.5,  0.5, 0}, Vector2{0, 1} },
    VertexQuad{ Vector3{0.5,  0.5, 0}, Vector2{0, 1} },
    VertexQuad{ Vector3{0.5, -0.5, 0}, Vector2{0, 0} },
    VertexQuad{ Vector3{-0.5, -0.5, 0}, Vector2{1, 0} },
};

WizardEngine::WizardEngine(const Config& config)
	: StardustEngine(config)
{
}

void WizardEngine::OnInit()
{

#if SD_D3D11
	mGraphicPipeline = new GraphicPipeline("assets/shaders/vert.dxbc", "assets/shaders/frag.dxbc");
#elif SD_VULKAN
	mGraphicPipeline = new GraphicPipeline("assets/shaders/vert.spv", "assets/shaders/frag.spv");
#endif

	mVertexBuffer = new VertexBuffer(vertices, 6, sizeof(VertexQuad));
	mTexture0 = new Texture2D("assets/textures/tiles_floor_5.png", false);
	mTexture1 = new Texture2D("assets/textures/tiles_floor_5_normal.png", false);
	mView = Matrix4x4::LookAt(Vector3(0.0f, 0.0f, -2.0f), Vector3::ZeroVector, Vector3::YAxisVector);
	mProj = Matrix4x4::Perspective(60.0f*(SD_PI/180.0f), 800.0f / 600.0f, 0.05f, 200.0f);
}

void WizardEngine::OnLateInit()
{
	mRenderer->LoadGraphicPipeline(mGraphicPipeline);
	mRenderer->LoadVertexBuffer(mVertexBuffer);
	mRenderer->LoadTexture2D(mTexture0);
	mRenderer->LoadTexture2D(mTexture1);
}

void WizardEngine::OnTick()
{
	mRenderer->BeginFrame();

	mRenderer->PushGraphicPipeline(mGraphicPipeline);
	
	mRenderer->SetPerFrameVariable<Matrix4x4>("View", mView);
	mRenderer->SetPerFrameVariable<Matrix4x4>("Proj", mProj);
	mRenderer->SetPerFrameVariable<float>("Time", 69.0f);
	mRenderer->PushPerFrameVariables();

	mRenderer->SetPerDrawVariable<Matrix4x4>("World", Matrix4x4::Translate(-0.75f, -0.25f, 0.0f));
	mRenderer->SetPerDrawVariable<Vector3>("Tint", Vector3{1.0f, 1.0f, 1.0f});
	mRenderer->PushPerDrawVariables();
	mRenderer->PushTexture(mTexture0, 0);
	mRenderer->PushVerteBuffer(mVertexBuffer);

	mRenderer->SetPerDrawVariable<Matrix4x4>("World", Matrix4x4::Translate(0.75f, -0.25f, 0.0f));
	mRenderer->SetPerDrawVariable<Vector3>("Tint", Vector3{1.0f, 1.0f, 1.0f});
	mRenderer->PushPerDrawVariables();
	mRenderer->PushTexture(mTexture1, 0);
	mRenderer->PushVerteBuffer(mVertexBuffer);

	static float angle = 0;
	mRenderer->SetPerDrawVariable<Matrix4x4>("World", Matrix4x4::Translate(0.0f, 0.0f, 0.0f) * Matrix4x4::RotateZ(angle));
	mRenderer->SetPerDrawVariable<Vector3>("Tint", Vector3{0.4f, 2.0f, 0.4f});
	mRenderer->PushPerDrawVariables();
	mRenderer->PushTexture(mTexture0, 0);
	mRenderer->PushVerteBuffer(mVertexBuffer);

	angle += 0.001f;
	if(angle > (SD_PI * 2.0f))
	{
		angle -= SD_PI * 2.0f;
	}

	
	mRenderer->EndFrame();
}

void WizardEngine::OnDestroy()
{
	delete mTexture1;
	delete mTexture0;
	delete mVertexBuffer;
	delete mGraphicPipeline;
}