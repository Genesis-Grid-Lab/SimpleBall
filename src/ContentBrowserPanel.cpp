#include "ContentBrowserPanel.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "rlImGui.h"

static std::string to_utf8(const std::filesystem::path& p){

#ifdef _WIN32
  auto u8 = p.u8string();
  return std::string(u8.begin(), u8.end());
#else
  return p.string();
#endif
}

// Once we have projects change this
extern const std::filesystem::path g_AssetPath = "Resources";

ContentBrowserPanel::ContentBrowserPanel() : m_CurrentDirectory(g_AssetPath) {

  m_DirectoryIcon =
      LoadTexture("Resources/Icons/ContentBrowser/DirectoryIcon.png");
  m_FileIcon = LoadTexture("Resources/Icons/ContentBrowser/FileIcon.png");
}

void ContentBrowserPanel::Shutdown() {
  UnloadTexture(m_DirectoryIcon);
  UnloadTexture(m_FileIcon);
}

void ContentBrowserPanel::Update() {
  ImGui::Begin("Content Browser");

  if (m_CurrentDirectory != std::filesystem::path(g_AssetPath)) {
    if (ImGui::Button("<-")) {
      m_CurrentDirectory = m_CurrentDirectory.parent_path();
    }
  }

  static float padding = 20.0f;
  static float thumbnailSize = 52.0f;
  float cellSize = thumbnailSize + padding;

  float panelWidth = ImGui::GetContentRegionAvail().x;
  int columnCount = (int)(panelWidth / cellSize);
  if (columnCount < 1)
    columnCount = 1;

  if (ImGui::BeginTable("ContentBrowser", columnCount)) {
    for (auto &directoryEntry :
         std::filesystem::directory_iterator(m_CurrentDirectory)) {
      ImGui::TableNextColumn();
    const auto &path = directoryEntry.path();
    auto relativePath = std::filesystem::relative(path, g_AssetPath);

    std::string filenameString = relativePath.filename().string();

    ImGui::PushID(filenameString.c_str());
    Texture icon = directoryEntry.is_directory() ? m_DirectoryIcon : m_FileIcon;
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));

    rlImGuiImageButtonSize(filenameString.c_str(), &icon,
                           Vector2{thumbnailSize, thumbnailSize});

    if(ImGui::BeginDragDropSource()){
      std::string itemPathStr = to_utf8(relativePath);
#ifdef _WIN32
      const wchar_t *itemPath = relativePath.c_str();
#else
      const char* itemPath = itemPathStr.c_str();
#endif

      ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath,
                                itemPathStr.size() + 1);

      ImGui::EndDragDropSource();
    }

    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
      if (directoryEntry.is_directory())
	m_CurrentDirectory /= path.filename();
    }

    ImGui::TextWrapped(filenameString.c_str());

    ImGui::PopID();
    
  }
    ImGui::EndTable();
}

  ImGui::End();
}

