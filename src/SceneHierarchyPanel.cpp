#include "SceneHierarchyPanel.h"
#include "Components.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstring>
#include "ScriptableEntity.h"

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

    if(!entity.HasComponent<SphereComponent>()){
      if (ImGui::MenuItem("Sphere")) {
        entity.AddComponent<SphereComponent>();
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
      "TransformComponent", entity,
      [](Entity, TransformComponent &tc) {
        DrawVec3Control("Translation", tc.Translation);
        DrawVec3Control("Rotation", tc.Rotation);
	DrawVec3Control("Scale", tc.Scale, 1.0f);
      },
      false);

  DrawComponent<CubeComponent>(
      "CubeComponent", entity,
      [](Entity, CubeComponent &cc) { DrawColorControl("Color", cc.color); });  

  DrawComponent<CameraComponent>(
      "Camera", entity,
      [](Entity, CameraComponent &cc) { DrawCameraControl(cc.Camera); });

  DrawComponent<SphereComponent>(
      "SphereComponent", entity,
      [](Entity, SphereComponent &sc) { DrawColorControl("Color", sc.color); });

  DrawComponent<NativeScriptComponent>("NativeScriptComponent", entity,
                                       [](Entity, NativeScriptComponent &nsc) {
					 
  });
}
