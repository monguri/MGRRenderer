#include "Scene.h"
#include "Director.h"
#include "Light.h"

namespace mgrrenderer
{

Scene::~Scene()
{
	for (Light* light : _light)
	{
		delete light;
	}

	for (Node* child : _children)
	{
		delete child;
	}
}

void Scene::init()
{
	Camera defaultCamera;
	defaultCamera.initAsDefault();
	_camera = defaultCamera;

	_cameraFor2D = defaultCamera;

	// デフォルトでアンビエントライトを持たせる
	AmbientLight* defaultLight = new (std::nothrow) AmbientLight(Color3B::WHITE);
	addLight(defaultLight);
}

void Scene::addLight(Light* light)
{
	_light.push_back(light);
}

void Scene::pushNode(Node* node)
{
	_children.push_back(node);
}

void Scene::update(float dt)
{
	_camera.update(dt);

	for (Node* child : _children)
	{
		child->update(dt);
	}

	//
	// シャドウマップの描画
	//
	for (Light* light : Director::getLight())
	{
		if (light->hasShadowMap())
		{
			light->beginRenderShadowMap();
		}
	}

	_camera.renderShadowMap();

	for (Node* child : _children)
	{
		child->renderShadowMap();
	}

	for (Light* light : Director::getLight())
	{
		if (light->hasShadowMap())
		{
			light->endRenderShadowMap();
		}
	}

	_clearCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		// TODO:これ、毎フレームやる必要あるのか？
		float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};

		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
		direct3dContext->ClearRenderTargetView(Director::getInstance()->getDirect3dRenderTarget(), clearColor);
		direct3dContext->ClearDepthStencilView(Director::getInstance()->getDirect3dDepthStencil(), D3D11_CLEAR_DEPTH, 1.0f, 0.0f);

#elif defined(MGRRENDERER_USE_OPENGL)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
	});
	Director::getRenderer().addCommand(&_clearCommand);

	// 最終的な描画
	_camera.renderWithShadowMap();

	for (Node* child : _children)
	{
		child->renderWithShadowMap();
	}
}

} // namespace mgrrenderer
