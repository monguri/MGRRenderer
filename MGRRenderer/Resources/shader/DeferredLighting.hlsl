//TODO: ���܂�GBufferPack,GBufferUnpack.hlsl�ɂ܂Ƃ܂�Ȃ���

static const float2 SPECULAR_POWER_RANGE = {10.0, 250.0};

cbuffer ViewMatrixInverse : register(b0)
{
	matrix _viewInverse;
};

cbuffer DepthTextureProjectionMatrix : register(b1)
{
	matrix _depthTextureProjection;
};

cbuffer DepthBiasMatrix : register(b2)
{
	matrix _depthBias;
};

cbuffer AmbientLightParameter : register(b3)
{
	float4 _ambientLightColor;
};

cbuffer DirectionalLightViewMatrix : register(b4)
{
	matrix _directionalLightView;
};

cbuffer DirectionalLightProjectionMatrix : register(b5)
{
	matrix _directionalLightProjection;
};

cbuffer DirectionalLightParameter : register(b6)
{
	float3 _directionalLightDirection;
	float _directionalLightHasShadowMap;
	float4 _directionalLightColor;
};

static const unsigned int NUM_FACE_CUBEMAP_TEXTURE = 6;
static const unsigned int MAX_NUM_POINT_LIGHT = 4; // ���ӁF�v���O�������ƒ萔�̈�v���K�v

cbuffer PointLightParameter : register(b7)
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

cbuffer SpotLightParameter : register(b8)
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

Texture2D<float4> _gBufferDepthStencil : register(t0); // �f�v�X�e�N�X�`����Texture2D<float>�ŏ\�������ATexture2D<float4>�ł��ǂݍ��߂�
Texture2D<float4> _gBufferColorSpecularIntensity : register(t1);
Texture2D<float4> _gBufferNormal : register(t2);
Texture2D<float4> _gBufferSpecularPower : register(t3);

Texture2D<float> _directionalLightShadowMap : register(t4);
TextureCube<float> _pointLightShadowCubeMap[MAX_NUM_POINT_LIGHT];
Texture2D<float> _spotLightShadowMap[MAX_NUM_SPOT_LIGHT];

SamplerState _pointSampler : register(s0);
SamplerComparisonState _pcfSampler : register(s1);
// �܂��V���h�E�}�b�v�͍l�����ĂȂ�

//static const float2 vertexPosition[4] = {
//	float2(-1.0, 1.0),
//	float2(1.0, 1.0),
//	float2(-1.0, -1.0),
//	float2(1.0, -1.0),
//};

struct VS_INPUT
{
	float2 position : POSITION;
	float2 texCoord : TEX_COORD;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float2 texCoord : TEX_COORD;
};

PS_INPUT VS(VS_INPUT input)
//PS_INPUT VS(uint vertexId : SV_VERTEXID)
{
	PS_INPUT output;

	output.position = float4(input.position, 0.0, 1.0);
	//output.position = float4(vertexPosition[vertexId].xy, 0.0, 1.0);
	output.texCoord = input.texCoord;
	//output.texCoord = output.position.xy;
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
	//float depth = _gBufferDepthStencil.Sample(_pointSampler, input.position.xy).x;
	float depth = _gBufferDepthStencil.Sample(_pointSampler, input.texCoord).x;
	float4 viewPosition;
	viewPosition.x = input.texCoord.x * 2 - 1; // [0, 1]����[-1, 1]�ւ̕ϊ�
	viewPosition.x = viewPosition.x * _depthTextureProjection._m32 / (depth + _depthTextureProjection._m22) / _depthTextureProjection._m00;
	viewPosition.y = -(input.texCoord.y * 2 - 1); // [1, 0]����[-1, 1]�ւ̕ϊ�
	viewPosition.y = viewPosition.y * _depthTextureProjection._m32 / (depth + _depthTextureProjection._m22) / _depthTextureProjection._m11;
	viewPosition.z = -_depthTextureProjection._m32 / (depth + _depthTextureProjection._m22);
	viewPosition.w = 1.0;
	
	float4 worldPosition = mul(viewPosition, _viewInverse);

	//float4 colorSpecularIntensity = _gBufferColorSpecularIntensity.Sample(_pointSampler, input.position.xy);
	float4 colorSpecularIntensity = _gBufferColorSpecularIntensity.Sample(_pointSampler, input.texCoord);
	float3 color = colorSpecularIntensity.xyz;
	float specularIntensity = colorSpecularIntensity.w;

	//float3 normalizedNormal = _gBufferNormal.Sample(_pointSampler, input.position.xy).xyz;
	float3 normalizedNormal = _gBufferNormal.Sample(_pointSampler, input.texCoord).xyz;
	float3 normal = normalizedNormal * 2.0 - 1.0;

	//float normalizedSpecularPower = _gBufferSpecularPower.Sample(_pointSampler, input.position.xy).x;
	float normalizedSpecularPower = _gBufferSpecularPower.Sample(_pointSampler, input.texCoord).x;
	float specularPower = SPECULAR_POWER_RANGE.x + SPECULAR_POWER_RANGE.y * normalizedSpecularPower;

	// ��͂����œ���position�ƁAnormal��color�𗘗p���āA���C�e�B���O�����ĐF�����߂�
	float3 diffuseSpecularLightColor = 0.0f;

	float shadowAttenuation = 1.0;
	if (_directionalLightHasShadowMap > 0.0)
	{
		float4 lightPosition = mul(worldPosition, _directionalLightView);
		lightPosition = mul(lightPosition, _directionalLightProjection);
		lightPosition = mul(lightPosition, _depthBias);
		lightPosition.xyz /= lightPosition.w;
		// z�t�@�C�e�B���O������邽�߂̔�����
		lightPosition.z -= 0.001;

		shadowAttenuation = _directionalLightShadowMap.SampleCmpLevelZero(_pcfSampler, lightPosition.xy, lightPosition.z);
	}

	diffuseSpecularLightColor += shadowAttenuation * computeLightedColor(normal, -_directionalLightDirection.xyz, _directionalLightColor.rgb, 1.0);

	unsigned int i = 0; // hlsl�ɂ�for���̏��������̃u���b�N�X�R�[�v���Ȃ�
	for (i = 0; i < MAX_NUM_POINT_LIGHT; i++)
	{
		if (!_pointLightParameter[i]._pointLightIsValid > 0.0)
		{
			continue;
		}

		float3 vertexToPointLightDirection = _pointLightParameter[i]._pointLightPosition - worldPosition.xyz;
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

		float3 vertexToSpotLightDirection = _spotLightParameter[i]._spotLightPosition - worldPosition.xyz;
		float3 dir = vertexToSpotLightDirection * _spotLightParameter[i]._spotLightRangeInverse;
		float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
		vertexToSpotLightDirection = normalize(vertexToSpotLightDirection);
		float spotLightCurrentAngleCos = dot(_spotLightParameter[i]._spotLightDirection, -vertexToSpotLightDirection);
		attenuation *= smoothstep(_spotLightParameter[i]._spotLightOuterAngleCos, _spotLightParameter[i]._spotLightInnerAngleCos, spotLightCurrentAngleCos);
		attenuation = clamp(attenuation, 0.0, 1.0);

		shadowAttenuation = 1.0;
		if (_spotLightParameter[i]._spotLightHasShadowMap > 0.0)
		{
			float4 lightPosition = mul(worldPosition, _spotLightParameter[i]._spotLightView);
			lightPosition = mul(lightPosition, _spotLightParameter[i]._spotLightProjection);
			lightPosition = mul(lightPosition, _depthBias);
			lightPosition.xyz /= lightPosition.w;
			// z�t�@�C�e�B���O������邽�߂̔�����
			lightPosition.z -= 0.00001;

			//TODO: �T���v���ɂ̓��e�����ł����A�N�Z�X�ł��Ȃ��̂łƂ肠�����̑Ή�
			switch (i)
			{
			case 0:
				shadowAttenuation = _spotLightShadowMap[0].SampleCmpLevelZero(_pcfSampler, lightPosition.xy, lightPosition.z);
				break;
			case 1:
				shadowAttenuation = _spotLightShadowMap[1].SampleCmpLevelZero(_pcfSampler, lightPosition.xy, lightPosition.z);
				break;
			case 2:
				shadowAttenuation = _spotLightShadowMap[2].SampleCmpLevelZero(_pcfSampler, lightPosition.xy, lightPosition.z);
				break;
			case 3:
				shadowAttenuation = _spotLightShadowMap[3].SampleCmpLevelZero(_pcfSampler, lightPosition.xy, lightPosition.z);
				break;
			}
		}

		diffuseSpecularLightColor += shadowAttenuation * computeLightedColor(normal, vertexToSpotLightDirection, _spotLightParameter[i]._spotLightColor, attenuation);
	}

	return float4((color * (diffuseSpecularLightColor + _ambientLightColor.rgb)), 1.0);
}
