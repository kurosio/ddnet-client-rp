#ifndef GAME_CLIENT_COMPONENTS_WINDOWS_H
#define GAME_CLIENT_COMPONENTS_WINDOWS_H
#include <game/client/component.h>

#include <vector>

/*
	UI elements.
*/
#define POPUP_REGISTER(f, o)  std::bind(f, o, std::placeholders::_1, std::placeholders::_2)

struct BaseElemUI
{
	virtual ~BaseElemUI() = default;

	class CWindowUI *m_pWindow{};
};

// popup
enum class PopupState : int { RENDER, YES, NO };
using PopupWindowCallback = std::function<void(CUIRect MainView, class CWindowUI *, PopupState)>;
struct PopupElemUI : BaseElemUI
{
	PopupElemUI()
	{
		m_pButtonYes = new CButtonContainer;
		m_pButtonNo = new CButtonContainer;
	}
	~PopupElemUI() override
	{
		delete m_pButtonYes;
		delete m_pButtonNo;
	}

	char m_aTextPopup[1024]{};
	PopupWindowCallback m_pCallback{};
	CButtonContainer *m_pButtonYes{};
	CButtonContainer *m_pButtonNo{};
};

// message
struct MessageElemUI : BaseElemUI
{
	MessageElemUI()
	{
		m_pButtonOk = new CButtonContainer;
	}
	~MessageElemUI() override
	{
		delete m_pButtonOk;
	}

	char m_aMessageText[2048]{};
	CButtonContainer *m_pButtonOk;
};

/*
	Windows UI elements are placed between CMenus and CWindows
	Because Teeworlds does not support a layer-by-layer UI checker.
*/
class CWindowController : public CComponent
{
	PopupElemUI *CreatePopupElement(const char *pMessage, PopupWindowCallback Callback) const;
	MessageElemUI *CreateInformationBoxElement(const char *pMessage) const;
	void CallbackRenderGuiPopupBox(CUIRect MainView, CWindowUI *pCurrentWindow);
	void CallbackRenderInfoWindow(CUIRect MainView, CWindowUI *pCurrentWindow);

	std::vector<BaseElemUI *> m_aElements;

public:
	~CWindowController() override;

	int Sizeof() const override { return sizeof(*this); }
	void OnRender() override;
	bool OnInput(IInput::CEvent Event) override;

	// popup elements
	void CreatePopupBox(int WindowFlags, const char *pWindowName, float Width, float AppendHeight, const char *pMessage, PopupWindowCallback Callback, CWindowUI *pWindow);
	void CreatePopupBox(int WindowFlags, const char *pWindowName, float Width, float AppendHeight, const char *pMessage, PopupWindowCallback Callback, bool *pDepent = nullptr);

	// message elements
	void CreateInformationBox(const char *pWindowName, float Width, const char *pMessage, CWindowUI *pWindow);
	void CreateInformationBox(const char *pWindowName, float Width, const char *pMessage, bool *pDepent = nullptr);

private:
	void Update(bool *pCursor) const;
};

#endif