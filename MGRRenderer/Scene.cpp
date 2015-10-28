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

void Scene::pushNode(Node* node)
{
	_children.push_back(node);
}

void Scene::update()
{
	glClear(GL_COLOR_BUFFER_BIT);

	for (Node* child : _children)
	{
		child->visit();
	}
}

} // namespace mgrrenderer
