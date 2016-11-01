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
	Camera defaultCamera;
	defaultCamera.initAsDefault();
	_camera = defaultCamera;

	_cameraFor2D = defaultCamera;

	// �f�t�H���g�ŃA���r�G���g���C�g����������
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

	// �s���߃��f���̓f�B�t�@�[�h�����_�����O
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
	// �V���h�E�}�b�v�̕`��
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
	// G�o�b�t�@�ƃV���h�E�}�b�v���g�����`��
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

	// �񓧉߃��f���̓f�B�t�@�[�h�����_�����O
	// ���߃��f���̓t�H���[�h�����_�����O
	// TODO:���߃��f���̂݃t�H���[�h�����_�����O����p�X������
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

	// 2D�m�[�h�͐[�x�̈������Ⴄ�̂ň�����������͂���
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