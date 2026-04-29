#pragma once

#include "Scene.h"
#include "Entity.h"

class SceneHierarchyPanel {
public:
  SceneHierarchyPanel() = default;
  SceneHierarchyPanel(const Ref<Scene> &scene);

  void SetContext(const Ref<Scene> &scene);
  Entity GetSelectedEntity() const { return m_SelectionContext; }
  void SetSelectedENtity(Entity entity);

  void Update();

  void Shutdown();

private:
  void DrawEntityNode(Entity entity);
  void DrawComponents(Entity entity);

private:
  Ref<Scene> m_Context;
  Entity m_SelectionContext;
  
};
