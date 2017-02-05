#pragma once
#include "Node.h"
#include "Light.h"
#include "Camera.h"
#include "renderer/CustomRenderCommand.h"
#include <array>
#include <vector>

namespace mgrrenderer
{

class Scene final :
	public Node
{
public:
	Scene();
	~Scene();
	void init();
	void pushNode(Node* node);
	void pushNode2D(Node* node);
	// TODO:本来はdtはスケジューラに渡せばいいのだが、今は各ノードでupdateメソッドでアニメーションをやってるのでdtをvisitとupdateに渡している
	void update(float dt);
	Camera& getCamera() { return _camera; }
	const Camera& getCameraFor2D() { return _cameraFor2D; } // 2D用の固定カメラなので外から修正させない
	void setCamera(const Camera& camera) { _camera = camera; } // まだ大してサイズ大きくないのでPODとして扱う

	AmbientLight* getAmbientLight() const { return _ambientLight; }
	void setAmbientLight(AmbientLight* light) { _ambientLight = light; }
	DirectionalLight* getDirectionalLight() const { return _directionalLight; }
	void setDirectionalLight(DirectionalLight* light) { _directionalLight = light; }
	void addPointLight(PointLight* light)
	{
		_pointLightList[_numPointLight] = light;
		_numPointLight++;
	}
	void addSpotLight(SpotLight* light)
	{
		_spotLightList[_numSpotLight] = light;
		_numSpotLight++;
	}
	size_t getNumPointLight() const { return _numPointLight; }
	PointLight* getPointLight(size_t index) const { return _pointLightList[index]; }
	size_t getNumSpotLight() const { return _numSpotLight; }
	SpotLight* getSpotLight(size_t index) const { return _spotLightList[index]; }

private:
	std::vector<Node*> _children;
	std::vector<Node*> _children2D;
	Camera _camera;
	Camera _cameraFor2D; // 2DようにSize(0,0,WINDOW_WIDTH,WINDOW_HEIGHT)が画面に入るように固定したカメラ

	AmbientLight* _ambientLight;
	DirectionalLight* _directionalLight;
	std::array<PointLight*, PointLight::MAX_NUM> _pointLightList;
	size_t _numPointLight;
	std::array<SpotLight*, SpotLight::MAX_NUM> _spotLightList;
	size_t _numSpotLight;

#if defined(MGRRENDERER_DEFERRED_RENDERING)
	CustomRenderCommand _prepareGBufferRenderingCommand;
	CustomRenderCommand _prepareDeferredRenderingCommand; // Gバッファの描画も含めてディファードレンダリングだが、命名をわかりやすくするためにGバッファを使った最終描画をこう呼んでいる
#elif defined(MGRRENDERER_FOWARD_RENDERING)
	CustomRenderCommand _prepareFowardRenderingCommand;
#endif
	CustomRenderCommand _prepareTransparentRenderingCommand;
	CustomRenderCommand _prepareFowardRendering2DCommand;
	CustomRenderCommand _renderDeferredCommand;
};

} // namespace mgrrenderer
