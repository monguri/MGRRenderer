#include "Scene.h"

namespace mgrrenderer
{

Scene::~Scene()
{
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
}

void Scene::pushNode(Node* node)
{
	_children.push_back(node);
}

void Scene::update(float dt)
{
	for (Node* child : _children)
	{
		child->visit(dt);
	}
}

} // namespace mgrrenderer
