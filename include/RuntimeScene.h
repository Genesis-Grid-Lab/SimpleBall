#pragma once

#include "LightHelper.h"
#include "Scene.h"
class RuntimeScene : public Scene {
public:
  RuntimeScene();
  virtual ~RuntimeScene() override;

  void OnRuntimeStart();
  void OnRuntimeStop();

  virtual void OnUpdate(float ts) override;

private:
  void ShadowPass();
  void DrawDepthModel(Model &model, Vector3 pos, Vector3 rot, Vector3 scale);
  void DrawDeptScene();

private:
  Shader m_LightShader;
  Shader m_DefaultShader;

  LightShaderCahe m_LightShaderCache;
  ShadowMap m_ShadowMap;

  Model m_CubeModel;
  Model m_SphereModel;
  Model m_PlaneModel;
  
};
