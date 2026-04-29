#pragma once

#include "Scene.h"
class RuntimeScene : public Scene {
public:
  RuntimeScene() = default;
  virtual ~RuntimeScene() override;

  void OnRuntimeStart();
  void OnRuntimeStop();

  virtual void OnUpdate(float ts) override;

private:
  
};
