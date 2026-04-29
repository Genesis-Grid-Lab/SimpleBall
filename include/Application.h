#pragma once

#include "ContentBrowserPanel.h"
#include "EditorScene.h"
#include "RuntimeScene.h"
#include "SceneHierarchyPanel.h"
#include "raylib.h"

class Application {
public:
  Application();
  ~Application() = default;

  void Run();

private:
  void NewScene();
  void OpenScene();
  void SaveScene();
  void SaveSceneAs();

  void OnScenePlay();
  void OnSceneStop();

 private:
   void UI_Toolbar();
   void ViewScene();
   void TestingGround();

private:
  Ref<EditorScene> m_EditorScene;
  Ref<RuntimeScene> m_RuntimeScene;

  enum class SceneState { Edit = 0, Play = 1, Paused = 2 };
  SceneState m_SceneState = SceneState::Edit;

  RenderTexture m_ViewTexture;
  bool m_Focused = false;
  bool m_Hovered = false;

  // Panels
  SceneHierarchyPanel m_SceneHierarchyPanel;
  ContentBrowserPanel m_ContentBrowserPanel;
};
