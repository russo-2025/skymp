#pragma once
#include "JsEngine.h"
#include "IBrowser.h"
#include "TPOverlayService.h"
#include <memory>

namespace BrowserApi {
/*struct State
{
  std::shared_ptr<OverlayService> overlayService;
};*/

JsValue SetVisible(const JsFunctionArguments& args);
JsValue SetFocused(const JsFunctionArguments& args);
//JsValue LoadUrl(const JsFunctionArguments& args, std::shared_ptr<State> state);
JsValue GetToken(const JsFunctionArguments& args);
//JsValue ExecuteJavaScript(const JsFunctionArguments& args, std::shared_ptr<State> state);

inline void Register(JsValue& exports, std::shared_ptr<IBrowser> browser)
{
  auto browserObj = JsValue::Object();
  browserObj.SetProperty("setVisible", JsValue::Function(SetVisible));
  browserObj.SetProperty("setFocused", JsValue::Function(SetFocused));
  browserObj.SetProperty(
    "loadUrl",
    JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
      return browser->LoadUrl(static_cast<std::string>(args[1]));
    }));
  browserObj.SetProperty("getToken", JsValue::Function(GetToken));
  browserObj.SetProperty(
    "executeJavaScript",
    JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
      browser->ExecuteJavaScript(static_cast<std::string>(args[1]));
      return JsValue::Undefined();
    }));
  exports.SetProperty("browser", browserObj);
}
}
