#include "PhysicsEngine.h"
#include "raymath.h"
#include "Components.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Scene.h"
#include "Entity.h"

bool PhysicsEngine::s_DebugDrawEnabled = false;

static JPH::Vec3 ToJolt(Vector3 v)
{
    return JPH::Vec3(v.x, v.y, v.z);
}

static Vector3 FromJolt(JPH::Vec3 v)
{
    return Vector3{ v.GetX(), v.GetY(), v.GetZ() };
}

PhysicsEngine::PhysicsEngine()
{
    JPH::RegisterDefaultAllocator();

    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    m_TempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

    uint32_t threadCount = std::max(1u, std::thread::hardware_concurrency() - 1);

    m_JobSystem = new JPH::JobSystemThreadPool(
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        threadCount
    );

    const uint32_t maxBodies = 1024;
    const uint32_t numBodyMutexes = 0;
    const uint32_t maxBodyPairs = 1024;
    const uint32_t maxContactConstraints = 1024;

    m_PhysicsSystem.Init(
        maxBodies,
        numBodyMutexes,
        maxBodyPairs,
        maxContactConstraints,
        m_BpLayerInterface,
        m_ObjectBpFilter,
        m_ObjectLayerPairFilter
    );

    m_PhysicsSystem.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
}

PhysicsEngine::~PhysicsEngine()
{
    auto& bodyInterface = m_PhysicsSystem.GetBodyInterface();

    for (auto& [entity, bodyID] : m_BodyMap)
    {
        bodyInterface.RemoveBody(bodyID);
        bodyInterface.DestroyBody(bodyID);
    }

    m_BodyMap.clear();

    delete m_JobSystem;
    delete m_TempAllocator;

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void PhysicsEngine::CreateCharacter(Scene &scene) {
  scene.GroupEntity<CharacterComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
        if(m_CharacterMap.contains(entity))
          return;

        JPH::Ref<JPH::Shape> shape;
        BoxColliderComponent box{};    

    if (scene.GetRegistry().all_of<BoxColliderComponent>(entity)) {
      auto &box = scene.GetRegistry().get<BoxColliderComponent>(entity);
      shape = new JPH::BoxShape(ToJolt(Vector3Multiply(box.Size, transform.Scale) * 0.5f));
    }
    else if (scene.GetRegistry().all_of<SphereColliderComponent>(entity)) {
      auto &sphere = scene.GetRegistry().get<SphereColliderComponent>(entity);
      shape = new JPH::SphereShape(sphere.Radius * transform.Scale.x); // Assuming uniform scaling for spheres
    }
    else if (scene.GetRegistry().all_of<CapsuleColliderComponent>(entity)) {
      auto &capsule = scene.GetRegistry().get<CapsuleColliderComponent>(entity);
      shape = new JPH::CapsuleShape(capsule.HalfHeight, capsule.Radius); // Assuming uniform scaling for radius and height
    } else {
      Vector3 finalSize = {
        box.Size.x * transform.Scale.x,
        box.Size.y * transform.Scale.y,
        box.Size.z * transform.Scale.z
    };

    JPH::Vec3 halfExtent = JPH::Vec3(
        finalSize.x * 0.5f,
        finalSize.y * 0.5f,
        finalSize.z * 0.5f
    );
      // Default to a unit box if no collider is found
      shape = new JPH::BoxShape(halfExtent);
    }

    JPH::CharacterVirtualSettings settings;
    settings.mShape = shape;
    settings.mMaxSlopeAngle = JPH::DegreesToRadians(comp.MaxSlopeAngle);
    // settings.mMaxSte

    comp.RuntimeCharacter =
        new JPH::CharacterVirtual(&settings, ToJolt(transform.Translation),
                                  JPH::Quat::sIdentity(), &m_PhysicsSystem);
    m_CharacterMap[entity] = comp.RuntimeCharacter;
        
  });
}

void PhysicsEngine::CreateBody(entt::entity entity,
                               const RigidbodyComponent &rb,
                               const TransformComponent &transform,
                               Scene& scene
)
{
    if (m_BodyMap.contains(entity))
      return;

    BoxColliderComponent box{};    

    // JPH::Ref<JPH::Shape> shape = new JPH::BoxShape(halfExtent);
    JPH::Ref<JPH::Shape> shape;

    if (scene.GetRegistry().all_of<BoxColliderComponent>(entity)) {
      auto &box = scene.GetRegistry().get<BoxColliderComponent>(entity);
      shape = new JPH::BoxShape(ToJolt(Vector3Multiply(box.Size, transform.Scale) * 0.5f));
    }
    else if (scene.GetRegistry().all_of<SphereColliderComponent>(entity)) {
      auto &sphere = scene.GetRegistry().get<SphereColliderComponent>(entity);
      shape = new JPH::SphereShape(sphere.Radius * transform.Scale.x); // Assuming uniform scaling for spheres
    }
    else if (scene.GetRegistry().all_of<CapsuleColliderComponent>(entity)) {
      auto &capsule = scene.GetRegistry().get<CapsuleColliderComponent>(entity);
      shape = new JPH::CapsuleShape(capsule.HalfHeight, capsule.Radius); // Assuming uniform scaling for radius and height
    } else {
      Vector3 finalSize = {
        box.Size.x * transform.Scale.x,
        box.Size.y * transform.Scale.y,
        box.Size.z * transform.Scale.z
    };

    JPH::Vec3 halfExtent = JPH::Vec3(
        finalSize.x * 0.5f,
        finalSize.y * 0.5f,
        finalSize.z * 0.5f
    );
      // Default to a unit box if no collider is found
      shape = new JPH::BoxShape(halfExtent);
    }

    JPH::EMotionType motionType = JPH::EMotionType::Dynamic;
    JPH::ObjectLayer layer = Layers::MOVING;

    if (rb.Type == BodyType::Static)
    {
        motionType = JPH::EMotionType::Static;
        layer = Layers::NON_MOVING;
    }
    else if (rb.Type == BodyType::Kinematic)
    {
        motionType = JPH::EMotionType::Kinematic;
        layer = Layers::MOVING;
    }

    Vector3 pos = Vector3Add(transform.Translation, box.Offset);

    JPH::BodyCreationSettings settings(
        shape,
        ToJolt(pos),
        JPH::Quat::sIdentity(),
        motionType,
        layer
    );

    settings.mFriction = 0.7f;
    settings.mRestitution = 0.1f;
    settings.mGravityFactor = rb.useGravity ? 1.0f : 0.0f;
    settings.mRestitution = rb.Restitution;
    settings.mFriction = rb.Friction;
    settings.mLinearDamping = rb.LinearDamping;
    settings.mAngularDamping = rb.AngularDamping;
    // settings.mAllowedDOFs = ;


    if (rb.Type == BodyType::Dynamic)
    {
        settings.mOverrideMassProperties =
            JPH::EOverrideMassProperties::CalculateInertia;

        settings.mMassPropertiesOverride.mMass = rb.Mass;
    }

    auto& bodyInterface = m_PhysicsSystem.GetBodyInterface();

    JPH::BodyID bodyID = bodyInterface.CreateAndAddBody(
        settings,
        rb.Type == BodyType::Static
            ? JPH::EActivation::DontActivate
            : JPH::EActivation::Activate
    );

    if (rb.Type == BodyType::Dynamic)
    {
        bodyInterface.SetLinearVelocity(bodyID, ToJolt(rb.Velocity));
        bodyInterface.SetAngularVelocity(bodyID, ToJolt(rb.AngularVelocity));
    }


    m_BodyMap[entity] = bodyID;
}

void PhysicsEngine::Clear()
{
    auto& bodyInterface = m_PhysicsSystem.GetBodyInterface();

    for (auto& [entity, bodyID] : m_BodyMap)
    {
        bodyInterface.RemoveBody(bodyID);
        bodyInterface.DestroyBody(bodyID);
    }

    m_BodyMap.clear();
}

void PhysicsEngine::DestroyBody(entt::entity entity)
{
    auto it = m_BodyMap.find(entity);
    if (it == m_BodyMap.end())
        return;

    auto& bodyInterface = m_PhysicsSystem.GetBodyInterface();

    bodyInterface.RemoveBody(it->second);
    bodyInterface.DestroyBody(it->second);

    m_BodyMap.erase(it);
}

void PhysicsEngine::Update(float deltaTime, Scene& scene)
{
    auto view = scene.GetRegistry().view<RigidbodyComponent, TransformComponent>();

    auto& bodyInterface = m_PhysicsSystem.GetBodyInterface();

    // 1. Create missing bodies
    for (auto entity : view)
    {
        auto& rb = view.get<RigidbodyComponent>(entity);
        auto &transform = view.get<TransformComponent>(entity);        

        if (!m_BodyMap.contains(entity) && !m_CharacterMap.contains(entity))
          CreateBody(entity, rb, transform, scene);
    }    

    // 2. ECS -> Jolt BEFORE simulation
    for (auto entity : view)
    {
        auto& rb = view.get<RigidbodyComponent>(entity);
        auto &transform = view.get<TransformComponent>(entity);
        ColliderComponent *collider = nullptr;
        Vector3 Offset;

        if(Entity(entity, &scene).HasComponent<BoxColliderComponent>()) {
          auto &box = scene.GetRegistry().get<BoxColliderComponent>(entity);
          Offset = box.Offset;
        } else if(Entity(entity, &scene).HasComponent<SphereColliderComponent>()) {
            auto& sphere = scene.GetRegistry().get<SphereColliderComponent>(entity);
            Offset = sphere.Offset;
        } else if(Entity(entity, &scene).HasComponent<CapsuleColliderComponent>()) {
            auto& capsule = scene.GetRegistry().get<CapsuleColliderComponent>(entity);
            Offset = capsule.Offset;
        } else {
          Offset = Vector3{0, 0, 0};
        }       

        auto it = m_BodyMap.find(entity);
        if (it == m_BodyMap.end())
            continue;

        if (!rb.Dirty)
            continue;

        JPH::BodyID bodyID = it->second;        

        Vector3 pos = Vector3Add(transform.Translation, Offset);

        JPH::EActivation activation =
            rb.Type == BodyType::Static
                ? JPH::EActivation::DontActivate
                : JPH::EActivation::Activate;

        bodyInterface.SetPositionAndRotation(
            bodyID,
            ToJolt(pos),
            JPH::Quat::sEulerAngles(ToJolt(transform.Rotation)),
            activation
        );

        if (rb.Type == BodyType::Dynamic)
        {
            bodyInterface.SetLinearVelocity(bodyID, ToJolt(rb.Velocity));
            bodyInterface.SetAngularVelocity(bodyID, ToJolt(rb.AngularVelocity));
        }

        rb.Dirty = false;
    }

    // 3. Step physics
    m_PhysicsSystem.Update(
        deltaTime,
        1,
        m_TempAllocator,
        m_JobSystem
    );

    // 4. Jolt -> ECS after simulation
    for (auto entity : view)
    {
        auto& rb = view.get<RigidbodyComponent>(entity);
        auto &transform = view.get<TransformComponent>(entity);
        ColliderComponent *collider = nullptr;
        Vector3 Offset;

        if(Entity(entity, &scene).HasComponent<BoxColliderComponent>()) {
          auto &box = scene.GetRegistry().get<BoxColliderComponent>(entity);
          Offset = box.Offset;
        } else if(Entity(entity, &scene).HasComponent<SphereColliderComponent>()) {
            auto& sphere = scene.GetRegistry().get<SphereColliderComponent>(entity);
            Offset = sphere.Offset;
        } else if(Entity(entity, &scene).HasComponent<CapsuleColliderComponent>()) {
            auto& capsule = scene.GetRegistry().get<CapsuleColliderComponent>(entity);
            Offset = capsule.Offset;
        }

        auto it = m_BodyMap.find(entity);
        if (it == m_BodyMap.end())
            continue;

        if (rb.Type == BodyType::Static)
            continue;

        JPH::BodyID bodyID = it->second;

        JPH::RVec3 pos = bodyInterface.GetCenterOfMassPosition(bodyID);

        transform.Translation = Vector3Subtract(
            Vector3{
                (float)pos.GetX(),
                (float)pos.GetY(),
                (float)pos.GetZ()
            },
            Offset
        );

        JPH::Quat rot = bodyInterface.GetRotation(bodyID);
        JPH::Vec3 euler = rot.GetEulerAngles();

        transform.Rotation = {
            euler.GetX(),
            euler.GetY(),
            euler.GetZ()
        };

        rb.Velocity = FromJolt(bodyInterface.GetLinearVelocity(bodyID));
        rb.AngularVelocity = FromJolt(bodyInterface.GetAngularVelocity(bodyID));
    }

    for (auto [entity, ch, transform] : scene.GetRegistry().view<CharacterComponent, TransformComponent>().each()) {
    // 1. Appliquer la gravité manuellement au vecteur de mouvement
    JPH::Vec3 velocity = ch.RuntimeCharacter->GetLinearVelocity();
    velocity += m_PhysicsSystem.GetGravity() * deltaTime;
    
    // 2. Mettre à jour et résoudre les collisions
    ch.RuntimeCharacter->Update(deltaTime, velocity, m_PhysicsSystem.GetDefaultBroadPhaseLayerFilter(Layers::MOVING), 
                                m_PhysicsSystem.GetDefaultLayerFilter(Layers::MOVING), {}, {}, *m_TempAllocator);
    
    // 3. Synchroniser la position ECS
    transform.Translation = FromJolt(ch.RuntimeCharacter->GetPosition());
}
}