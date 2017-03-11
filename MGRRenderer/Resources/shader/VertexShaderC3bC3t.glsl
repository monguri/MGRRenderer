#version 430

const int MAX_NUM_POINT_LIGHT = 4; // ���ӁF�v���O�������ƒ萔�̈�v���K�v
const int MAX_NUM_SPOT_LIGHT = 4; // ���ӁF�v���O�������ƒ萔�̈�v���K�v

attribute vec3 a_position; // ���ꂪvec3�ɂȂ��Ă���̂ɒ��� TODO:�Ȃ��Ȃ̂��H
attribute vec3 a_normal;
attribute vec2 a_texCoord;
attribute vec4 a_blendWeight;
attribute vec4 a_blendIndex;

const int MAX_SKINNING_JOINT = 20; // TODO:�Ȃ�60�܂łȂ̂��H

uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_depthBiasMatrix;
uniform mat4 u_normalMatrix;
uniform bool u_pointLightIsValid[MAX_NUM_POINT_LIGHT];
uniform vec3 u_pointLightPosition[MAX_NUM_POINT_LIGHT];
uniform bool u_spotLightIsValid[MAX_NUM_SPOT_LIGHT];
uniform vec3 u_spotLightPosition[MAX_NUM_SPOT_LIGHT];
//uniform vec3 u_cameraPosition;
uniform mat4 u_matrixPalette[MAX_SKINNING_JOINT];
varying vec4 v_normal;
varying vec2 v_texCoord;
varying vec4 v_worldPosition;
varying vec3 v_vertexToPointLightDirection[MAX_NUM_POINT_LIGHT];
varying vec3 v_vertexToSpotLightDirection[MAX_NUM_SPOT_LIGHT];
//varying vec3 v_vertexToCameraDirection;

vec4 getPosition()
{
	mat4 skinMatrix = u_matrixPalette[int(a_blendIndex[0])] * a_blendWeight[0];

	if (a_blendWeight[1] > 0.0)
	{
		skinMatrix += u_matrixPalette[int(a_blendIndex[1])] * a_blendWeight[1];

		if (a_blendWeight[2] > 0.0)
		{
			skinMatrix += u_matrixPalette[int(a_blendIndex[2])] * a_blendWeight[2];

			if (a_blendWeight[3] > 0.0)
			{
				skinMatrix += u_matrixPalette[int(a_blendIndex[3])] * a_blendWeight[3];
			}
		}
	}

	vec4 position = vec4(a_position, 1.0);
	vec4 skinnedPosition = skinMatrix * position;
	skinnedPosition.w = 1.0;
	return skinnedPosition;
}

void main()
{
	vec4 worldPosition = u_modelMatrix * getPosition();
	for (uint i = 0; i < MAX_NUM_POINT_LIGHT; i++)
	{
		if (!u_pointLightIsValid[i])
		{
			continue;
		}

		v_vertexToPointLightDirection[i] = u_pointLightPosition[i] - worldPosition.xyz;
	}

	for (uint i = 0; i < MAX_NUM_SPOT_LIGHT; i++)
	{
		if (!u_spotLightIsValid[i])
		{
			continue;
		}

		v_vertexToSpotLightDirection[i] = u_spotLightPosition[i] - worldPosition.xyz;
	}
	//v_vertexToCameraDirection = u_cameraPosition - worldPosition.xyz;

	gl_Position = u_projectionMatrix * u_viewMatrix * worldPosition;
	vec4 normal = vec4(a_normal, 1.0);
	v_normal = vec4(normalize((u_normalMatrix * normal).xyz), 1.0);
	v_texCoord = a_texCoord;
	v_texCoord.y = 1.0 - v_texCoord.y; // c3b�̎���ɂ�����
	v_worldPosition = worldPosition;
}
