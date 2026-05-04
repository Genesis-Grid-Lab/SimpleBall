#include "SceneHierarchyPanel.h"
#include "Components.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstring>
#include "ScriptableEntity.h"
#include "imgui_stdlib.h"
#include "raylib.h"

//----------------------
// Samll UI helpers
//----------------------

static bool DrawColorControl(const char* label, Color& color)
{
    float col[4] = {
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f,
        color.a / 255.0f
    };

    if (ImGui::ColorEdit4(label, col))
    {
        color.r = (unsigned char)(col[0] * 255.0f);
        color.g = (unsigned char)(col[1] * 255.0f);
        color.b = (unsigned char)(col[2] * 255.0f);
        color.a = (unsigned char)(col[3] * 255.0f);
        return true;
    }

    return false;
}

static bool DrawVec3Control(const char *label, Vector3 &values,
                            float resetValue = 0.0f,
                            float columnWidth = 100.0f) {
  bool changed = false;

  ImGui::PushID(label);
  ImGui::PushID(&values);

  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, columnWidth);
  ImGui::TextUnformatted(label);
  ImGui::NextColumn();

  ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

  const float lineHeight =
      ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
  const ImVec2 btn = {lineHeight + 3.0f, lineHeight};

  auto axis = [&](const char *axisText, float &v, const ImVec4 &col,
                  const char *dragID) {
    ImGui::PushStyleColor(ImGuiCol_Button, col);
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4{col.x + 0.1f, col.y + 0.1f, col.z + 0.1f, col.w});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
    if (ImGui::Button(axisText, btn)) {
      v = resetValue;
      changed = true;
    }

    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    changed |= ImGui::DragFloat(dragID, &v, 0.1f); // <- unique per axis    
    ImGui::PopItemWidth();
    ImGui::SameLine();
  };

  axis("X", values.x, ImVec4{0.8f, 0.1f, 0.15f, 1.0f}, "##X");
  axis("Y", values.y, ImVec4{0.2f, 0.7f, 0.2f, 1.0f}, "##Y");
  axis("Z", values.z, ImVec4{0.1f, 0.25f, 0.8f, 1.0f}, "##Z");

  ImGui::PopStyleVar();
  ImGui::Columns(1);

  ImGui::PopID(); // &values
  ImGui::PopID(); // label

  return changed;
}


static bool DrawCameraControl(Camera3D& camera)
{
    bool changed = false;

    changed |= DrawVec3Control("Target", camera.target);
    changed |= DrawVec3Control("Up", camera.up);

    changed |= ImGui::DragFloat("FOV", &camera.fovy, 0.1f, 1.0f, 179.0f);

    const char* projectionTypes[] = {
        "Perspective",
        "Orthographic"
    };

    int currentProjection = camera.projection;

    if (ImGui::Combo("Projection", &currentProjection, projectionTypes, 2))
    {
        camera.projection = currentProjection;
        changed = true;
    }

    return changed;
}

/** Generic foldout for components with add/remove menu on the right
    uiFn(entity, component) should return true if it changed something **/

template <typename T, typename UIFunc>
static void DrawComponent(const char *name, Entity entity, UIFunc uiFn,
                          bool removale = true) {
  if (!entity.HasComponent<T>())
    return;

  auto &comp = entity.GetComponent<T>();

  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
      ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
      ImGuiTreeNodeFlags_FramePadding;

  ImVec2 contentRegion = ImGui::GetContentRegionAvail();
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});

  bool open =
      ImGui::TreeNodeEx((void *)typeid(T).hash_code(), flags, "%s", name);
  ImGui::PopStyleVar();

  // settings button on the right
  float lineHeight =
      ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
  ImGui::SameLine(contentRegion.x - lineHeight * 0.5f);
  if (ImGui::Button("...", ImVec2{lineHeight, lineHeight}))
    ImGui::OpenPopup("ComponentSettings");

  bool removeComponent = false;
  if (ImGui::BeginPopup("ComponentSettings")) {
    if (removale && ImGui::MenuItem("Remove component"))
      removeComponent = true;
    ImGui::EndPopup();
  }

  if (open) {
    uiFn(entity, comp);
    ImGui::TreePop();
  }

  if (removeComponent)
    entity.RemoveComponent<T>();
}

//-------------------------------
// SceneHierarchypanel
//-------------------------------
void SceneHierarchyPanel::SetSelectedENtity(Entity entity) {
  m_SelectionContext = entity;
}

SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene> &context) {
  SetContext(context);
}

void SceneHierarchyPanel::SetContext(const Ref<Scene> &context) {
  m_Context = context;
  m_SelectionContext = {};
}

void SceneHierarchyPanel::Shutdown(){

  }

void SceneHierarchyPanel::Update() {
  ImGui::Begin("Scene Hierarchy");

  // List entities
  m_Context->GetRegistry().view<entt::entity>().each([&](auto eID) {
    Entity entity{eID, m_Context.get()};
    DrawEntityNode(entity);
  });

  // Blank-space context menu
  if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
    m_SelectionContext = {};

  if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight |
                                            ImGuiPopupFlags_NoOpenOverItems)) {
    if (ImGui::MenuItem("Create Empty"))
      m_SelectionContext = m_Context->CreateEntity("Empty");
    ImGui::EndPopup();
  }

  ImGui::End();

  ImGui::Begin("Properties");
  if (m_SelectionContext)
    DrawComponents(m_SelectionContext);
  ImGui::End();

  m_Context->GroupEntity<IDComponent>(
      [this](auto entity, auto &comp, auto &transform, auto id) {
        if (m_SelectionContext == entity)
          comp.Active = true;
        else {
	  comp.Active = false;
	}
      });
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity){
  auto &tag = entity.GetComponent<TagComponent>().Tag;

  ImGuiTreeNodeFlags flags =
      ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
      ImGuiTreeNodeFlags_OpenOnArrow;
  flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

  bool opened = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, flags,
                                  "%s", tag.c_str());

  if (ImGui::IsItemClicked())
    m_SelectionContext = entity;

  bool entityDeleted = false;
  if(ImGui::BeginPopupContextItem()){
    if (ImGui::MenuItem("Duplicate"))
      m_Context->DuplicateEntity(entity);
    if (ImGui::MenuItem("Delete"))
      entityDeleted = true;
    ImGui::EndPopup();
  }

  if (opened) {
    // (Children would be drawn here if you support hierarchy)
    ImGui::TreePop();
  }

  if (entityDeleted) {
    if (m_SelectionContext == entity)
      m_SelectionContext = {};
    m_Context->DestroyEntity(entity);
  }
}

static void DrawAddComponentPopup(Entity entity) {
  if (ImGui::BeginPopup("AddComponentPopup")) {
    if (!entity.HasComponent<CameraComponent>()) {
      if (ImGui::MenuItem("Camera")) {
        entity.AddComponent<CameraComponent>();
        ImGui::CloseCurrentPopup();        
      }
    }

    if(!entity.HasComponent<CubeComponent>()){
      if (ImGui::MenuItem("Cube")) {
        entity.AddComponent<CubeComponent>();
	ImGui::CloseCurrentPopup();
      }
    }

    if (!entity.HasComponent<PlaneComponent>()) {
      if (ImGui::MenuItem("Plane")) {
        entity.AddComponent<PlaneComponent>();
	ImGui::CloseCurrentPopup();
      }
    }    

    if(!entity.HasComponent<SphereComponent>()){
      if (ImGui::MenuItem("Sphere")) {
        entity.AddComponent<SphereComponent>();
	ImGui::CloseCurrentPopup();
      }
    }

    if(!entity.HasComponent<ModelComponent>()){
      if (ImGui::MenuItem("Model")) {
        entity.AddComponent<ModelComponent>();
	ImGui::CloseCurrentPopup();
      }
    }

    if (!entity.HasComponent<SpriteComponent>()) {
      if(ImGui::MenuItem("Sprite")){
        entity.AddComponent<SpriteComponent>();
	ImGui::CloseCurrentPopup();
      }
    }

    if(!entity.HasComponent<LightComponent>()){
      if (ImGui::MenuItem("Light")) {
        entity.AddComponent<LightComponent>();
	ImGui::CloseCurrentPopup();
      }
    }

    if (!entity.HasComponent<RigidbodyComponent>()) {
      if(ImGui::MenuItem("Rigidbody")){
        entity.AddComponent<RigidbodyComponent>();
	ImGui::CloseCurrentPopup();
      }
    }

    if(!entity.HasComponent<CharacterComponent>()){
      if(ImGui::MenuItem("Character")){
        entity.AddComponent<CharacterComponent>();
        ImGui::CloseCurrentPopup();          
      }
    }

    if (!entity.HasComponent<BoxColliderComponent>()) {
      if (ImGui::MenuItem("BoxCollider")) {
        entity.AddComponent<BoxColliderComponent>();
        ImGui::CloseCurrentPopup();        
      }
    }

    if(!entity.HasComponent<SphereColliderComponent>()){
      if(ImGui::MenuItem("SphereCollider")){
        entity.AddComponent<SphereColliderComponent>();
  ImGui::CloseCurrentPopup();
      }
    }

    if(!entity.HasComponent<CapsuleColliderComponent>()){
      if(ImGui::MenuItem("CapsuleCollider")){
        entity.AddComponent<CapsuleColliderComponent>();
  ImGui::CloseCurrentPopup();
      }
    }

    if(!entity.HasComponent<AudioListenerComponent>()){
      if (ImGui::MenuItem("AudioListerner")) {
        entity.AddComponent<AudioListenerComponent>();
	ImGui::CloseCurrentPopup();
      }
    }

    if (!entity.HasComponent<AudioSourceComponent>()) {
      if(ImGui::MenuItem("AudioSource")){
        entity.AddComponent<AudioSourceComponent>();
	ImGui::CloseCurrentPopup();
      }
    }

    if (!entity.HasComponent<NativeScriptComponent>()) {
      if (ImGui::MenuItem("NativeScript")) {
	class DefaultScript : public ScriptableEntity {
        public:	  
	};
        entity.AddComponent<NativeScriptComponent>().Bind<DefaultScript>();
	ImGui::CloseCurrentPopup();
      }
    }

    if (!entity.HasComponent<LuaScriptComponent>()) {
      if(ImGui::MenuItem("LuaScript")){
        entity.AddComponent<LuaScriptComponent>().scriptPath =
            "Resources/Scripts/test.lua";        
	ImGui::CloseCurrentPopup();
        }
    }

    if(!entity.HasComponent<EditorOnlyComponent>()){
      if (ImGui::MenuItem("EditOnly")) {
        entity.AddComponent<EditorOnlyComponent>();
	ImGui::CloseCurrentPopup();
      }
    }

    ImGui::EndPopup();
  }
}

void SceneHierarchyPanel::DrawComponents(Entity entity) {  
  // Tag
  if (entity.HasComponent<TagComponent>()) {
    auto &tag = entity.GetComponent<TagComponent>().Tag;
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, tag.c_str(), sizeof(buffer) - 1);
    if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
      tag = std::string(buffer);
  }
  // Add Component button
  ImGui::SameLine();
  if (ImGui::Button("Add Component"))
    ImGui::OpenPopup("AddComponentPopup");
  DrawAddComponentPopup(entity);

  // Transform
  DrawComponent<TransformComponent>(
      "TransformComponent", entity, [](Entity entt, TransformComponent &tc) {
        if(DrawVec3Control("Translation", tc.Translation)) {
            if(entt.HasComponent<RigidbodyComponent>()) {
                auto& rb = entt.GetComponent<RigidbodyComponent>();
                rb.Dirty = true;
            }
        }
        DrawVec3Control("Rotation", tc.Rotation);
	DrawVec3Control("Scale", tc.Scale, 1.0f);
      },
      false);

  DrawComponent<CameraComponent>("Camera", entity,
                                 [](Entity, CameraComponent &cc) {
                                   DrawCameraControl(cc.Camera);
                                   ImGui::Checkbox("Use Target Mode", &cc.UseTargetMode);
                                   ImGui::Checkbox("Primary", &cc.Primary);
				   ImGui::Checkbox("FixedAspectRatio", &cc.FixedAspecRatio);
                                   });

  DrawComponent<CubeComponent>("CubeComponent", entity,
                               [](Entity, CubeComponent &cc) {
                                 DrawColorControl("Color", cc.Tint);
				 ImGui::Checkbox("SceneLighting", &cc.useSceneLighting);
                               });

  DrawComponent<PlaneComponent>("PlaneComponent", entity,
                                [](Entity, PlaneComponent &pc) {
				  DrawColorControl("Color", pc.Tint);
				  ImGui::Checkbox("SceneLighting", &pc.useSceneLighting);
				});

  DrawComponent<SphereComponent>("SphereComponent", entity,
                                 [](Entity, SphereComponent &sc) {
                                   DrawColorControl("Color", sc.Tint);
				   ImGui::Checkbox("SceneLighting", &sc.useSceneLighting);
                                   });

  DrawComponent<ModelComponent>("ModelComponent", entity,
                                [](Entity, ModelComponent &mc) {
                                  ImGui::Text(mc.ModelPath.c_str());
				  ImGui::Checkbox("SceneLighting", &mc.useSceneLighting);
				  DrawColorControl("Tint", mc.Tint);
                                });

  DrawComponent<SpriteComponent>("SpriteComponent", entity,
                                 [](Entity, SpriteComponent &sc) {
                                   ImGui::Text(sc.TexturePath.c_str());
				   ImGui::Checkbox("SceneLighting", &sc.useSceneLighting);
                                   DrawColorControl("Tint", sc.Tint);
                                 });

  DrawComponent<LightComponent>(
      "LightComponent", entity, [](Entity, LightComponent &lc) {
        const char *items[] = {"Directional", "Point", "Spot"};
	int selected = (int)lc.Type;

        if (ImGui::Combo("Type", &selected, items, IM_ARRAYSIZE(items))) {
	  lc.Type = (LightType)selected;
	}
                                  
	DrawColorControl("Color", lc.ColorValue);
	ImGui::DragFloat("Intensity", &lc.Intensity);

	if (lc.Type == LightType::Point || lc.Type == LightType::Spot) {
	  ImGui::DragFloat("Range", &lc.Range, 0.5f, 0.0f, 1000.0f);
	}

	if (lc.Type == LightType::Directional || lc.Type == LightType::Spot) {
	  DrawVec3Control("Direction", lc.Direction);
        }

	if (lc.Type == LightType::Spot) {
	  // Contrôle de l'angle du cône
	  ImGui::SliderFloat("Spot Angle", &lc.SpotAngle, 1.0f, 90.0f);
	}

	ImGui::Checkbox("Shadow", &lc.CastShadow);
      });

  DrawComponent<RigidbodyComponent>("RigidbodyComponent", entity,
                                    [](Entity, RigidbodyComponent &rc) {
				      const char *items[] = {"Static", "Dynamic", "Kinematic"};
				      int selected = (int)rc.Type;

                                      if (ImGui::Combo("Type", &selected, items,
                                                       IM_ARRAYSIZE(items))) {
                                        rc.Type = (BodyType)selected;
                                      }

				      switch (selected) {
				      case 0:
					rc.Type = BodyType::Static;
					break;
				      case 1:
					rc.Type = BodyType::Dynamic;
					break;
				      case 3:
					rc.Type = BodyType::Kinematic;
					break;
                                      }

                                      DrawVec3Control("Velocity", rc.Velocity);
                                      DrawVec3Control("AngularVelocity",
                                                      rc.AngularVelocity);

                                      ImGui::DragFloat("Mass", &rc.Mass);
                                      ImGui::DragFloat("Restitution",
                                                       &rc.Restitution);
                                      ImGui::DragFloat("Friction", &rc.Friction);
                                      ImGui::DragFloat("LinearDamping",
                                                       &rc.LinearDamping);
                                      ImGui::DragFloat("AngularDamping",
                                                       &rc.AngularDamping);

                                      ImGui::Checkbox("Gravity",
                                                      &rc.useGravity);
                                    });

  DrawComponent<CharacterComponent>("CharacterComponent", entity,
                                    [](Entity, CharacterComponent &cc) {
                                      // DrawVec3Control("Velocity", cc.Velocity);
                                      // ImGui::DragFloat("Speed", &cc.Speed);
                                      // ImGui::DragFloat("Jump Strength",
                                      //                  &cc.JumpStrength);
                                      // ImGui::Checkbox("Grounded", &cc.Grounded);
                                    });

  DrawComponent<BoxColliderComponent>("BoxColliderComponent", entity,
                                      [](Entity, BoxColliderComponent &bc) {
                                        DrawVec3Control("Offset", bc.Offset);
                                        DrawVec3Control("Size", bc.Size);

					ImGui::Checkbox("IsTrigger", &bc.IsTrigger);
                                      });

  DrawComponent<SphereColliderComponent>(
      "SphereColliderComponent", entity,
      [](Entity, SphereColliderComponent &sc) {
        DrawVec3Control("Offset", sc.Offset);
        ImGui::DragFloat("Radius", &sc.Radius);

        ImGui::Checkbox("IsTrigger", &sc.IsTrigger);
      });

  DrawComponent<CapsuleColliderComponent>(
      "CapsuleColliderComponent", entity,
      [](Entity, CapsuleColliderComponent &cc) {
        DrawVec3Control("Offset", cc.Offset);
        ImGui::DragFloat("Radius", &cc.Radius);
        ImGui::DragFloat("HalfHeight", &cc.HalfHeight);

        ImGui::Checkbox("IsTrigger", &cc.IsTrigger);
      });  

  DrawComponent<AudioSourceComponent>("AudioSourceComponent", entity,
                                      [](Entity, AudioSourceComponent &asc) {
                                        ImGui::Text(asc.SoundPath.c_str());

                                        ImGui::Checkbox("PlayOnStart",
                                                        &asc.PlayOnStart);
                                        ImGui::Checkbox("Loop", &asc.Loop);

                                        ImGui::DragFloat("Volume", &asc.Volume);
					ImGui::DragFloat("Pitch", &asc.Pitch);
                                      });
  DrawComponent<AudioListenerComponent>(
      "AudioListenerComponent", entity,
      [](Entity, AudioListenerComponent &alc) {
	ImGui::Checkbox("Primary", &alc.Primary);
        
  });
  
  DrawComponent<NativeScriptComponent>("NativeScriptComponent", entity,
                                       [](Entity, NativeScriptComponent &nsc) {
					 
  });

  DrawComponent<LuaScriptComponent>(
      "LuaScriptComponent", entity, [](Entity, LuaScriptComponent &component) {
	// 1. Only load if the buffer is empty (or use a flag)
        if (component.sourceCode.empty() && !component.scriptPath.empty()) {
          char *loaded = LoadFileText(component.scriptPath.c_str());
          if (loaded) {
            component.sourceCode = loaded;
            UnloadFileText(loaded); // Clean up raylib's memory immediately
          }
        }

        // 2. Edit the persistent string (Requires #include "imgui_stdlib.h")
        if (ImGui::InputTextMultiline("##source", &component.sourceCode,
                                      ImVec2(-1.0f, 200.0f),
                                      ImGuiInputTextFlags_AllowTabInput)) {
          // This returns true when the text changes
          component.isDirty = true;
        }

        // 3. Save button
        if (ImGui::Button("Save Script")) {
          SaveFileText(component.scriptPath.c_str(),
                       (char *)component.sourceCode.c_str());
        }
      });

  DrawComponent<EditorOnlyComponent>("EditorOnlyComponent", entity,
                                     [](Entity, EditorOnlyComponent &eoc) {
                                       ImGui::Checkbox("Visible", &eoc.Visible);
                                       ImGui::Checkbox("Locked", &eoc.Loacked);                                       
  });
  
  
}

