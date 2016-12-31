#include "Scene.h"
#include "renderer/Director.h"
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
	_camera.initAsDefault3D();
	_cameraFor2D.initAsDefault2D();

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
		switch (light->getLightType()) {
		case LightType::AMBIENT:
			// 何もしない
			break;
		case LightType::DIRECTION:
		{
			DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
			if (dirLight->hasShadowMap())
			{
				dirLight->prepareShadowMapRendering();
			}

			for (Node* child : _children)
			{
				child->renderShadowMap();
			}
		}
			break;
		case LightType::POINT:
		{
			PointLight* pointLight = static_cast<PointLight*>(light);
#if defined(MGRRENDERER_USE_DIRECT3D)
			if (pointLight->hasShadowMap())
			{
				pointLight->prepareShadowMapRendering();
			}

			for (Node* child : _children)
			{
				child->renderShadowMap();
			}
#elif defined(MGRRENDERER_USE_OPENGL)
			if (pointLight->hasShadowMap())
			{
				for (int i = (int)CubeMapFace::X_POSITIVE; i < (int)CubeMapFace::NUM_CUBEMAP_FACE; i++)
				{
					pointLight->prepareShadowMapRendering((CubeMapFace)i);

					for (Node* child : _children)
					{
						child->renderShadowMap((CubeMapFace)i);
					}
				}
			}
#endif
		}
			break;
		case LightType::SPOT:
		{
			SpotLight* spotLight = static_cast<SpotLight*>(light);
			if (spotLight->hasShadowMap())
			{
				spotLight->prepareShadowMapRendering();
			}

			for (Node* child : _children)
			{
				child->renderShadowMap();
			}
		}
			break;
		default:
			break;
		}
	}

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

	// 非透過モデルはディファードレンダリング
	// 透過モデルはフォワードレンダリング
	// TODO:透過モデルのみフォワードレンダリングするパスを入れる
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
