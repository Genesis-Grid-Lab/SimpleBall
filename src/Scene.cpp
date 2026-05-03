#include "Scene.h"
#include "Components.h"
#include "Entity.h"
#include "RuntimeScene.h"

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
  CopyComponentIfExists<BoxColliderComponent>(newEntity, entity);
  
  // Audio
  CopyComponentIfExists<AudioListenerComponent>(newEntity, entity);
  CopyComponentIfExists<AudioSourceComponent>(newEntity, entity);
  
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
  CopyComponent<BoxColliderComponent>(dstSceneRegistry, srcSceneRegistry,
                                      enttMap);

  // Audio
  CopyComponent<AudioListenerComponent>(dstSceneRegistry, srcSceneRegistry,
                                        enttMap);
  CopyComponent<AudioSourceComponent>(dstSceneRegistry, srcSceneRegistry,
                                      enttMap);

  // Scripting
  CopyComponent<NativeScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
  CopyComponent<LuaScriptComponent>(dstSceneRegistry, srcSceneRegistry,
                                    enttMap);

  // Editor-only
  CopyComponent<EditorOnlyComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
  return newScene;
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
void Scene::OnComponentAdded(Entity entity, BoxColliderComponent &component) {}

// Audio
template <>
void Scene::OnComponentAdded(Entity entity, AudioListenerComponent &component) {
}

template <>
void Scene::OnComponentAdded(Entity entity, AudioSourceComponent &component){}

// Scripting
template <>
void Scene::OnComponentAdded(Entity entity, NativeScriptComponent &componet) {}

template <>
void Scene::OnComponentAdded(Entity entity, LuaScriptComponent& component){}
