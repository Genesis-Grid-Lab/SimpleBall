#pragma once

#include <raylib.h>
#include <stdexcept>
#include <string>
#include <unordered_map>

class ResourceManager {
public:
  ResourceManager() = delete;

  template <typename Asset>
  static Asset &Load(const std::string &name, const std::string &path) {
    auto &map = GetMap<Asset>();

    if (map.contains(name))
      return map.at(name);

    Asset resource = LoadFromFile<Asset>(path);

    map.emplace(name, resource);
    return map.at(name);
  }

  template <typename Asset> static Asset &Get(const std::string &name) {
    return GetMap<Asset>().at(name);
  }

  template <typename Asset> static bool Has(const std::string &name) {
    return GetMap<Asset>().contains(name);
  }

  template <typename Asset> static void Unload(const std::string &name) {
    auto &map = GetMap<Asset>();

    auto it = map.find(name);
    if (it == map.end())
      return;

    UnloadResource<Asset>(it->second);
    map.erase(it);
  }

  static void UnloadAll() {
    for (auto &[name, texture] : s_Textures)
      UnloadTexture(texture);

    for (auto &[name, sound] : s_Sounds)
      UnloadSound(sound);

    for (auto &[name, music] : s_Music)
      UnloadMusicStream(music);

    for (auto &[name, font] : s_Fonts)
      UnloadFont(font);

    for (auto &[name, shader] : s_Shaders)
      UnloadShader(shader);

    s_Textures.clear();
    s_Sounds.clear();
    s_Music.clear();
    s_Fonts.clear();
    s_Shaders.clear();
  }

public:
  static Shader& LoadShaderResource(
				    const std::string& name,
				    const std::string& vertexPath,
				    const std::string& fragmentPath
				    )
  {
    if (s_Shaders.contains(name))
      return s_Shaders.at(name);

    Shader shader = LoadShader(
			       vertexPath.empty() ? nullptr : vertexPath.c_str(),
			       fragmentPath.empty() ? nullptr : fragmentPath.c_str()
			       );

    if (shader.id == 0)
      throw std::runtime_error("Failed to load shader: " + name);

    s_Shaders.emplace(name, shader);
    return s_Shaders.at(name);
  }  
private:
  template <typename Asset>
    static std::unordered_map<std::string, Asset>& GetMap(){
    if constexpr (std::is_same_v<Asset, Texture2D>)
      return s_Textures;
    else if constexpr (std::is_same_v<Asset, Sound>)
      return s_Sounds;
    else if constexpr (std::is_same_v<Asset, Music>)
      return s_Music;
    else if constexpr (std::is_same_v<Asset, Font>)
      return s_Fonts;
    else if constexpr (std::is_same_v<Asset, Shader>)
      return s_Shaders;
    else
      static_assert(sizeof(Asset) == 0, "Unsupported resource type");
  }

  template <typename Asset> static Asset LoadFromFile(const std::string &path) {
    if constexpr (std::is_same_v<Asset, Texture2D>) {
      Texture2D texture = LoadTexture(path.c_str());
      if (texture.id == 0)
        throw std::runtime_error("Failed to load texture: " + path);
      return texture;
    }
    else if constexpr (std::is_same_v<Asset, Sound>){
      Sound sound = LoadSound(path.c_str());
      if (sound.frameCount == 0)
        throw std::runtime_error("Failed to load sound: " + path);
      return sound;
    }    
    else if constexpr (std::is_same_v<Asset, Music>){
      Music music = LoadMusicStream(path.c_str());
      if (music.frameCount == 0)
        throw std::runtime_error("Failed to load music: " + path);
      return music;
    }
    else if constexpr (std::is_same_v<Asset, Font>) {
      Font font = LoadFont(path.c_str());
      if (font.texture.id == 0)
        throw std::runtime_error("Failed to load font: " + path);
      return font;
    }    
    else
      static_assert(sizeof(Asset) == 0, "Unsupported resource type");
  }

  template <typename Asset> static void UnloadResource(Asset &resource) {
    if constexpr (std::is_same_v<Asset, Texture2D>)
      UnloadTexture(resource);
    else if constexpr (std::is_same_v<Asset, Sound>)
      UnloadSound(resource);    
    else if constexpr (std::is_same_v<Asset, Music>)
      UnloadMusicStream(resource);    
    else if constexpr (std::is_same_v<Asset, Font>)
      UnloadFont(resource);    
    else if constexpr (std::is_same_v<Asset, Shader>)
      UnloadShader(resource);    
    else
      static_assert(sizeof(Asset) == 0, "Unsupported resource type");
  }
private:
  inline static std::unordered_map<std::string, Texture2D> s_Textures;
  inline static std::unordered_map<std::string, Sound> s_Sounds;
  inline static std::unordered_map<std::string, Music> s_Music;
  inline static std::unordered_map<std::string, Font> s_Fonts;
  inline static std::unordered_map<std::string, Shader> s_Shaders;
};
