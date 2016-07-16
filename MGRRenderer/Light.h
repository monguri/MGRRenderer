#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "GroupBeginRenderCommand.h"
#include "GroupEndRenderCommand.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include <d3d11.h>
#endif

namespace mgrrenderer
{

enum class LightType : int
{
	NONE = -1,

	AMBIENT,
	DIRECTION,
	POINT,
	SPOT,

	NUM_LIGHT_TYPE,
};

class Light :
	public Node
{
public:
	// 直接Lightのインスタンスを作ることはできない
#if defined(MGRRENDERER_USE_DIRECT3D)
	virtual const void* getConstantBufferDataPointer() const = 0;
#endif
	virtual LightType getLightType() const = 0;
	float getIntensity() const { return _intensity; }
	virtual void setIntensity(float intensity) { _intensity = intensity; }
	virtual bool hasShadowMap() const = 0;
	virtual void beginRenderShadowMap() = 0;
	virtual void endRenderShadowMap() = 0;

protected:
	Light();

private:
	float _intensity;
};

class AmbientLight :
	public Light
{
public:
#if defined(MGRRENDERER_USE_DIRECT3D)
	struct ConstantBufferData
	{
		Color4F color;
	};
#endif

	AmbientLight(const Color3B& color);

	LightType getLightType() const override { return LightType::AMBIENT; };
#if defined(MGRRENDERER_USE_DIRECT3D)
	const void* getConstantBufferDataPointer() const { return &_constantBufferData; }
#endif
	void setColor(const Color3B& color) override;
	void setIntensity(float intensity) override;
	bool hasShadowMap() const override { return false; } // シャドウマップはアンビエントライトには使えない
	void beginRenderShadowMap() override {};
	void endRenderShadowMap() override {};

private:
#if defined(MGRRENDERER_USE_DIRECT3D)
	ConstantBufferData _constantBufferData;
#endif
};

class DirectionalLight :
	public Light
{
public:
	struct ShadowMapData
	{
		Mat4 viewMatrix;
		Mat4 projectionMatrix;
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11Texture2D* depthTexture;
		ID3D11DepthStencilView* depthTextureDepthStencilView;
		ID3D11ShaderResourceView* depthTextureShaderResourceView;
		ID3D11SamplerState* depthTextureSamplerState;
#elif defined(MGRRENDERER_USE_OPENGL)
		GLuint frameBufferId;
		GLuint textureId;
#endif

#if defined(MGRRENDERER_USE_OPENGL)
		ShadowMapData() : frameBufferId(0), textureId(0) {};
#endif
	};

#if defined(MGRRENDERER_USE_DIRECT3D)
	struct ConstantBufferData
	{
		Vec3 direction;
		float hasShadowMap; // ConstantBufferのパッキングのためにfloatにしているが、0.0fか1.0fを入れてboolの代わりに使う
		Color4F color;
	};
#endif

	DirectionalLight(const Vec3& direction, const Color3B& color);
	~DirectionalLight();

	LightType getLightType() const override { return LightType::DIRECTION; };
	const Vec3& getDirection() const { return _direction; }
	const ShadowMapData& getShadowMapData() const { return _shadowMapData; }
#if defined(MGRRENDERER_USE_DIRECT3D)
	const void* getConstantBufferDataPointer() const { return &_constantBufferData; }
#endif
	void setColor(const Color3B& color) override;
	void setIntensity(float intensity) override;
	void prepareShadowMap(const Vec3& targetPosition, float cameraDistanceFromTarget, const Size& size);
	bool hasShadowMap() const override { return _constantBufferData.hasShadowMap > 0.0f; }
	void beginRenderShadowMap() override;
	void endRenderShadowMap() override;

private:
	Vec3 _direction;
#if defined(MGRRENDERER_USE_DIRECT3D)
	ConstantBufferData _constantBufferData;
#endif
	ShadowMapData _shadowMapData;
	GroupBeginRenderCommand _beginRenderCommand;
	GroupEndRenderCommand _endRenderCommand;
};

class PointLight :
	public Light
{
public:
#if defined(MGRRENDERER_USE_DIRECT3D)
	struct ConstantBufferData
	{
		Color4F color;
		Vec3 position;
		float rangeInverse;
	};
#endif

	PointLight(const Vec3& position, const Color3B& color, float range);

	LightType getLightType() const override { return LightType::POINT; };
	float getRange() const { return _range; }
#if defined(MGRRENDERER_USE_DIRECT3D)
	const void* getConstantBufferDataPointer() const { return &_constantBufferData; }
#endif
	void setColor(const Color3B& color) override;
	void setIntensity(float intensity) override;
	bool hasShadowMap() const override { return false; } // シャドウマップはポイントライトには使えない
	void beginRenderShadowMap() override {};
	void endRenderShadowMap() override {};

private:
	// 減衰率計算に用いる、光の届く範囲　正しい物理計算だと無限遠まで届くが、そうでないモデルをcocosが使ってるのでそれを採用
	float _range;
#if defined(MGRRENDERER_USE_DIRECT3D)
	ConstantBufferData _constantBufferData;
#endif
};

class SpotLight :
	public Light
{
public:
#if defined(MGRRENDERER_USE_DIRECT3D)
	struct ConstantBufferData
	{
		Vec3 position;
		float rangeInverse;
		Color3F color;
		float innerAngleCos;
		Vec3 direction;
		float outerAngleCos;
	};
#endif

	SpotLight(const Vec3& position, const Vec3& direction, const Color3B& color, float range, float innerAngle, float outerAngle);

	LightType getLightType() const override { return LightType::SPOT; };
	const Vec3& getDirection() const { return _direction; }
	float getRange() const { return _range; }
	float getInnerAngleCos() const { return _innerAngleCos; }
	float getOuterAngleCos() const { return _outerAngleCos; }
#if defined(MGRRENDERER_USE_DIRECT3D)
	const void* getConstantBufferDataPointer() const { return &_constantBufferData; }
#endif
	void setColor(const Color3B& color) override;
	void setIntensity(float intensity) override;
	bool hasShadowMap() const override { return false; } // シャドウマップはスポットライトには使えない
	void beginRenderShadowMap() override {};
	void endRenderShadowMap() override {};

private:
	Vec3 _direction;
	float _range;
	float _innerAngleCos;
	float _outerAngleCos;
#if defined(MGRRENDERER_USE_DIRECT3D)
	ConstantBufferData _constantBufferData;
#endif
};

} // namespace mgrrenderer
