#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/ContactListener.h>

#include <entt/entt.hpp>
#include <unordered_map>

#include "Components.h"
#include <raylib.h>

class Scene;

namespace Layers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr uint32_t NUM_LAYERS = 2;
}

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        m_ObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_ObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    uint32_t GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return m_ObjectToBroadPhase[layer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
        if (layer == BroadPhaseLayers::NON_MOVING) return "NON_MOVING";
        if (layer == BroadPhaseLayers::MOVING) return "MOVING";
        return "UNKNOWN";
    }
#endif

private:
    JPH::BroadPhaseLayer m_ObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer broadPhaseLayer) const override
    {
        switch (layer)
        {
            case Layers::NON_MOVING:
                return broadPhaseLayer == BroadPhaseLayers::MOVING;

            case Layers::MOVING:
                return true;

            default:
                return false;
        }
    }
};

class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override
    {
        if (a == Layers::MOVING && b == Layers::MOVING) return true;
        if (a == Layers::MOVING && b == Layers::NON_MOVING) return true;
        if (a == Layers::NON_MOVING && b == Layers::MOVING) return true;

        return false;
    }
};

class PhysicsEngine
{
public:
    PhysicsEngine();
    ~PhysicsEngine();

    static void ToggleDebugDraw() { s_DebugDrawEnabled = !s_DebugDrawEnabled; }
    static bool IsDebugDrawEnabled() { return s_DebugDrawEnabled; }

    void CreateBody(entt::entity entity, const RigidbodyComponent &rb,
                    const TransformComponent &transform, Scene &scene);

    void CreateCharacter(Scene &scene);

    void DestroyBody(entt::entity entity);
    void Clear();

    void Update(float deltaTime, Scene& scene);

private:
    JPH::PhysicsSystem m_PhysicsSystem;

    BPLayerInterfaceImpl m_BpLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl m_ObjectBpFilter;
    ObjectLayerPairFilterImpl m_ObjectLayerPairFilter;

    JPH::TempAllocatorImpl* m_TempAllocator = nullptr;
    JPH::JobSystemThreadPool* m_JobSystem = nullptr;

    std::unordered_map<entt::entity, JPH::BodyID> m_BodyMap;
    std::unordered_map<entt::entity, JPH::CharacterVirtual *> m_CharacterMap;

    static bool s_DebugDrawEnabled;
};