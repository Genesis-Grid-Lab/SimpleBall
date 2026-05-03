#pragma once

#include <raylib.h>
#include <string>

struct ShaderLight {
  int enabled;
  int type;

  Vector3 position;
  Vector3 direction;

  Vector4 color;

  float intensity;
  float range;
  float spotAngle;
};

void UploadLight(Shader shader, int index, const ShaderLight &light);

struct ShadowMap {
  RenderTexture2D RenderTexture;

  Matrix LightView;
  Matrix LightProjection;
  Matrix LightSpaceMatrix;

  Shader DepthShader;

  int ShadowMapLoc = -1;
  int LightSpaceLoc = -1;
};

constexpr int MAX_LIGHTS = 16;

struct LightShaderCahe {
  Shader shader = {0};

  int uLightCount = -1;

  int lightSpaceMatrix = -1;
  int shadowMap = -1;
  int useShadows = -1;

  struct LightLoc {
    int enabled = -1;
    int type = -1;
    int position = -1;
    int direction = -1;
    int color = -1;
    int intensity = -1;
    int range = -1;
    int spotAngle = -1;
  };

  LightLoc lights[MAX_LIGHTS];

  void Init(Shader s){
    shader = s;

    uLightCount = GetShaderLocation(shader, "uLightCount");

    lightSpaceMatrix = GetShaderLocation(shader, "lightSpaceMatrix");
    shadowMap = GetShaderLocation(shader, "shadowMap");
    useShadows = GetShaderLocation(shader, "useShadows");

    for (int i = 0; i < MAX_LIGHTS; i++)
      {
	std::string base = "uLights[" + std::to_string(i) + "]";

	lights[i].enabled   = GetShaderLocation(shader, (base + ".enabled").c_str());
	lights[i].type      = GetShaderLocation(shader, (base + ".type").c_str());
	lights[i].position  = GetShaderLocation(shader, (base + ".position").c_str());
	lights[i].direction = GetShaderLocation(shader, (base + ".direction").c_str());
	lights[i].color     = GetShaderLocation(shader, (base + ".color").c_str());
	lights[i].intensity = GetShaderLocation(shader, (base + ".intensity").c_str());
	lights[i].range     = GetShaderLocation(shader, (base + ".range").c_str());
	lights[i].spotAngle = GetShaderLocation(shader, (base + ".spotAngle").c_str());
      }
  }

  void SetLightCount(int count){
    SetShaderValue(shader, uLightCount, &count, SHADER_UNIFORM_INT);
  }

  void UploadLight(int index, const ShaderLight& light)
  {
    if (index < 0 || index >= MAX_LIGHTS)
      return;

    const auto& loc = lights[index];

    SetShaderValue(shader, loc.enabled, &light.enabled, SHADER_UNIFORM_INT);
    SetShaderValue(shader, loc.type, &light.type, SHADER_UNIFORM_INT);
    SetShaderValue(shader, loc.position, &light.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc.direction, &light.direction, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc.color, &light.color, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, loc.intensity, &light.intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, loc.range, &light.range, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, loc.spotAngle, &light.spotAngle, SHADER_UNIFORM_FLOAT);
  }

  void SetShadowData(Matrix lightSpace, Texture2D depthTexture, bool enabled)
  {
    int enabledInt = enabled ? 1 : 0;

    SetShaderValue(shader, useShadows, &enabledInt, SHADER_UNIFORM_INT);
    SetShaderValueMatrix(shader, lightSpaceMatrix, lightSpace);
    SetShaderValueTexture(shader, shadowMap, depthTexture);
  }
};
