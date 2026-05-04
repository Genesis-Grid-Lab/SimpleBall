#pragma once

#include "Jolt.h"
#include "Physics/Body/BodyID.h"
#include "Physics/Character/CharacterVirtual.h"
#include "UUID.h"
#include "raylib.h"
#include "sol/sol.hpp"

//---------------------------------------
// Core
//---------------------------------------

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

//---------------------------------------
// Rendering
//---------------------------------------

struct RenderComponent {
  Color Tint = WHITE;
  bool useSceneLighting = true;
};

struct PlaneComponent : RenderComponent {

  PlaneComponent() = default;
  PlaneComponent(const PlaneComponent &) = default;
};

struct CubeComponent : RenderComponent {

  CubeComponent() = default;
  CubeComponent(const CubeComponent &) = default;
};

struct SphereComponent : RenderComponent {
  
  SphereComponent() = default;
  SphereComponent(const SphereComponent &) = default;
};

struct ModelComponent : RenderComponent {
  std::string ModelPath = "";
  Model model;
  bool Loaded = false;

  ModelComponent() = default;
  ModelComponent(const ModelComponent &) = default;
  ModelComponent(const std::string &path) : ModelPath(path) {}
};

struct SpriteComponent : RenderComponent {
  std::string TexturePath = "";

  SpriteComponent() = default;
  SpriteComponent(const SpriteComponent &) = default;
  SpriteComponent(const std::string &path) : TexturePath(path) {}
};

//---------------------------------------
// Camera
//---------------------------------------

struct CameraComponent {
  Camera3D Camera = {0};
  Vector3 Target = {0, 0, 0};
  bool Primary = false;
  bool FixedAspecRatio = false;
  bool UseTargetMode = false;

  CameraComponent() = default;
  CameraComponent(const CameraComponent &) = default;
  CameraComponent(const Camera3D &camera) : Camera(camera) {}
};

//---------------------------------------
// Lighting
//---------------------------------------

enum class LightType { Directional = 0, Point, Spot };

struct LightComponent {
  LightType Type = LightType::Point;

  Color ColorValue = WHITE;
  
  float Intensity = 1.0f;
  float Range = 10.0f;
  float SpotAngle = 1.0f;

  Vector3 Direction = {0.0f, -1.0f, 0.0f};

  bool CastShadow = false;

  LightComponent() = default;
  LightComponent(const LightComponent &) = default;
};

//---------------------------------------
// Physics-ish V!
//---------------------------------------

enum class BodyType { Static = 0, Dynamic, Kinematic };

struct RigidbodyComponent {
  BodyType Type = BodyType::Dynamic;

  Vector3 Velocity = {0, 0, 0};
  Vector3 AngularVelocity = {0, 0, 0};

  float Mass = 1.0f;
  float Restitution = 0.5f;
  float Friction = 0.2f;
  float LinearDamping = 0.5f;
  float AngularDamping = 0.5f;

  bool useGravity = true;
  bool Dirty = true;

  JPH::BodyID RuntimeBodyID;

  RigidbodyComponent() = default;
  RigidbodyComponent(const RigidbodyComponent &) = default;
};

struct CharacterComponent {
  float MaxSlopeAngle = 45.0f;
  float MaxStepHeight = 0.3f;
  JPH::CharacterVirtual *RuntimeCharacter = nullptr;

  CharacterComponent() = default;
  CharacterComponent(const CharacterComponent &) = default;
};

struct ColliderComponent {
  Vector3 Offset = {0, 0, 0};
  bool IsTrigger = false;
};

struct BoxColliderComponent : ColliderComponent {  
  Vector3 Size = {1, 1, 1};

  BoxColliderComponent() = default;
  BoxColliderComponent(const BoxColliderComponent &) = default;
};

struct SphereColliderComponent : ColliderComponent {
  float Radius = 0.5f;

  SphereColliderComponent() = default;
  SphereColliderComponent(const SphereColliderComponent &) = default;
};

struct CapsuleColliderComponent : ColliderComponent {
  float Radius = 0.5f;
  float HalfHeight = 0.5f;

  CapsuleColliderComponent() = default;
  CapsuleColliderComponent(const CapsuleColliderComponent &) = default;
};

//---------------------------------------
// Audio
//---------------------------------------

struct AudioSourceComponent {
  std::string SoundPath;

  bool PlayOnStart = false;
  bool Loop = false;

  float Volume = 1.0f;
  float Pitch = 1.0f;

  AudioSourceComponent() = default;
  AudioSourceComponent(const AudioSourceComponent &) = default;
};

struct AudioListenerComponent {
  bool Primary = true;

  AudioListenerComponent() = default;
  AudioListenerComponent(const AudioListenerComponent &) = default;
};

//---------------------------------------
// Animation
//---------------------------------------

struct AnimationComponent {
  std::string AnimationPath;
  ModelAnimation *AnimsPtr = nullptr;
  unsigned int AnimationsCount = 0;
  int CurrentAnimationIndex = 0;
  int CurrentFrame = 0;
  bool IsPlaying = true;
  float FrameTime = 0.0f;
  float Speed = 1.0f;
  bool Loaded = false;

  void Play(int index) {
    if(index >= 0 && index < (int)AnimationsCount){
      CurrentAnimationIndex = index;
      CurrentFrame = 0;
      FrameTime = 0.0f;
    }
  }

  AnimationComponent() = default;
  AnimationComponent(const AnimationComponent &) = default;
  AnimationComponent(const std::string &path) : AnimationPath(path) {}
};

//---------------------------------------
// Scripting
//---------------------------------------

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

struct LuaScriptComponent {
  std::string scriptPath;
  std::string sourceCode;
  
  bool isDirty = false;
  bool Valid = false;

  sol::table Instance;

  LuaScriptComponent() = default;
  LuaScriptComponent(const LuaScriptComponent &) = default;
};

//---------------------------------------
// Editor-only
//---------------------------------------

struct EditorOnlyComponent {
  bool Visible = true;
  bool Loacked = false;

  EditorOnlyComponent() = default;
  EditorOnlyComponent(const EditorOnlyComponent &) = default;  
};
