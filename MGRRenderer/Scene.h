#pragma once
#include "Node.h"
#include "Camera.h"
#include "CustomRenderCommand.h"
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
	const Camera& getCameraFor2D() { return _cameraFor2D; } // 2D用の固定カメラなので外から修正させない
	void setCamera(const Camera& camera) { _camera = camera; } // まだ大してサイズ大きくないのでPODとして扱う
	// TODO:直接触るのでなくイテレータとか使って隠ぺいしたい
	const std::vector<Light*>& getLight() const {return _light;}
	void addLight(Light* light);
	Light* getDefaultLight() { return _light[0]; }// ライトを削除するI/Fは用意してないので、0番目のAmbientLightが必ずデフォルトライトになる

private:
	std::vector<Node*> _children;
	Camera _camera;
	Camera _cameraFor2D; // 2DようにSize(0,0,WINDOW_WIDTH,WINDOW_HEIGHT)が画面に入るように固定したカメラ
	std::vector<Light*> _light;
	CustomRenderCommand _clearCommand;
};

} // namespace mgrrenderer
