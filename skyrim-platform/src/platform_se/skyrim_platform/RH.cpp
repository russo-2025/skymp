#include "RH.h"

#include <RE/MenuControls.h>
#include <RE/MenuEventHandler.h>
#include <RE/UI.h>
#include <RE/IMenu.h>
#include <RE/UIMessage.h>
#include <RE/Console.h>

namespace RE {
	class ConsoleOpenHandler : public MenuEventHandler {
	public:
		inline static constexpr auto RTTI = RTTI_MenuEventHandler;

		ConsoleOpenHandler() = default;
		virtual ~ConsoleOpenHandler() = default;

		virtual bool CanProcess(InputEvent* a_event) = 0;
		virtual bool ProcessKinect(KinectEvent* a_event);
		virtual bool ProcessThumbstick(ThumbstickEvent* a_event);
		virtual bool ProcessMouseMove(MouseMoveEvent* a_event);
		virtual bool ProcessButton(ButtonEvent* a_event);

		bool   registered;	// 0C
		UInt8  unk0D;		// 0D
		UInt16 pad0E;		// 0E
	};
}

class EmptyEventHandler : public RE::MenuEventHandler {
public:
	bool CanProcess(RE::InputEvent * e) override { return false; }
	bool ProcessKinect(RE::KinectEvent * e) override { return false; }
	bool ProcessThumbstick(RE::ThumbstickEvent * e) override { return false; }
	bool ProcessMouseMove(RE::MouseMoveEvent * e) override { return false; }
	bool ProcessButton(RE::ButtonEvent * e) override { return false; }
};

class EmptyMenu : public RE::IMenu {
public:
	void Accept(CallbackProcessor* a_processor) override { return; }

	void PostCreate() { return; }
	void Unk_03(void) { return; }
	RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage& a_message) { return RE::UI_MESSAGE_RESULTS::kIgnore; }
	void AdvanceMovie(float a_interval, UInt32 a_currentTime) { return; }
	void PostDisplay() { return; }
	void PreDisplay() { return; }
	void RefreshPlatform() { return; }
};

void DisableConsole() {
	auto mc = RE::MenuControls::GetSingleton();
	auto ui = RE::UI::GetSingleton();

	ui->menuMap.insert_or_assign({ "Console", {
		nullptr,
		[]()-> RE::IMenu* { return (RE::IMenu*)new EmptyMenu(); }
	} });

	mc->RemoveHandler((RE::MenuEventHandler*)mc->consoleOpenHandler.get());
	mc->consoleOpenHandler = RE::BSTSmartPointer<RE::ConsoleOpenHandler>((RE::ConsoleOpenHandler*)(RE::MenuEventHandler*)new EmptyEventHandler);
	mc->RemoveHandler((RE::MenuEventHandler*)mc->consoleOpenHandler.get());
}