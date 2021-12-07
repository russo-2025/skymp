#include "RH.h"

// DisableConsole
// DisableDoubleSpeed
#include <RE/ButtonEvent.h>
#include <RE/ConsoleLog.h>
#include <RE/IMenu.h>
#include <RE/InputEvent.h>
#include <RE/MenuControls.h>
#include <RE/MenuEventHandler.h>
#include <RE/UI.h>
#include <RE/UIMessage.h>

//DevApi::DisableCtrlPrtScnHotkey
#include "DevApi.h"

// SetWorldFOV
// SetFirstPersonFOV
#include <RE/PlayerCamera.h>

namespace RE {
class ScreenshotHandler : public MenuEventHandler
{
public:
  inline static constexpr auto RTTI = RTTI_MenuEventHandler;

  ScreenshotHandler() = default;
  virtual ~ScreenshotHandler() = default;

  virtual bool CanProcess(InputEvent* a_event) = 0;
  virtual bool ProcessKinect(KinectEvent* a_event);
  virtual bool ProcessThumbstick(ThumbstickEvent* a_event);
  virtual bool ProcessMouseMove(MouseMoveEvent* a_event);
  virtual bool ProcessButton(ButtonEvent* a_event);
};

class ConsoleOpenHandler : public MenuEventHandler
{
public:
  inline static constexpr auto RTTI = RTTI_MenuEventHandler;

  ConsoleOpenHandler() = default;
  virtual ~ConsoleOpenHandler() = default;

  virtual bool CanProcess(InputEvent* a_event) = 0;
  virtual bool ProcessKinect(KinectEvent* a_event);
  virtual bool ProcessThumbstick(ThumbstickEvent* a_event);
  virtual bool ProcessMouseMove(MouseMoveEvent* a_event);
  virtual bool ProcessButton(ButtonEvent* a_event);

  bool registered; // 0C
  UInt8 unk0D;     // 0D
  UInt16 pad0E;    // 0E
};
}

namespace RH {
class EmptyEventHandler : public RE::MenuEventHandler
{
public:
  bool CanProcess(RE::InputEvent* e) override { return false; }
  bool ProcessKinect(RE::KinectEvent* e) override { return false; }
  bool ProcessThumbstick(RE::ThumbstickEvent* e) override { return false; }
  bool ProcessMouseMove(RE::MouseMoveEvent* e) override { return false; }
  bool ProcessButton(RE::ButtonEvent* e) override { return false; }
};

class WrapperScreenShotEventHandler : public RE::MenuEventHandler
{
public:
  WrapperScreenShotEventHandler::WrapperScreenShotEventHandler(
    RE::MenuEventHandler* originalHandler_)
    : originalHandler(originalHandler_)
  {
  }

  bool CanProcess(RE::InputEvent* e) override
  {
    if (e->eventType == RE::INPUT_EVENT_TYPE::kButton) {
      if (strcmp(e->QUserEvent().c_str(), "Screenshot") == 0) {
        return originalHandler->CanProcess(e);
      }
    }

    return false;
  }
  bool ProcessKinect(RE::KinectEvent* e) override { return false; }
  bool ProcessThumbstick(RE::ThumbstickEvent* e) override { return false; }
  bool ProcessMouseMove(RE::MouseMoveEvent* e) override { return false; }
  bool ProcessButton(RE::ButtonEvent* e) override
  {
    if (strcmp(e->QUserEvent().c_str(), "Screenshot") == 0) {
      return originalHandler->ProcessButton(e);
    }

    return false;
  }

  RE::MenuEventHandler* originalHandler;
};

class EmptyMenu : public RE::IMenu
{
public:
  void Accept(CallbackProcessor* a_processor) override { return; }

  void PostCreate() { return; }
  void Unk_03(void) { return; }
  RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage& a_message)
  {
    return RE::UI_MESSAGE_RESULTS::kIgnore;
  }
  void AdvanceMovie(float a_interval, UInt32 a_currentTime) { return; }
  void PostDisplay() { return; }
  void PreDisplay() { return; }
  void RefreshPlatform() { return; }
};

void SetFirstPersonFOV(double fov)
{
  auto pc = RE::PlayerCamera::GetSingleton();
  pc->lock.Lock();
  pc->firstPersonFOV = (float)fov;
  pc->lock.Unlock();
}

void SetWorldFOV(double fov)
{
  auto pc = RE::PlayerCamera::GetSingleton();
  pc->lock.Lock();
  pc->worldFOV = (float)fov;
  pc->lock.Unlock();
}

void DisableConsole()
{
  auto mc = RE::MenuControls::GetSingleton();
  auto ui = RE::UI::GetSingleton();

  ui->menuMap.insert_or_assign({ "Console", { nullptr,
      []() -> RE::IMenu* {
          return (
              RE::IMenu*)new EmptyMenu();
      }
  }});

  mc->RemoveHandler((RE::MenuEventHandler*)mc->consoleOpenHandler.get());
  mc->consoleOpenHandler = RE::BSTSmartPointer<RE::ConsoleOpenHandler>(
    (RE::ConsoleOpenHandler*)(RE::MenuEventHandler*)new EmptyEventHandler);
  mc->AddHandler((RE::MenuEventHandler*)mc->consoleOpenHandler.get());
}

void DisableModMenu()
{
  auto ui = RE::UI::GetSingleton();

  ui->menuMap.insert_or_assign(
    { "Mod Manager Menu", { nullptr, []() -> RE::IMenu* {
                             return (RE::IMenu*)new EmptyMenu();
                           } } });

  ui->menuMap.insert_or_assign(
    { "Creation Club Menu", { nullptr, []() -> RE::IMenu* {
                               return (RE::IMenu*)new EmptyMenu();
                             } } });
}

void Init()
{
  DisableConsole();
  DisableModMenu();
  DevApi::DisableCtrlPrtScnHotkey();
}
}