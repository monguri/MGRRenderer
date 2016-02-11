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

void Scene::update(float dt)
{
	_camera.update(dt);

	for (Node* child : _children)
	{
		child->update(dt);
	}

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//
	// �V���h�E�}�b�v�̕`��
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

	// �ŏI�I�ȕ`��
	_camera.renderWithShadowMap();

	for (Node* child : _children)
	{
		child->renderWithShadowMap();
	}
}

} // namespace mgrrenderer
