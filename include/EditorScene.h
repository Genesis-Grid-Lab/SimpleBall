#pragma once

#include "LightHelper.h"
#include "Scene.h"
#include "EditorCamera.h"
#include "SceneHierarchyPanel.h"

class EditorScene : public Scene {
public:
  EditorScene();
  virtual ~EditorScene() override;

  virtual void OnUpdate(float ts) override;

  void SetViewportState(bool hovered, bool focused) {
    m_EditorCamera.SetViewportState(hovered, focused);
  }
  void SetMousePos(Vector2 pos) { m_RelativeMousPos = pos; }  

  const Camera &GetCamera() { return m_EditorCamera.GetCam(); }

  SceneHierarchyPanel *m_SceneHierarchy;
  Vector2 VSIZE;
  Vector2 VPOS;

private:
  void Control();
  void ShadowPass();
  void DrawDepthModel(Model &model, Vector3 pos, Vector3 rot, Vector3 scale);
  void DrawDeptScene();

private:
  EditorCamera m_EditorCamera;
  int m_GizmoState;
  Vector2 m_RelativeMousPos;

  Ray m_Ray;
  RayCollision m_Collision;
  Shader m_LightShader;
  Shader m_DefaultShader;

  LightShaderCahe m_LightShaderCache;
  ShadowMap m_ShadowMap;

  // toremove
  Mesh cube;
  Model skybox;

  Model m_CubeModel;
  Model m_SphereModel;
  Model m_PlaneModel;
};
