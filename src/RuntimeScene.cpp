#include "RuntimeScene.h"
#include "Components.h"
#include "LuaScriptEngine.h"
#include "ScriptableEntity.h"
#include "raylib.h"
#include <sol/forward.hpp>

RuntimeScene::~RuntimeScene() {}

//--------------------------------------------------------------------
// Scene::OnRuntimeStart
//--------------------------------------------------------------------
void RuntimeScene::OnRuntimeStart() {
  GroupEntity<NativeScriptComponent>(
      [=](auto entity, auto &comp, auto &transform, auto id) {
	if(!comp.Instance){
          // ASSERT(comp.InstantiateScript,
          //        "NativeSCriptComponent missing Bind()");

          comp.Instance = comp.InstantiateScript();

          comp.Instance->m_Entity = Entity(entity, this);
          comp.Instance->m_Scene = this;
	  comp.Instance->OnCreate();
        } else {
          comp.Instance->m_Entity = Entity(entity, this);
	  comp.Instance->m_Scene = this;
        }
      });

  GroupEntity<LuaScriptComponent>(
      [this](auto entity, auto &comp, auto &transform, auto id) {
	LuaScriptEngine::LoadScript(Entity(entity, this), comp);
  });
}

//--------------------------------------------------------------------
// Scene::OnRuntimeStop
//--------------------------------------------------------------------
void RuntimeScene::OnRuntimeStop() {}


//--------------------------------------------------------------------
// Scene::OnUpdate
//--------------------------------------------------------------------
void RuntimeScene::OnUpdate(float ts) {
  ClearBackground(SKYBLUE);

  GroupEntity<NativeScriptComponent>(
      [=](auto entity, auto &comp, auto &transform, auto id) {
        if (!comp.Instance) {
          // ASSERT(comp.InstantiateScript,
          //        "NativeSCriptComponent missing Bind()");

          comp.Instance = comp.InstantiateScript();

          comp.Instance->m_Entity = Entity(entity, this);
          comp.Instance->m_Scene = this;
	  comp.Instance->OnCreate();
        } else {
          comp.Instance->m_Entity = Entity(entity, this);
	  comp.Instance->m_Scene = this;
        }
        comp.Instance->OnUpdate(ts);
      });

  GroupEntity<LuaScriptComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
        if (!comp.Valid)
          return;

        sol::protected_function onUpdate = comp.Instance["onUpdate"];

        if (onUpdate.valid()) {
          auto result = onUpdate(comp.Instance,ts);

          if (!result.valid()) {
            sol::error err = result;

	    TraceLog(LOG_ERROR, "Lua onUpdate error: %s", err.what());
	  }
	}
  });

  bool CamPresent = false;

  GroupEntity<CameraComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
        CamPresent = true;

	comp.Camera.position = transform.Translation;

	BeginMode3D(comp.Camera);
      });

  GroupEntity<CubeComponent>(
      [this](auto entity, auto &comp, auto &transform, auto id) {
	DrawCubeV(transform.Translation, transform.Scale, comp.color);
      });

  GroupEntity<SphereComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
        DrawSphere(transform.Translation, transform.Scale.x, comp.color);
      });  



  if(CamPresent)
    EndMode3D();

  DrawFPS(10, 10);

  FlushEntityDestruction();
}
