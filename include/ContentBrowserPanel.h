#pragma once

#include "raylib.h"
#include <filesystem>

class ContentBrowserPanel {
public:
  ContentBrowserPanel();
  void Update();

  void Shutdown();

private:
  std::filesystem::path m_CurrentDirectory;

  Texture2D m_DirectoryIcon;
  Texture2D m_FileIcon;
};
