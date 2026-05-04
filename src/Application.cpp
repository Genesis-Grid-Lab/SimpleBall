#include "Application.h"
#include "Components.h"
#include "LuaScriptEngine.h"
#include "RuntimeScene.h"
#include "SceneHierarchyPanel.h"
#include "imgui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"
#include "ResourceManager.h"
#include "ScriptableEntity.h"

Application::Application() {
  ResourceManager::LoadShaderResource("LightShader", "Resources/Shaders/lighting.vs", "Resources/Shaders/lighting.fs");
  m_EditorScene = CreateRef<EditorScene>();
  m_RuntimeScene = CreateRef<RuntimeScene>();

  LuaScriptEngine::Init();

  ResourceManager::Load<Texture2D>("IconPlay", "Resources/Icons/PlayButton.png");
  ResourceManager::Load<Texture2D>("IconStop",
                                   "Resources/Icons/StopButton.png");

  m_SceneHierarchyPanel = SceneHierarchyPanel(m_EditorScene);
  m_EditorScene->m_SceneHierarchy = &m_SceneHierarchyPanel;

  TestingGround();
}

void Application::TestingGround() {

  auto floor = m_EditorScene->CreateEntity("Floor");
  auto& fTC = floor.GetComponent<TransformComponent>();
  floor.AddComponent<PlaneComponent>().Tint = PURPLE;
  fTC.Scale = {10, 0, 10};
  floor.AddComponent<BoxColliderComponent>().Size = Vector3{1, 1, 1};
  floor.AddComponent<RigidbodyComponent>().Type = BodyType::Static;

  auto cube = m_EditorScene->CreateEntity("Cube");
  auto &cubeTC = cube.GetComponent<TransformComponent>();
  auto &cubeComp = cube.AddComponent<CubeComponent>();
  cubeTC.Scale = Vector3{2, 2, 2};
  cubeComp.Tint = GREEN;

  cubeTC.Translation = Vector3{0, 5, 0};
  cube.AddComponent<BoxColliderComponent>().Size = Vector3{1, 1, 1};
  cube.AddComponent<RigidbodyComponent>().Type = BodyType::Dynamic;

  class cubeControl : public ScriptableEntity {
  public:
    virtual void OnCreate() override {
      
    }
    virtual void OnDestroy() override {}
    virtual void OnUpdate(float ts) override {
      
    }
  };
  cube.AddComponent<NativeScriptComponent>().Bind<cubeControl>();

  auto cam = m_EditorScene->CreateEntity("cam");
  auto &camTc = cam.GetComponent<TransformComponent>();
  auto &camComp = cam.AddComponent<CameraComponent>();
  camComp.Camera.fovy = 45.0f;
  camComp.Camera.projection = CAMERA_PERSPECTIVE;
  camComp.Camera.up = {0, 1, 0};
  camComp.Camera.target = cubeTC.Translation;

  camTc.Translation = {-9.8f, 7.0f, 20.0f};
  camTc.Rotation = {0, 2.6f, 0};

  auto light = m_EditorScene->CreateEntity("Light");
  auto& lTC = light.GetComponent<TransformComponent>();
  auto& lcomp = light.AddComponent<LightComponent>();
  lcomp.Type = LightType::Directional;
  lcomp.CastShadow = true;
  lTC.Translation = {0, 7, 0};


  auto man = m_EditorScene->CreateEntity("Man");
  auto &manMC = man.AddComponent<ModelComponent>();
  manMC.ModelPath = "Resources/greenman.glb";
  manMC.model = ResourceManager::Load<Model>(manMC.ModelPath, manMC.ModelPath);
  manMC.Loaded = true;
  auto &manAnim = man.AddComponent<AnimationComponent>();
  manAnim.AnimationPath = "Resources/greenman.glb";
  manAnim.Play(2);
  man.AddComponent<LuaScriptComponent>().scriptPath = "Resources/Scripts/test.lua";  
}

void Application::Run() {  

  m_EditorScene->SetViewportState(m_Hovered, m_Focused);
  m_EditorScene->SetMousePos(m_RelativeMousePos);
  m_EditorScene->VSIZE = VSIZE;
  m_EditorScene->VPOS = VPOS;

    switch (m_SceneState) {
    case SceneState::Edit: {
      
      m_EditorScene->OnUpdate(GetFrameTime());
      
      break;
    }
    case SceneState::Play: {
	m_RuntimeScene->OnUpdate(GetFrameTime());
      break;
    }
    case SceneState::Paused: {
      break;
    }
  }
  // }
  UI_Toolbar();
  m_SceneHierarchyPanel.Update();
  m_ContentBrowserPanel.Update();
  ViewScene();
  Gizmo();  
}

void Application::Gizmo() {
}

void Application::ViewScene() {
  bool Open = true;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  
  if (ImGui::Begin("3D View", &Open, ImGuiWindowFlags_NoScrollbar))
    {
      m_Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
      m_Hovered = ImGui::IsWindowHovered(ImGuiFocusedFlags_ChildWindows);
      VPOS = {ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y};
      VSIZE = {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y};

      ImVec2 vPos = ImGui::GetCursorScreenPos();
      ImVec2 mPos = ImGui::GetMousePos();

      m_RelativeMousePos = {mPos.x - vPos.x, mPos.y - vPos.y};
      // draw the view
      if(m_SceneState == SceneState::Edit)
        rlImGuiImageRenderTextureFit(&m_EditorScene->GetViewTexture(), true);
      else if(m_SceneState == SceneState::Play)
        rlImGuiImageRenderTextureFit(&m_RuntimeScene->GetViewTexture(), true);
    }
  
  ImGui::End();
  ImGui::PopStyleVar();
}

void Application::UI_Toolbar() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  auto &colors = ImGui::GetStyle().Colors;
  const auto &buttonHovered = colors[ImGuiCol_ButtonHovered];
  ImGui::PushStyleColor(
      ImGuiCol_ButtonHovered,
      ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
  const auto &buttonActive = colors[ImGuiCol_ButtonActive];
  ImGui::PushStyleColor(
      ImGuiCol_ButtonActive,
      ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

  ImGui::Begin("##toolbar", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoScrollWithMouse);

  auto &m_IconPlay = ResourceManager::Get<Texture2D>("IconPlay");
  auto& m_IconStop = ResourceManager::Get<Texture2D>("IconStop");

  float size = ImGui::GetWindowHeight() - 4.0f;
  Texture icon = m_SceneState == SceneState::Edit ? m_IconPlay : m_IconStop;

  std::string play = m_SceneState == SceneState::Edit ? "Play" : "Stop";
  ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) -
                       (size * 0.5f));

  if (rlImGuiImageButtonSize("##play", &icon, Vector2{size, size})) {
    if (m_SceneState == SceneState::Edit)
      OnScenePlay();
    else if (m_SceneState == SceneState::Play)
      OnSceneStop();
    
  }
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(3);
  ImGui::End();

}

void Application::NewScene() {
  
}
void Application::OpenScene(){

  }
void Application::SaveScene() {
  
}
void Application::SaveSceneAs() {
  
}

void Application::OnScenePlay() {
  m_SceneState = SceneState::Play;

  if (!m_RuntimeScene)
      m_RuntimeScene = CreateRef<RuntimeScene>();

  m_RuntimeScene = Scene::Copy<RuntimeScene>(m_EditorScene);
  m_RuntimeScene->OnRuntimeStart();
  
  m_SceneHierarchyPanel.SetContext(m_RuntimeScene);

}
void Application::OnSceneStop(){
  if (m_SceneState != SceneState::Play)
      return;

  m_SceneState = SceneState::Edit;

  m_RuntimeScene->OnRuntimeStop();

  m_SceneHierarchyPanel.SetContext(m_EditorScene);
}
