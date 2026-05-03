#include "LightHelper.h"
#include "raylib.h"
#include <string>


void UploadLight(Shader shader, int index, const ShaderLight &light){

  std::string base = "uLights[" + std::to_string(index) + "]";

  SetShaderValue(shader, GetShaderLocation(shader, (base + ".enabled").c_str()),
                 (void *)&light.enabled, SHADER_UNIFORM_INT);

  SetShaderValue(shader, GetShaderLocation(shader, (base + ".type").c_str()),
                 (void *)&light.type, SHADER_UNIFORM_INT);

  SetShaderValue(shader,
                 GetShaderLocation(shader, (base + ".position").c_str()),
                 (void *)&light.position, SHADER_UNIFORM_VEC3);

  SetShaderValue(shader,
                 GetShaderLocation(shader, (base + ".direction").c_str()),
                 (void *)&light.direction, SHADER_UNIFORM_VEC3);

  SetShaderValue(shader, GetShaderLocation(shader, (base + ".color").c_str()),
                 (void *)&light.color, SHADER_UNIFORM_VEC4);

  SetShaderValue(shader,
                 GetShaderLocation(shader, (base + ".intensity").c_str()),
                 (void *)&light.intensity, SHADER_UNIFORM_FLOAT);

  SetShaderValue(shader, GetShaderLocation(shader, (base + ".range").c_str()),
                 (void *)&light.range, SHADER_UNIFORM_FLOAT);

  SetShaderValue(shader, GetShaderLocation(shader, (base + ".spotAngle").c_str()),
                 (void *)&light.spotAngle, SHADER_UNIFORM_FLOAT);  

  
}
