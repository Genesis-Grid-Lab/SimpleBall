#include <memory>
#include <raylib.h>
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include "imgui.h"
#include "rlImGui.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#define screenWidth 1280
#define screenHeight 800

//------------------------ Scope = unique pointer ---------------------
template <typename T> using Scope = std::unique_ptr<T>;
template <typename T, typename... Args>
constexpr Scope<T> CreateScope(Args &&...args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

//------------------------ Ref = shared pointer ------------------------
template <typename T> using Ref = std::shared_ptr<T>;
template <typename T, typename... Args>
  constexpr Ref<T> CreateRef(Args&& ... args){
  return std::make_shared<T>(std::forward<Args>(args)...);
}

