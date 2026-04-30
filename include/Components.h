#pragma once

#include "UUID.h"
#include "raylib.h"

struct IDComponent {
  UUID ID;
  bool Active = false;

  IDComponent() = default;
  IDComponent(const IDComponent &) = default;
  IDComponent(const UUID& uuid) : ID(uuid) {} 
};

struct TagComponent {
  std::string Tag;

  TagComponent() = default;
  TagComponent(const TagComponent &) = default;
  TagComponent(const std::string &tag) : Tag(tag) {}
};

struct TransformComponent {
  Vector3 Translation = {0,0,0};
  Vector3 Rotation = {0,0,0};
  Vector3 Scale = {1,1,1};

  TransformComponent() = default;
  TransformComponent(const TransformComponent &) = default;
  TransformComponent(const Vector3 &translation) : Translation(translation) {}
};

struct CameraComponent {
  Camera3D Camera = {0};

  CameraComponent() = default;
  CameraComponent(const CameraComponent &) = default;
  CameraComponent(const Camera3D &camera) : Camera(camera) {}  
};

struct CubeComponent {
  Color color = WHITE;

  CubeComponent() = default;
  CubeComponent(const CubeComponent &) = default;
  CubeComponent(const Color &col)
      :color(col) {}
};

struct SphereComponent {
  Color color = WHITE;
  
  SphereComponent() = default;
  SphereComponent(const SphereComponent &) = default;
  SphereComponent(const Color &col) : color(col) {}
};

class ScriptableEntity;

struct NativeScriptComponent {
  ScriptableEntity *Instance = nullptr;

  ScriptableEntity *(*InstantiateScript)();
  void (*DestroyScript)(NativeScriptComponent *);

  template <typename T> void Bind() {
    InstantiateScript = []() {
      return static_cast<ScriptableEntity*>(new T());
    };

    DestroyScript = [](NativeScriptComponent *nsc) {
      delete nsc->Instance;
      nsc->Instance = nullptr;
    };
  }  
};
