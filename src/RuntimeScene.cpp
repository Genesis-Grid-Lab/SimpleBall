#include "RuntimeScene.h"
#include "Components.h"
#include "ScriptableEntity.h"

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
		comp.Instance->OnUpdate(ts);
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
