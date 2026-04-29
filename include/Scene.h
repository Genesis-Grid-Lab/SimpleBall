#pragma once
#include "Components.h"
#include <entt.hpp>

class Entity;

class Scene {
public:
  virtual ~Scene() = default;

  Entity CreateEntity(const std::string &name = std::string()); // Create entity with random UUID
  Entity CreateEntityWithUUID(UUID uuid,
                              const std::string &name = std::string()); // Create entity with givin UUID

  void DestroyEntity(Entity entity); // Add entity to pool for deletions
  void
  DestroyEntityNow(Entity entity); // Destroy entity with out adding to pool
  void FlushEntityDestruction(); // call at end of update

  void DuplicateEntity(Entity entity);

  template <typename T> static Ref<T> Copy(const Ref<Scene> &other);

  template <typename Entt, typename Comp, typename Task>
  void ViewEntity(Task &&task) {
    /* ASSERT(std::is_base_of<Entity, Entt>::value, "error viewing entt"); */
    m_Registry.view<Comp>().each([this, &task](const auto entity, auto &comp) {
      task(std::move(Entt(entity, this)), comp);
    });
  }

  template <typename Comp, typename Task> void GroupEntity(Task &&task) {
    auto group = m_Registry.group<Comp>(entt::get<TransformComponent>);
    for (auto entity : group) {
      auto &comp = group.template get<Comp>(entity);
      auto &transform = group.template get<TransformComponent>(entity);
      task(std::move(Entity(entity, this)), comp, transform, entity);
    }
  }

  virtual void OnUpdate(float ts) = 0;

  entt::registry &GetRegistry() { return m_Registry; }  
private:
  template <typename T> void OnComponentAdded(Entity entity, T &component);
protected:
  friend class Entity;

  entt::registry m_Registry;
  std::vector<entt::entity>
      m_DestroyQueue; // queued destroys flushed after update
  
};
