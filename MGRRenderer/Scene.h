#pragma once
#include "Node.h"
#include <vector>

namespace mgrrenderer
{

class Scene :
	public Node
{
public:
	Scene();
	~Scene();
	
	void pushNode(Node* node);
	void update();

private:
	std::vector<Node*> _children;
};

} // namespace mgrrenderer
