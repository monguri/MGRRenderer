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

	for (Node* child : _children2D)
	{
		delete child;
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

void Scene::pushNode2D(Node* node)
{
	_children2D.push_back(node);
}

void Scene::update(float dt)
{
	_camera.update(dt);

	for (Node* child : _children)
	{
		child->update(dt);
	}

	_cameraFor2D.update(dt);

	for (Node* child : _children2D)
	{
		child->update(dt);
	}

	_camera.prepareRendering();

	for (Node* child : _children)
	{
		child->prepareRendering();
	}

	_cameraFor2D.prepareRendering();

	for (Node* child : _children2D)
	{
		child->prepareRendering();
	}

	//_camera.renderGBuffer();

	// 不透過モデルはディファードレンダリング
	_prepareGBufferRenderingCommand.init([=]
	{
		Director::getRenderer().prepareGBufferRendering();
	});
	Director::getRenderer().addCommand(&_prepareGBufferRenderingCommand);

	for (Node* child : _children)
	{
		child->renderGBuffer();
	}

	//
	// シャドウマップの描画
	//
	for (Light* light : getLight())
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

	//
	// Gバッファとシャドウマップを使った描画
	//
	_prepareDifferedRenderingCommand.init([=]
	{
		Director::getRenderer().prepareDifferedRendering();
	});
	Director::getRenderer().addCommand(&_prepareDifferedRenderingCommand);

	_renderDifferedCommand.init([=]
	{
		Director::getRenderer().renderDiffered();
	});
	Director::getRenderer().addCommand(&_renderDifferedCommand);

	// 非透過モデルはディファードレンダリング
	// 透過モデルはフォワードレンダリング
	_prepareFowardRenderingCommand.init([=]
	{
		Renderer::prepareFowardRendering();
	});
	Director::getRenderer().addCommand(&_prepareFowardRenderingCommand);

	_camera.renderForward();

	//for (Node* child : _children)
	//{
	//	child->renderForward();
	//}

	// 2Dノードは深度の扱いが違うので一つ準備処理をはさむ
	_prepareFowardRendering2DCommand.init([=]
	{
		Renderer::prepareFowardRendering2D();
	});
	Director::getRenderer().addCommand(&_prepareFowardRendering2DCommand);

	_cameraFor2D.renderForward();

	for (Node* child : _children2D)
	{
		child->renderForward();
	}
}

} // namespace mgrrenderer
