#pragma once
#include "Node.h"
#include "Camera.h"
#include <vector>

namespace mgrrenderer
{

class Light;

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
	// TODO:直接触るのでなくイテレータとか使って隠ぺいしたい
	const std::vector<Light*>& getLight() const {return _light;}
	void addLight(Light* light);
	Light* getDefaultLight() { return _light[0]; }// ライトを削除するI/Fは用意してないので、0番目のAmbientLightが必ずデフォルトライトになる

private:
	std::vector<Node*> _children;
	Camera _camera;
	std::vector<Light*> _light;
};

} // namespace mgrrenderer
