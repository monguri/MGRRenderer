#include "GBufferPack.hlsl"

cbuffer ModelMatrix : register(b0)
{
	matrix _model;
};

cbuffer ViewMatrix : register(b1)
{
	matrix _view;
};

cbuffer ProjectionMatrix : register(b2)
{
	matrix _projection;
};

cbuffer DepthBiasMatrix : register(b3)
{
	matrix _depthBias;
};

cbuffer NormalMatrix : register(b4)
{
	matrix _normal;
};

cbuffer MultiplyColor : register(b5)
{
	float4 _multiplyColor;
};

cbuffer AmbientLightParameter : register(b6)
{
	float4 _ambientLightColor;
};

cbuffer DirectionalLightViewMatrix : register(b7)
{
	matrix _directionalLightView;
};

cbuffer DirectionalLightProjectionMatrix : register(b8)
{
	matrix _directionalLightProjection;
};

cbuffer DirectionalLightParameter : register(b9)
{
	float3 _directionalLightDirection;
	float _directionalLightHasShadowMap;
	float3 _directionalLightColor;
	float _directionalLightIsValid;
};

static const unsigned int NUM_FACE_CUBEMAP_TEXTURE = 6;
static const unsigned int MAX_NUM_POINT_LIGHT = 4; // ���ӁF�v���O�������ƒ萔�̈�v���K�v

cbuffer PointLightParameter : register(b10)
{
	struct {
		matrix _pointLightView[NUM_FACE_CUBEMAP_TEXTURE];
		matrix _pointLightProjection;
		float3 _pointLightColor;
		float _pointLightHasShadowMap;
		float3 _pointLightPosition;
		float _pointLightRangeInverse;
		float _pointLightIsValid;
		float3 _pointLightPadding;
	} _pointLightParameter[MAX_NUM_POINT_LIGHT];
};

static const unsigned int MAX_NUM_SPOT_LIGHT = 4; // ���ӁF�v���O�������ƒ萔�̈�v���K�v

cbuffer SpotLightParameter : register(b11)
{
	struct {
		matrix _spotLightView;
		matrix _spotLightProjection;
		float3 _spotLightPosition;
		float _spotLightRangeInverse;
		float3 _spotLightColor;
		float _spotLightInnerAngleCos;
		float3 _spotLightDirection;
		float _spotLightOuterAngleCos;
		float _spotLightHasShadowMap;
		float _spotLightIsValid;
		float2 _spotLightPadding;
	} _spotLightParameter[MAX_NUM_SPOT_LIGHT];
};


Texture2D _directionalLightShadowMap : register(t0);
TextureCube<float> _pointLightShadowCubeMap[MAX_NUM_POINT_LIGHT];
Texture2D<float> _spotLightShadowMap[MAX_NUM_SPOT_LIGHT];

SamplerComparisonState _pcfSampler : register(s0);

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 worldPosition : POSITION;
	float4 directionalLightPosition : DIRECTIONAL_LIGHT_POSITION;
	float3 normal : NORMAL;
	float3 vertexToPointLightDirection[MAX_NUM_POINT_LIGHT] : POINT_LIGHT_DIRECTION;
	float4 spotLightPosition[MAX_NUM_SPOT_LIGHT] : SPOT_LIGHT_POSITION;
	float3 vertexToSpotLightDirection[MAX_NUM_SPOT_LIGHT] : SPOT_LIGHT_DIRECTION;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output;

	float4 position = float4(input.position, 1.0);
	output.worldPosition = mul(position, _model);
	position = mul(output.worldPosition, _view);
	output.position = mul(position, _projection);

	float4 directionalLightPosition = mul(output.worldPosition, _directionalLightView);
	directionalLightPosition = mul(directionalLightPosition, _directionalLightProjection);
	output.directionalLightPosition = mul(directionalLightPosition, _depthBias);
	output.directionalLightPosition.xyz /= output.directionalLightPosition.w;

	float4 normal = float4(input.normal, 1.0);
	output.normal = mul(normal, _normal).xyz;

	return output;
}

float3 computeLightedColor(float3 normalVector, float3 lightDirection, float3 lightColor, float attenuation)
{
	float diffuse = max(dot(normalVector, lightDirection), 0.0);
	float3 diffuseColor = lightColor * diffuse * attenuation;
	return diffuseColor;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
	float3 normal = normalize(input.normal); // �f�[�^�`���̎��_��normalize����ĂȂ��@��������͗l

	// ��͂����œ���position�ƁAnormal��color�𗘗p���āA���C�e�B���O�����ĐF�����߂�
	float3 diffuseSpecularLightColor = 0.0f;

	float shadowAttenuation = 1.0;
	if (_directionalLightIsValid > 0.0f)
	{
		if (_directionalLightHasShadowMap > 0.0)
		{
			// z�t�@�C�e�B���O������邽�߂̔�����
			input.directionalLightPosition.z -= 0.001;

			shadowAttenuation = _directionalLightShadowMap.SampleCmpLevelZero(_pcfSampler, input.directionalLightPosition.xy, input.directionalLightPosition.z);
		}

		diffuseSpecularLightColor += shadowAttenuation * computeLightedColor(normal, -_directionalLightDirection.xyz, _directionalLightColor, 1.0);
	}

	unsigned int i = 0; // hlsl�ɂ�for���̏��������̃u���b�N�X�R�[�v���Ȃ�
	for (i = 0; i < MAX_NUM_POINT_LIGHT; i++)
	{
		if (!_pointLightParameter[i]._pointLightIsValid > 0.0)
		{
			continue;
		}

		float3 vertexToPointLightDirection = _pointLightParameter[i]._pointLightPosition - input.worldPosition.xyz;
		float3 dir = vertexToPointLightDirection * _pointLightParameter[i]._pointLightRangeInverse;
		float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);

		shadowAttenuation = 1.0;
		if (_pointLightParameter[i]._pointLightHasShadowMap > 0.0)
		{
			//TODO:�Ƃ肠�����s��v�Z�łȂ�hlsl�{�̂Ƃ���ɏ����Ă���
			// �L���[�u�}�b�v�̂ǂ̖ʂ����ׂ邽�߁A3���ň�ԍ��W���傫���l��T��
			float3 pointLightToVertexDirection = -vertexToPointLightDirection;
			float3 absPosition = abs(pointLightToVertexDirection);
			float maxCoordinateVal = max(absPosition.x, max(absPosition.y, absPosition.z));
			float pointLightDepth = (-_pointLightParameter[i]._pointLightProjection._m22 * maxCoordinateVal + _pointLightParameter[i]._pointLightProjection._m32) / maxCoordinateVal;

			pointLightDepth -= 0.0001;

			// �����ϊ��͕\�����Ă݂Č��߂�
			// TODO:POSITIVE���̌����̓e�X�g���ĂȂ�
			if (maxCoordinateVal == absPosition.x)
			{
				switch (i)
				{
				case 0:
					shadowAttenuation = _pointLightShadowCubeMap[0].SampleCmpLevelZero(_pcfSampler, float3(pointLightToVertexDirection.x, pointLightToVertexDirection.y, -pointLightToVertexDirection.z), pointLightDepth);
					break;
				case 1:
					shadowAttenuation = _pointLightShadowCubeMap[1].SampleCmpLevelZero(_pcfSampler, float3(pointLightToVertexDirection.x, pointLightToVertexDirection.y, -pointLightToVertexDirection.z), pointLightDepth);
					break;
				case 2:
					shadowAttenuation = _pointLightShadowCubeMap[2].SampleCmpLevelZero(_pcfSampler, float3(pointLightToVertexDirection.x, pointLightToVertexDirection.y, -pointLightToVertexDirection.z), pointLightDepth);
					break;
				case 3:
					shadowAttenuation = _pointLightShadowCubeMap[3].SampleCmpLevelZero(_pcfSampler, float3(pointLightToVertexDirection.x, pointLightToVertexDirection.y, -pointLightToVertexDirection.z), pointLightDepth);
					break;
				}
			}
			else if (maxCoordinateVal == absPosition.y)
			{
				if (pointLightToVertexDirection.y > 0)
				{
					switch (i)
					{
					case 0:
						shadowAttenuation = _pointLightShadowCubeMap[0].SampleCmpLevelZero(_pcfSampler, float3(-pointLightToVertexDirection.x, pointLightToVertexDirection.y, pointLightToVertexDirection.z), pointLightDepth);
						break;
					case 1:
						shadowAttenuation = _pointLightShadowCubeMap[1].SampleCmpLevelZero(_pcfSampler, float3(-pointLightToVertexDirection.x, pointLightToVertexDirection.y, pointLightToVertexDirection.z), pointLightDepth);
						break;
					case 2:
						shadowAttenuation = _pointLightShadowCubeMap[2].SampleCmpLevelZero(_pcfSampler, float3(-pointLightToVertexDirection.x, pointLightToVertexDirection.y, pointLightToVertexDirection.z), pointLightDepth);
						break;
					case 3:
						shadowAttenuation = _pointLightShadowCubeMap[3].SampleCmpLevelZero(_pcfSampler, float3(-pointLightToVertexDirection.x, pointLightToVertexDirection.y, pointLightToVertexDirection.z), pointLightDepth);
						break;
					}
				}
				else
				{
					switch (i)
					{
					case 0:
						shadowAttenuation = _pointLightShadowCubeMap[0].SampleCmpLevelZero(_pcfSampler, float3(pointLightToVertexDirection.x, pointLightToVertexDirection.y, -pointLightToVertexDirection.z), pointLightDepth);
						break;
					case 1:
						shadowAttenuation = _pointLightShadowCubeMap[1].SampleCmpLevelZero(_pcfSampler, float3(pointLightToVertexDirection.x, pointLightToVertexDirection.y, -pointLightToVertexDirection.z), pointLightDepth);
						break;
					case 2:
						shadowAttenuation = _pointLightShadowCubeMap[2].SampleCmpLevelZero(_pcfSampler, float3(pointLightToVertexDirection.x, pointLightToVertexDirection.y, -pointLightToVertexDirection.z), pointLightDepth);
						break;
					case 3:
						shadowAttenuation = _pointLightShadowCubeMap[3].SampleCmpLevelZero(_pcfSampler, float3(pointLightToVertexDirection.x, pointLightToVertexDirection.y, -pointLightToVertexDirection.z), pointLightDepth);
						break;
					}
				}
			}
			else // if (maxCoordinateVal == absPosition.z)
			{
				switch (i)
				{
				case 0:
					shadowAttenuation = _pointLightShadowCubeMap[0].SampleCmpLevelZero(_pcfSampler, float3(-pointLightToVertexDirection.x, pointLightToVertexDirection.y, pointLightToVertexDirection.z), pointLightDepth);
					break;
				case 1:
					shadowAttenuation = _pointLightShadowCubeMap[1].SampleCmpLevelZero(_pcfSampler, float3(-pointLightToVertexDirection.x, pointLightToVertexDirection.y, pointLightToVertexDirection.z), pointLightDepth);
					break;
				case 2:
					shadowAttenuation = _pointLightShadowCubeMap[2].SampleCmpLevelZero(_pcfSampler, float3(-pointLightToVertexDirection.x, pointLightToVertexDirection.y, pointLightToVertexDirection.z), pointLightDepth);
					break;
				case 3:
					shadowAttenuation = _pointLightShadowCubeMap[3].SampleCmpLevelZero(_pcfSampler, float3(-pointLightToVertexDirection.x, pointLightToVertexDirection.y, pointLightToVertexDirection.z), pointLightDepth);
					break;
				}
			}
		}

		diffuseSpecularLightColor += shadowAttenuation * computeLightedColor(normal, normalize(vertexToPointLightDirection), _pointLightParameter[i]._pointLightColor, attenuation);
	}

	for (i = 0; i < MAX_NUM_SPOT_LIGHT; i++)
	{
		if (!_spotLightParameter[i]._spotLightIsValid > 0.0)
		{
			continue;
		}

		float3 vertexToSpotLightDirection = _spotLightParameter[i]._spotLightPosition - input.worldPosition.xyz;
		float3 dir = vertexToSpotLightDirection * _spotLightParameter[i]._spotLightRangeInverse;
		float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
		vertexToSpotLightDirection = normalize(vertexToSpotLightDirection);
		float spotLightCurrentAngleCos = dot(_spotLightParameter[i]._spotLightDirection, -vertexToSpotLightDirection);
		attenuation *= smoothstep(_spotLightParameter[i]._spotLightOuterAngleCos, _spotLightParameter[i]._spotLightInnerAngleCos, spotLightCurrentAngleCos);
		attenuation = clamp(attenuation, 0.0, 1.0);

		shadowAttenuation = 1.0;
		if (_spotLightParameter[i]._spotLightHasShadowMap > 0.0)
		{
			// VS�Ōv�Z�������Ƃ��낾���AVS�̏o�͂ɔz��͎��ĂȂ��̂�
			float4 spotLightPosition = mul(input.worldPosition, _spotLightParameter[i]._spotLightView);
			spotLightPosition = mul(spotLightPosition, _spotLightParameter[i]._spotLightProjection);
			spotLightPosition = mul(spotLightPosition, _depthBias);
			spotLightPosition.xyz /= spotLightPosition.w;
			// z�t�@�C�e�B���O������邽�߂̔�����
			spotLightPosition.z -= 0.00001;

			//TODO: �T���v���ɂ̓��e�����ł����A�N�Z�X�ł��Ȃ��̂łƂ肠�����̑Ή�
			switch (i)
			{
			case 0:
				shadowAttenuation = _spotLightShadowMap[0].SampleCmpLevelZero(_pcfSampler, spotLightPosition.xy, spotLightPosition.z);
				break;
			case 1:
				shadowAttenuation = _spotLightShadowMap[1].SampleCmpLevelZero(_pcfSampler, spotLightPosition.xy, spotLightPosition.z);
				break;
			case 2:
				shadowAttenuation = _spotLightShadowMap[2].SampleCmpLevelZero(_pcfSampler, spotLightPosition.xy, spotLightPosition.z);
				break;
			case 3:
				shadowAttenuation = _spotLightShadowMap[3].SampleCmpLevelZero(_pcfSampler, spotLightPosition.xy, spotLightPosition.z);
				break;
			}
		}

		diffuseSpecularLightColor += shadowAttenuation * computeLightedColor(normal, vertexToSpotLightDirection, _spotLightParameter[i]._spotLightColor, attenuation);
	}

	return _multiplyColor * float4(shadowAttenuation * diffuseSpecularLightColor + _ambientLightColor.rgb, 1.0);
}
