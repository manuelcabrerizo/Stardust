#ifndef WIZARD_ENGINE_H
#define WIZARD_ENGINE_H

#include "StardustEngine.h"

#include "EventBus.h"

#include "TextRenderer.h"
#include "GraphicPipeline.h"
#include "VertexBuffer.h"
#include "Texture2D.h"

#include "Model.h"

#include "math/Matrix4x4.h"
#include "math/Vector4.h"
#include "math/Vector3.h"

class WizardEngine : public StardustEngine, EventListener
{
public:
	WizardEngine(const Config& config);
private:
	void OnInit() override;
	void OnLateInit() override;
	void OnTick(float deltaTime) override;
	void OnDestroy() override;

	void OnEvent(const Event& event) override;
	void OnWindowResizeEvent(const WindowResizeEvent& windowResizeEvent);

	GraphicPipeline* m3DGraphicPipeline;
	GraphicPipeline* m2DGraphicPipeline;
	VertexBuffer* mVertexBuffer;
	Texture2D* mTexture0;
	Texture2D* mTexture1;

	Matrix4x4 mView;
	Matrix4x4 mPerspective;
	Matrix4x4 mOrthographic;

	Model* mModel;
	TextRenderer* mText;

	int mFPS;
	int mFrameCounter;
};

#endif