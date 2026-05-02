#pragma once
#include "sol/sol.hpp"

class Entity;
struct LuaScriptComponent;

class LuaScriptEngine {
public:
  static void Init();
  static sol::state &Get() { return s_Lua; }
  static void LoadScript(Entity entity, LuaScriptComponent& comp);

private:
  static void RegisterTypes();

private:
  inline static sol::state s_Lua;
};
