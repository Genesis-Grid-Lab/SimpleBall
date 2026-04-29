#pragma once

#include "Scene.h"
#include "EditorCameraController.h"

class EditorScene : public Scene {
public:
  EditorScene();
  virtual ~EditorScene() override;

  virtual void OnUpdate(float ts) override;

  const Camera &GetCamera() { return m_EditorCamera;}

private:
  Camera m_EditorCamera;
  EditorCameraController Controller;

  // toremove
  Mesh cube;
  Model skybox;
};
