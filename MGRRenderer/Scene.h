#pragma once
#include "Node.h"
#include "Camera.h"
#include <vector>

namespace mgrrenderer
{

class Scene final :
	public Node
{
public:
	~Scene();
	void init();
	void pushNode(Node* node);
	// TODO:本来はdtはスケジューラに渡せばいいのだが、今は各ノードでupdateメソッドでアニメーションをやってるのでdtをvisitとupdateに渡している
	void update(float dt);
	Camera& getCamera() { return _camera; }
	void setCamera(const Camera& camera) { _camera = camera; } // まだ大してサイズ大きくないのでPODとして扱う

private:
	std::vector<Node*> _children;
	Camera _camera;
};

} // namespace mgrrenderer
