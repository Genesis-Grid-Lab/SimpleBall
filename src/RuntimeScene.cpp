#include "RuntimeScene.h"
#include "Components.h"
#include "Entity.h"

RuntimeScene::~RuntimeScene() {}

//--------------------------------------------------------------------
// Scene::OnRuntimeStart
//--------------------------------------------------------------------
void RuntimeScene::OnRuntimeStart() {}

//--------------------------------------------------------------------
// Scene::OnRuntimeStop
//--------------------------------------------------------------------
void RuntimeScene::OnRuntimeStop() {}


//--------------------------------------------------------------------
// Scene::OnUpdate
//--------------------------------------------------------------------
void RuntimeScene::OnUpdate(float ts) {

  ClearBackground(SKYBLUE);

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



  if(CamPresent)
    EndMode3D();

  DrawFPS(10, 10);

  FlushEntityDestruction();
}
