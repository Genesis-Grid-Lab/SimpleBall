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

  s_Lua.set_function("print", [](sol::variadic_args args)
  {
    std::string out;

    for (auto v : args)
      {
        out += v.as<std::string>();
        out += " ";
      }

    TraceLog(LOG_INFO, "[Lua] %s", out.c_str());
  });

  s_Lua["KEY_W"] = KEY_W;
  s_Lua["KEY_A"] = KEY_A;
  s_Lua["KEY_S"] = KEY_S;
  s_Lua["KEY_D"] = KEY_D;
}

void LuaScriptEngine::LoadScript(Entity entity, LuaScriptComponent &comp) {
  sol::load_result loaded = s_Lua.load_file(comp.scriptPath);

  TraceLog(LOG_WARNING, "Loading Lua script: %s", comp.scriptPath.c_str());

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

  if (result.get_type() != sol::type::table)
    {
      TraceLog(LOG_ERROR, "Lua script did not return a table");
      comp.Valid = false;
      return;
    }  

  sol::table scripTable = result;

  // comp.Instance = scripTable;
  comp.Instance = result.get<sol::table>();
  comp.Valid = true;

  comp.Instance["entity"] = entity;

  comp.Instance = result.get<sol::table>();  

  sol::protected_function onCreate = comp.Instance["OnCreate"];

  if (onCreate.valid()) {
    auto createResult = onCreate(comp.Instance);

    if (!createResult.valid()) {
      sol::error err = createResult;
      TraceLog(LOG_WARNING, "Lua onCreate error: %s", err.what());
    }
  }

}
