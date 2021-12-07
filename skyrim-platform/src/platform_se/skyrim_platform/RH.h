#pragma once

#include "JsEngine.h"

namespace RH {
void Init();

void SetFirstPersonFOV(double fov);
void SetWorldFOV(double fov);

inline void Register(JsValue& exports)
{
  auto obj = JsValue::Object();
  obj.SetProperty(
    "SetFirstPersonFOV",
    JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
      SetFirstPersonFOV(args[1]);
      return JsValue::Undefined();
    }));
  obj.SetProperty(
    "SetWorldFOV",
    JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
      SetWorldFOV(args[1]);
      return JsValue::Undefined();
    }));
  exports.SetProperty("rh", obj);
}
}