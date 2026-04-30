#pragma once

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

private:
  void Control();

private:
  EditorCamera m_EditorCamera;
  int m_GizmoState;
  Vector2 m_RelativeMousPos;

  Ray m_Ray;
  RayCollision m_Collision;

  // toremove
  Mesh cube;
  Model skybox;
};
