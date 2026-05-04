#include "Scene.h"
#include "Components.h"
#include "Entity.h"
#include "RuntimeScene.h"
#include "ResourceManager.h"

template <typename Comp>
static void CopyComponent(entt::registry &dst, entt::registry &src, const std::unordered_map<UUID, entt::entity> &enttMap){
  auto view = src.view<Comp>();
  for (auto e : view) {
    UUID uuid = src.get<IDComponent>(e).ID;
    // ASSERT(enttMap.find(uuid) != enttMap.end());
    entt::entity dstEnttID = enttMap.at(uuid);

    auto &component = src.get<Comp>(e);
    dst.emplace_or_replace<Comp>(dstEnttID, component);
  }
}

template <typename Comp>
static void CopyComponentIfExists(Entity dst, Entity src) {
  if (src.HasComponent<Comp>())
    dst.AddOrReplaceComponent<Comp>(src.GetComponent<Comp>());
}

Entity Scene::CreateEntity(const std::string &name) {
  return CreateEntityWithUUID(UUID(), name);
}

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string &name) {
  Entity entity = {m_Registry.create(), this};
  entity.AddComponent<IDComponent>(uuid);
  entity.AddComponent<TransformComponent>();
  auto &tag = entity.AddComponent<TagComponent>();
  tag.Tag = name.empty() ? "Entity" : name;
  return entity;
}

void Scene::DestroyEntityNow(Entity entity) { m_Registry.destroy(entity); }

void Scene::DestroyEntity(Entity entity) {
  m_DestroyQueue.push_back((entt::entity)entity);
  if(entity.HasComponent<RigidbodyComponent>() && entity.HasComponent<BoxColliderComponent>()) {
    m_PhysicsEngine.DestroyBody((entt::entity)entity);
  }
}

void Scene::FlushEntityDestruction(){
  if (m_DestroyQueue.empty())
    return;
  for (auto e : m_DestroyQueue)
    if (m_Registry.valid(e))
      m_Registry.destroy(e);

  m_DestroyQueue.clear();
}

void Scene::DuplicateEntity(Entity entity) {
  auto name = entity.GetName();
  auto newEntity = CreateEntity(name);

  // Core
  CopyComponentIfExists<TransformComponent>(newEntity, entity);
  
  // Rendering
  CopyComponentIfExists<CubeComponent>(newEntity, entity);
  CopyComponentIfExists<PlaneComponent>(newEntity, entity);
  CopyComponentIfExists<SphereComponent>(newEntity, entity);
  CopyComponentIfExists<ModelComponent>(newEntity, entity);
  CopyComponentIfExists<SpriteComponent>(newEntity, entity);
  
  // Camera
  CopyComponentIfExists<CameraComponent>(newEntity, entity);
  
  // Lighting
  CopyComponentIfExists<LightComponent>(newEntity, entity);
  
  // Physics
  CopyComponentIfExists<RigidbodyComponent>(newEntity, entity);
  CopyComponentIfExists<CharacterComponent>(newEntity, entity);
  CopyComponentIfExists<BoxColliderComponent>(newEntity, entity);
  CopyComponentIfExists<SphereColliderComponent>(newEntity, entity);
  CopyComponentIfExists<CapsuleColliderComponent>(newEntity, entity);
  
  // Audio
  CopyComponentIfExists<AudioListenerComponent>(newEntity, entity);
  CopyComponentIfExists<AudioSourceComponent>(newEntity, entity);

  // Animation
  CopyComponentIfExists<AnimationComponent>(newEntity, entity);
  
  // Scripting
  CopyComponentIfExists<NativeScriptComponent>(newEntity, entity);
  CopyComponentIfExists<LuaScriptComponent>(newEntity, entity);
  
  // Editor-only
  CopyComponentIfExists<EditorOnlyComponent>(newEntity, entity);
}

template <typename T>
Ref<T> Scene::Copy(const Ref<Scene> &other) {
  Ref<T> newScene = CreateRef<T>();

  auto &srcSceneRegistry = other->m_Registry;
  auto &dstSceneRegistry = newScene->m_Registry;
  std::unordered_map<UUID, entt::entity> enttMap;

  // Create entities in new scene
  auto idView = srcSceneRegistry.view<IDComponent>();
  for (auto e : idView) {
    UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
    const auto &name = srcSceneRegistry.get<TagComponent>(e).Tag;
    Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
    enttMap[uuid] = (entt::entity)newEntity;
  }

  // Copy components (except IDComponent and TagComponent)
  // Core
  CopyComponent<TransformComponent>(dstSceneRegistry, srcSceneRegistry,
                                    enttMap);

  // Rendering
  CopyComponent<CubeComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
  CopyComponent<PlaneComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
  CopyComponent<SphereComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
  CopyComponent<ModelComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
  CopyComponent<SpriteComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);

  // Camera
  CopyComponent<CameraComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);

  // Lighting
  CopyComponent<LightComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);

  // Physics
  CopyComponent<RigidbodyComponent>(dstSceneRegistry, srcSceneRegistry,
                                    enttMap);
  CopyComponent<CharacterComponent>(dstSceneRegistry, srcSceneRegistry,
                                    enttMap);
  CopyComponent<BoxColliderComponent>(dstSceneRegistry, srcSceneRegistry,
                                      enttMap);
  CopyComponent<SphereColliderComponent>(dstSceneRegistry, srcSceneRegistry,
                                         enttMap);
  CopyComponent<CapsuleColliderComponent>(dstSceneRegistry, srcSceneRegistry,
                                         enttMap);
  // Audio
  CopyComponent<AudioListenerComponent>(dstSceneRegistry, srcSceneRegistry,
                                        enttMap);
  CopyComponent<AudioSourceComponent>(dstSceneRegistry, srcSceneRegistry,
                                      enttMap);

  // Animation
  CopyComponent<AnimationComponent>(dstSceneRegistry, srcSceneRegistry,
                                  enttMap);

  // Scripting
  CopyComponent<NativeScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
  CopyComponent<LuaScriptComponent>(dstSceneRegistry, srcSceneRegistry,
                                    enttMap);

  // Editor-only
  CopyComponent<EditorOnlyComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
  return newScene;
}

void Scene::UpdateAnimationSystem(float ts) {
  auto view = m_Registry.view<AnimationComponent, ModelComponent>();

  for(auto entity : view) {
    auto &animation = view.get<AnimationComponent>(entity);
    auto &modelComp = view.get<ModelComponent>(entity); // Le composant de l'entité

    if(!animation.IsPlaying || animation.AnimationPath.empty())
      continue;

    // 1. Récupération sécurisée des animations    
    if (!modelComp.Loaded) {
      if(!ResourceManager::Has<Model>(modelComp.ModelPath)) {
        // ASSERT(false, "Model not found for animation");
        modelComp.model = ResourceManager::Load<Model>(modelComp.ModelPath, modelComp.ModelPath);
      }else {
        modelComp.model = ResourceManager::Get<Model>(modelComp.ModelPath);
      }      
      modelComp.Loaded = true;
    }

    if (!animation.Loaded) {
      if (!ResourceManager::Has<ModelAnimation *>(animation.AnimationPath)) {
          animation.AnimsPtr = ResourceManager::Load<ModelAnimation *>(animation.AnimationPath, animation.AnimationPath);
          // Tu devrais aussi stocker le count quelque part lors du Load
      } else {
        
        animation.AnimsPtr = ResourceManager::Get<ModelAnimation *>(animation.AnimationPath);
      }
      animation.AnimationsCount = 4; // A adapter selon comment tu stockes ça    
      animation.Loaded = true;
    }



    if (!animation.AnimsPtr) continue;

    ModelAnimation &currentAnim = animation.AnimsPtr[animation.CurrentAnimationIndex];

    // 2. Gestion du timing
    animation.FrameTime += ts * animation.Speed;
    float frameDuration = 1.0f / 24.0f; // Ajuste selon ton export (souvent 30.0f)

    if(animation.FrameTime >= frameDuration) {
        animation.FrameTime = 0.0f;
        animation.CurrentFrame = (animation.CurrentFrame + 1) % currentAnim.keyframeCount;

        // 3. Mise à jour : Utilise le modèle SPECIFIQUE à cette entité
        // Si modelComp.Model est une copie unique, ça marchera.
        // Si c'est une référence au manager, ils bougeront tous pareil.
        UpdateModelAnimation(modelComp.model, currentAnim,
                             animation.CurrentFrame);

        // UpdateModelAnimation(modelComp.model, currentAnim, 10);        

      }      
  }

}


template Ref<RuntimeScene> Scene::Copy<RuntimeScene>(const Ref<Scene>&);
// template Ref<EditorScene> Scene::Copy<EditorScene>(const Ref<Scene>&);

// Core
template <typename T>
void Scene::OnComponentAdded(Entity entity, T &component) {}

template <>
void Scene::OnComponentAdded(Entity entity, IDComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity, TransformComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity, TagComponent &component) {}

// Rendering
template <>
void Scene::OnComponentAdded(Entity entity, CubeComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity, PlaneComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity, SphereComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity, ModelComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity, SpriteComponent &component) {}

// Camera
template <>
void Scene::OnComponentAdded(Entity entity, CameraComponent &component) {}

// Lighting
template <>
void Scene::OnComponentAdded(Entity entity, LightComponent &component) {}

// Physics
template <>
void Scene::OnComponentAdded(Entity entity, RigidbodyComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity, CharacterComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity, BoxColliderComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity,
                             SphereColliderComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity,
                             CapsuleColliderComponent &component) {}

// Audio
template <>
void Scene::OnComponentAdded(Entity entity, AudioListenerComponent &component) {
}

template <>
void Scene::OnComponentAdded(Entity entity, AudioSourceComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity, AnimationComponent &component) {}

// Scripting
template <>
void Scene::OnComponentAdded(Entity entity, NativeScriptComponent &componet) {}

template <>
void Scene::OnComponentAdded(Entity entity, LuaScriptComponent &component) {}

template <>
void Scene::OnComponentAdded(Entity entity, EditorOnlyComponent &component) {}
