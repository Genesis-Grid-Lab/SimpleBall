#include "RuntimeScene.h"
#include "Components.h"
#include "LuaScriptEngine.h"
#include "ResourceManager.h"
#include "ScriptableEntity.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <sol/forward.hpp>
#include "PhysicsEngine.h"

RuntimeScene::RuntimeScene() {
  m_ViewTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  m_LightShader = ResourceManager::Get<Shader>("LightShader");

  m_LightShaderCache.Init(m_LightShader);

  m_DefaultShader.id = rlGetShaderIdDefault();

  m_ShadowMap.RenderTexture = LoadRenderTexture(2048, 2048);

  m_ShadowMap.DepthShader = ResourceManager::Get<Shader>("shadowMap");

  m_ShadowMap.ShadowMapLoc = GetShaderLocation(m_LightShader, "shadowMap");

  m_ShadowMap.LightSpaceLoc = GetShaderLocation(m_LightShader, "lightSpaceMatrix");

  m_CubeModel = LoadModelFromMesh(GenMeshCube(1, 1, 1));
  m_SphereModel = LoadModelFromMesh(GenMeshSphere(1, 32, 32));
  m_PlaneModel = LoadModelFromMesh(GenMeshPlane(1, 1, 1, 1));
}

RuntimeScene::~RuntimeScene() {}

void RuntimeScene::DrawDepthModel(Model &model, Vector3 pos, Vector3 rot,
                                  Vector3 scale) {
  Shader oldShader = model.materials[0].shader;
  model.materials[0].shader = m_ShadowMap.DepthShader;

  DrawModelEx(model, pos, {0, 1, 0}, rot.y * RAD2DEG, scale, WHITE);
  model.materials[0].shader = oldShader;
}

void RuntimeScene::DrawDeptScene() {
  GroupEntity<CubeComponent>(
      [this](auto entity, auto &comp, auto &transform, auto id) {
  DrawDepthModel(m_CubeModel, transform.Translation, transform.Rotation, transform.Scale);
      });

  GroupEntity<PlaneComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
          DrawDepthModel(m_PlaneModel, transform.Translation, transform.Rotation, transform.Scale);
  });

  GroupEntity<SphereComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
        DrawDepthModel(m_SphereModel, transform.Translation, transform.Rotation,
                       transform.Scale);
      });

  GroupEntity<ModelComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
        if (!ResourceManager::Has<Model>(comp.ModelPath))
          return;
        
        DrawDepthModel(ResourceManager::Get<Model>(comp.ModelPath), transform.Translation, transform.Rotation, transform.Scale);
      });

}

void RuntimeScene::ShadowPass() {
  LightComponent *shadowLight = nullptr;

  GroupEntity<LightComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
        if(comp.Type == LightType::Directional && comp.CastShadow){
          shadowLight = &comp;	  
	}
      });

  if (!shadowLight)
    return;

  Vector3 lightDir = Vector3Normalize(shadowLight->Direction);
  Vector3 lightPos = Vector3Scale(lightDir, -10.0f);

  Camera lightCam = {0};
  lightCam.position = lightPos;
  lightCam.target = {0, 0, 0};
  lightCam.up = {0, 1, 0};
  lightCam.fovy = 30.0f;
  lightCam.projection = CAMERA_ORTHOGRAPHIC;

  m_ShadowMap.LightView =
      MatrixLookAt(lightCam.position, lightCam.target, lightCam.up);
  m_ShadowMap.LightProjection = MatrixOrtho(-30, 30, -30, 30, 0.1f, 100.0f);
  m_ShadowMap.LightSpaceMatrix =
      MatrixMultiply(m_ShadowMap.LightView, m_ShadowMap.LightProjection);

  BeginTextureMode(m_ShadowMap.RenderTexture);
  ClearBackground(WHITE);
  BeginMode3D(lightCam);
  rlDisableBackfaceCulling();
  DrawDeptScene();
  rlEnableBackfaceCulling();
  EndMode3D();
  EndTextureMode();
}

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

  auto view =
      GetRegistry()
          .view<RigidbodyComponent, BoxColliderComponent, TransformComponent>();

  for (auto entity : view) {
    auto& rn = view.get<RigidbodyComponent>(entity);
    auto& bc = view.get<BoxColliderComponent>(entity);
    auto &tr = view.get<TransformComponent>(entity);

    m_PhysicsEngine.CreateBody(entity, rn, tr, *this);
  }
}

//--------------------------------------------------------------------
// Scene::OnRuntimeStop
//--------------------------------------------------------------------
void RuntimeScene::OnRuntimeStop() {
  m_PhysicsEngine.Clear();
}


//--------------------------------------------------------------------
// Scene::OnUpdate
//--------------------------------------------------------------------
void RuntimeScene::OnUpdate(float ts) {
  ShadowPass();
  BeginTextureMode(m_ViewTexture);

  ClearBackground(SKYBLUE);

  m_PhysicsEngine.Update(ts, *this);

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

        sol::protected_function onUpdate = comp.Instance["OnUpdate"];

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


    int lightIndex = 0;

    GroupEntity<LightComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
	  ShaderLight light;
	  light.enabled = 1;
	  light.type = (int)comp.Type;
	  light.position = transform.Translation;
	  light.direction = comp.Direction;
	  light.color = { comp.ColorValue.r / 255.0f, comp.ColorValue.g / 255.0f, 
			  comp.ColorValue.b / 255.0f, comp.ColorValue.a / 255.0f };
	  light.intensity = comp.Intensity;
          light.range = comp.Range;
          float cosAngle = cosf(comp.SpotAngle * DEG2RAD);

	  light.spotAngle = cosAngle;

	  m_LightShaderCache.UploadLight(lightIndex, light);
	  lightIndex++;
	  
        });

    m_LightShaderCache.SetLightCount(lightIndex);
    m_LightShaderCache.SetShadowData(m_ShadowMap.LightSpaceMatrix, m_ShadowMap.RenderTexture.depth, true);
  

  GroupEntity<CubeComponent>(
      [this](auto entity, auto &comp, auto &transform, auto id) {
	if (comp.useSceneLighting)
	  {
	    m_CubeModel.materials[0].shader = m_LightShader;

	  }else {
	  m_CubeModel.materials[0].shader = m_DefaultShader;
          }

          Matrix matRotation = MatrixRotateXYZ(
              {transform.Rotation.x * RAD2DEG, transform.Rotation.y * RAD2DEG,
               transform.Rotation.z * RAD2DEG});

          m_CubeModel.transform = MatrixMultiply(MatrixScale(transform.Scale.x, transform.Scale.y, transform.Scale.z),
                                                   matRotation);

	DrawModel(m_CubeModel, transform.Translation, 1.0f, comp.Tint);
      });

  GroupEntity<PlaneComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
	  if (comp.useSceneLighting){
	    m_PlaneModel.materials[0].shader = m_LightShader;
          } else {            
	    m_PlaneModel.materials[0].shader = m_DefaultShader;
          }

          Matrix matRotation = MatrixRotateXYZ(
              {transform.Rotation.x * RAD2DEG, transform.Rotation.y * RAD2DEG,
               transform.Rotation.z * RAD2DEG});

          m_PlaneModel.transform = MatrixMultiply(MatrixScale(transform.Scale.x, transform.Scale.y, transform.Scale.z),
                                                   matRotation);

          DrawModel(m_PlaneModel, transform.Translation, 1.0f, comp.Tint);   
	});

  GroupEntity<SphereComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
	if (comp.useSceneLighting){
	  m_SphereModel.materials[0].shader = m_LightShader;	   
	}
	else {
	  m_SphereModel.materials[0].shader = m_DefaultShader;
        }

          Matrix matRotation = MatrixRotateXYZ(
              {transform.Rotation.x * RAD2DEG, transform.Rotation.y * RAD2DEG,
               transform.Rotation.z * RAD2DEG});

          m_SphereModel.transform = MatrixMultiply(MatrixScale(transform.Scale.x, transform.Scale.y, transform.Scale.z),
                                                   matRotation);

	        DrawModel(m_SphereModel, transform.Translation, 1.0f, comp.Tint);
      });

      GroupEntity<ModelComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
          if (!ResourceManager::Has<Model>(comp.ModelPath))
	    return;
          // todo: trim comp.ModelPath
          Model &model = ResourceManager::Get<Model>(comp.ModelPath);

	  if(comp.useSceneLighting){
            for (int i = 0; i < model.materialCount; i++)
	      model.materials[i].shader = m_LightShader;
          }
	  else{
	    for (int i = 0; i < model.materialCount; i++)
              model.materials[i].shader = m_DefaultShader;
          }

          Matrix matRotation = MatrixRotateXYZ(
              {transform.Rotation.x * RAD2DEG, transform.Rotation.y * RAD2DEG,
               transform.Rotation.z * RAD2DEG});

          model.transform = MatrixMultiply(MatrixScale(transform.Scale.x, transform.Scale.y, transform.Scale.z),
                                                   matRotation);

          DrawModel(model, transform.Translation, 1.0f, comp.Tint); 
	});



  if(CamPresent)
    EndMode3D();

  DrawFPS(10, 10);

  EndTextureMode();

  FlushEntityDestruction();
}
