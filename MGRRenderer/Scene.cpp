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

	_camera.renderGBuffer();

	// 不透過モデルはディファードレンダリング
	_prepareDifferedRenderingCommand.init([=]
	{
		Director::getRenderer().prepareDifferedRendering();
	});
	Director::getRenderer().addCommand(&_prepareDifferedRenderingCommand);

	for (Node* child : _children)
	{
		child->renderGBuffer();
	}

	//
	// シャドウマップの描画
	//
	for (Light* light : Director::getLight())
	{
		if (light->hasShadowMap())
		{
			light->prepareShadowMapRendering();
		}
	}

	for (Node* child : _children)
	{
		child->renderShadowMap();
	}

	// 透過モデルはフォワードレンダリング
	_prepareFowardRenderingCommand.init([=]
	{
		Renderer::prepareFowardRendering();
	});
	Director::getRenderer().addCommand(&_prepareFowardRenderingCommand);

	_camera.renderWithShadowMap();

	for (Node* child : _children)
	{
		child->renderWithShadowMap();
	}
}

} // namespace mgrrenderer
