#include "Application.h"
#include "ResourceManager.h"
#include "raylib.h"

int main() {

  InitWindow(screenWidth, screenHeight, "SimpleBall");

  SetTargetFPS(60);

  rlImGuiSetup(true);
  ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

#ifdef IMGUI_HAS_DOCK
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

  Scope<Application> app = CreateScope<Application>();

  bool run = true;

  bool showDemoWindow = false;

  SetExitKey(0);

  ImGui::GetIO().IniFilename = "Data/default.ini";
  
  while (!WindowShouldClose() && run) {
    BeginDrawing();
    {
      rlImGuiBegin();
#ifdef IMGUI_HAS_DOCK
      ImGui::DockSpaceOverViewport(0,  NULL, ImGuiDockNodeFlags_PassthruCentralNode); // set ImGuiDockNodeFlags_PassthruCentralNode so that we can see the raylib contents behind the dockspace
#endif

      if (ImGui::BeginMainMenuBar()) {

        if (ImGui::BeginMenu("File"))
          {
            if (ImGui::MenuItem("Quit"))
              run = false;
            
            ImGui::EndMenu();
          }
        
        if (ImGui::BeginMenu("Window"))
	  {
	    if (ImGui::MenuItem("Demo Window", nullptr, showDemoWindow))
	      showDemoWindow = !showDemoWindow;

      if (ImGui::MenuItem("Fullscreen"))
        ToggleBorderlessWindowed();   
        
	    ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug")) {
          if (ImGui::MenuItem("Show Physics Colliders"))
              PhysicsEngine::ToggleDebugDraw();
          ImGui::EndMenu();
      }
      
    
    
  

  if(ImGui::BeginMenu("Help"))
    {
      if (ImGui::MenuItem("Documentation"))
        OpenURL("");

      ImGui::EndMenu();
    }
	ImGui::EndMainMenuBar();
      }

      ClearBackground(BLUE);
      app->Run();

      if (showDemoWindow)
	ImGui::ShowDemoWindow(&showDemoWindow);

      rlImGuiEnd();
    }
    EndDrawing();
  }

  ResourceManager::UnloadAll();
  rlImGuiShutdown(); 
  CloseWindow();
   
  return 0;  
}
