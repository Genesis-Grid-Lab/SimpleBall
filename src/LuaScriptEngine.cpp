#define SOL_ALL_SAFETIES_ON 1
#include "LuaScriptEngine.h"
#include "Components.h"
#include "Entity.h"
#include "raylib.h"
#include <sol/error.hpp>
#include <sol/forward.hpp>
#include <sol/load_result.hpp>
#include <sol/protected_function_result.hpp>
#include <sol/raii.hpp>

// sol::state LuaScriptEngine::s_Lua = ;

void LuaScriptEngine::Init() {
  s_Lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table,
                       sol::lib::string);

  RegisterTypes();

  TraceLog(LOG_WARNING, "INIT LUA SCRIPT");
}

void LuaScriptEngine::RegisterTypes() {
  s_Lua.new_usertype<Vector3>(
      "Vector3", sol::constructors<Vector3(), Vector3(float, float, float)>(),
      "x", &Vector3::x, "y", &Vector3::y, "z", &Vector3::z);

  s_Lua.new_usertype<TransformComponent>(
      "TransformComponent", "Translation", &TransformComponent::Translation,
      "Rotation", &TransformComponent::Rotation, "Scale",
      &TransformComponent::Scale);

  s_Lua.new_usertype<Entity>("Entity", "GetTransform",
                             [](Entity &entity) -> TransformComponent & {
                               return entity.GetComponent<TransformComponent>();
                             });

  s_Lua.set_function("IsKeyDown", [](int key) { return ::IsKeyDown(key); });

  s_Lua["KEY_W"] = KEY_W;
  s_Lua["KEY_A"] = KEY_A;
  s_Lua["KEY_S"] = KEY_S;
  s_Lua["KEY_D"] = KEY_D;
}

void LuaScriptEngine::LoadScript(Entity entity, LuaScriptComponent &comp) {
  sol::load_result loaded = s_Lua.load_file(comp.scriptPath);

  if(!loaded.valid()){
    sol::error err = loaded;
    TraceLog(LOG_WARNING, "Lua load error: %s", err.what());
    comp.Valid = false;
    return;
  }

  sol::protected_function_result result = loaded();

  if (!result.valid()) {
    sol::error err = result;
    TraceLog(LOG_WARNING, "Lua run error: %s", err.what());
    comp.Valid = false;
    return;
  }

  sol::table scripTable = result;

  comp.Instance = scripTable;
  comp.Valid = true;

  comp.Instance["entity"] = entity;

  sol::protected_function onCreate = comp.Instance["onCreate"];

  if (onCreate.valid()) {
    auto createResult = onCreate(comp.Instance);

    if (!createResult.valid()) {
      sol::error err = createResult;
      TraceLog(LOG_WARNING, "Lua onCreate error: %s", err.what());
    }
  }
}
