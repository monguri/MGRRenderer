#include "Scene.h"
#include "renderer/Director.h"
#include "Light.h"

namespace mgrrenderer
{

Scene::Scene() :
_ambientLight(nullptr),
_directionalLight(nullptr),
_numPointLight(0),
_numSpotLight(0)
{}

Scene::~Scene()
{
	if (_ambientLight != nullptr)
	{
		delete _ambientLight;
		_ambientLight = nullptr;
	}

	if (_directionalLight != nullptr)
	{
		delete _directionalLight;
		_directionalLight = nullptr;
	}

	for (const PointLight* light : _pointLightList)
	{
		delete light;
	}

	for (const SpotLight* light : _spotLightList)
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
	_camera.initAsDefault3D();
	_cameraFor2D.initAsDefault2D();

	for (size_t i = 0; i < PointLight::MAX_NUM; i++)
	{
		_pointLightList[i] = nullptr;
	}

	for (size_t i = 0; i < SpotLight::MAX_NUM; i++)
	{
		_spotLightList[i] = nullptr;
	}

	// デフォルトでアンビエントライトを持たせる
	_ambientLight = new (std::nothrow) AmbientLight(Color3B::WHITE);
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

#if defined(MGRRENDERER_DEFERRED_RENDERING)
	//_camera.renderGBuffer();

	// 不透過モデルはディファードレンダリング
	_prepareGBufferRenderingCommand.init([=]
	{
		Director::getRenderer().prepareGBufferRendering();
	});
	Director::getRenderer().addCommand(&_prepareGBufferRenderingCommand);

	for (Node* child : _children)
	{
		if (!child->getIsTransparent())
		{
			child->renderGBuffer();
		}
	}
#endif

	//
	// シャドウマップの描画
	//
	if (_directionalLight != nullptr && _directionalLight->hasShadowMap())
	{
		_directionalLight->prepareShadowMapRendering();

		for (Node* child : _children)
		{
			if (!child->getIsTransparent())
			{
				child->renderDirectionalLightShadowMap(_directionalLight);
			}
		}
	}

	for (size_t i = 0; i < PointLight::MAX_NUM; i++)
	{
		PointLight* pointLight = _pointLightList[i];
		if (pointLight == nullptr || !pointLight->hasShadowMap())
		{
			continue;
		}

#if defined(MGRRENDERER_USE_DIRECT3D)
		pointLight->prepareShadowMapRendering();

		for (Node* child : _children)
		{
			if (!child->getIsTransparent())
			{
				child->renderPointLightShadowMap(i, pointLight);
			}
		}
#elif defined(MGRRENDERER_USE_OPENGL)
		for (int face = (int)CubeMapFace::X_POSITIVE; face < (int)CubeMapFace::NUM_CUBEMAP_FACE; face++)
		{
			pointLight->prepareShadowMapRendering((CubeMapFace)face);

			for (Node* child : _children)
			{
				if (!child->getIsTransparent())
				{
					child->renderPointLightShadowMap(i, pointLight, (CubeMapFace)face);
				}
			}
		}
#endif
	}

	for (size_t i = 0; i < SpotLight::MAX_NUM; i++)
	{
		SpotLight* spotLight = _spotLightList[i];
		if (spotLight == nullptr || !spotLight->hasShadowMap())
		{
			continue;
		}

		spotLight->prepareShadowMapRendering();

		for (Node* child : _children)
		{
			if (!child->getIsTransparent())
			{
				child->renderSpotLightShadowMap(i, spotLight);
			}
		}
	}

#if defined(MGRRENDERER_DEFERRED_RENDERING)
	//
	// Gバッファとシャドウマップを使った描画
	//
	_prepareDeferredRenderingCommand.init([=]
	{
		Director::getRenderer().prepareDeferredRendering();
	});
	Director::getRenderer().addCommand(&_prepareDeferredRenderingCommand);

	_renderDeferredCommand.init([=]
	{
		Director::getRenderer().renderDeferred();
	});
	Director::getRenderer().addCommand(&_renderDeferredCommand);
#elif defined(MGRRENDERER_FOWARD_RENDERING)
	_prepareFowardRenderingCommand.init([=]
	{
		Director::getRenderer().prepareFowardRendering();
	});
	Director::getRenderer().addCommand(&_prepareFowardRenderingCommand);

	_camera.renderForward();
	for (Node* child : _children)
	{
		if (!child->getIsTransparent())
		{
			child->renderForward();
		}
	}
#endif
	_prepareTransparentRenderingCommand.init([=]
	{
		Director::getRenderer().prepareTransparentRendering();
	});
	Director::getRenderer().addCommand(&_prepareTransparentRenderingCommand);

	// 透過モデルパス
	std::map<float, Node*> transparentNodes; // キーはカメラからの2乗距離。mapは格納時に2分木としてソートされているのを利用する

	for (Node* child : _children)
	{
		// 透過物同士
		if (child->getIsTransparent())
		{
			float distSqFromCamera = (child->getPosition() - Director::getCamera().getPosition()).lengthSquare();
			transparentNodes[distSqFromCamera] = child;
		}
	}

	for (std::map<float, Node*>::reverse_iterator it = transparentNodes.rbegin(); it != transparentNodes.rend(); ++it)
	{
		it->second->renderForward();
	}

	// 2Dノードは深度の扱いが違うので一つ準備処理をはさむ
	_prepareFowardRendering2DCommand.init([=]
	{
		Director::getRenderer().prepareFowardRendering2D();
	});
	Director::getRenderer().addCommand(&_prepareFowardRendering2DCommand);

	_cameraFor2D.renderForward();

	for (Node* child : _children2D)
	{
		child->renderForward();
	}
}

} // namespace mgrrenderer
