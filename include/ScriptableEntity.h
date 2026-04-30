#pragma once

#include "Entity.h"

class ScriptableEntity {
public:
  virtual ~ScriptableEntity() {}

  template <typename T> T &GetComponent() { return m_Entity.GetComponent<T>(); }

  Scene *GetScene() { return m_Scene; }

protected:
  virtual void OnCreate() {}
  virtual void OnDestroy() {}
  virtual void OnUpdate(float dt) {}

private:
  Entity m_Entity;
  Scene *m_Scene;

  friend class Scene;
  friend class RuntimeScene;
  friend class SceneHierarchyPanel;
};
