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
	// TODO:�{����dt�̓X�P�W���[���ɓn���΂����̂����A���͊e�m�[�h��update���\�b�h�ŃA�j���[�V����������Ă�̂�dt��visit��update�ɓn���Ă���
	void update(float dt);
	Camera& getCamera() { return _camera; }
	const Camera& getCameraFor2D() { return _cameraFor2D; } // 2D�p�̌Œ�J�����Ȃ̂ŊO����C�������Ȃ�
	void setCamera(const Camera& camera) { _camera = camera; } // �܂��債�ăT�C�Y�傫���Ȃ��̂�POD�Ƃ��Ĉ���
	// TODO:���ڐG��̂łȂ��C�e���[�^�Ƃ��g���ĉB�؂�������
	const std::vector<Light*>& getLight() const {return _light;}
	void addLight(Light* light);
	Light* getDefaultLight() { return _light[0]; }// ���C�g���폜����I/F�͗p�ӂ��ĂȂ��̂ŁA0�Ԗڂ�AmbientLight���K���f�t�H���g���C�g�ɂȂ�

private:
	std::vector<Node*> _children;
	Camera _camera;
	Camera _cameraFor2D; // 2D�悤��Size(0,0,WINDOW_WIDTH,WINDOW_HEIGHT)����ʂɓ���悤�ɌŒ肵���J����
	std::vector<Light*> _light;
	CustomRenderCommand _clearCommand;
};

} // namespace mgrrenderer
