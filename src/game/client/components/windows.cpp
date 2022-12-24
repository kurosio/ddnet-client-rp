#include <game/client/ui_rect.h>
#include "windows.h"

#include <engine/keys.h>

#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/ui.h>

#include <utility>

#include "menus.h"


/*
	Windows GUI elements are placed between CMenus and CWindows
	Because Teeworlds does not support a layer-by-layer UI checker.
*/

CWindowController::~CWindowController()
{
	for(auto *pElement : m_aElements)
		delete pElement;
	for(auto *pWindow : CWindowUI::ms_aWindows)
		delete pWindow;

	m_aElements.clear();
	CWindowUI::ms_aWindows.clear();
}

void CWindowController::OnRender()
{
	// update
	bool ShowCursor = false;
	Update(&ShowCursor);

	// render cursor
	if(ShowCursor)
		RenderTools()->RenderCursor(vec2(UI()->MouseX(), UI()->MouseY()), 24.f);

	// finish all checks button logics
	UI()->FinishCheck();
}

bool CWindowController::OnInput(IInput::CEvent Event)
{
	// hotkeys
	if(Input()->KeyIsPressed(KEY_LCTRL))
	{
		if(CWindowUI *pWindowActive = CWindowUI::GetActiveWindow())
		{
			if((pWindowActive->m_Flags & CWindowUI::WINDOWFLAG_CLOSE) && Input()->KeyPress(KEY_Q))
			{
				pWindowActive->Close();
				return true;
			}

			if((pWindowActive->m_Flags & CWindowUI::WINDOWFLAG_MINIMIZE) && Input()->KeyPress(KEY_M))
			{
				pWindowActive->MinimizeWindow();
				return true;
			}
		}
	}

	return false;
}

void CWindowController::Update(bool* pCursor) const
{
	// update hovered the active highlighted area
	if(auto pHovered = std::find_if(CWindowUI::ms_aWindows.begin(), CWindowUI::ms_aWindows.end(), [this](CWindowUI *p) 
		{ return p->IsRenderAllowed() && (UI()->MouseInside(&p->m_MainRect) || p->IsMoving()); }); pHovered != CWindowUI::ms_aWindows.end())
		UI()->SetHoveredWindow(*pHovered);

	// draw in reverse order as they are sorted here
	auto [ScreenX, ScreenY, ScreenW, ScreenH] = *UI()->Screen();
	Graphics()->MapScreen(ScreenX, ScreenY, ScreenW, ScreenH);
	for(int i = static_cast<int>(CWindowUI::ms_aWindows.size()) - 1; i >= 0; i--)
	{
		CWindowUI *pWindow = CWindowUI::ms_aWindows[i];
		if(pWindow->IsRenderAllowed())
		{
			// start check only this window
			UI()->StartCheckWindow(pWindow);

			if(CWindowUI::GetActiveWindow() != pWindow && Input()->KeyPress(KEY_MOUSE_1) && UI()->MouseHovered(&pWindow->m_MainRect))
				CWindowUI::SetActiveWindow(pWindow);

			// end check only this window
			UI()->FinishCheckWindow();

			pWindow->Render();
			*pCursor = true;
		}
	}

	// clear hovered active highlighted area
	UI()->SetHoveredWindow(nullptr);
}

/* =====================================================================
 * Window UI Elements                                           |     UI
 * ===================================================================== */
static float s_InformationBoxLabelSpace = 8.0f;
template<class T, std::enable_if_t<std::is_convertible_v<T *, BaseElemUI *>, bool> = true>
static void UpdateElement(std::vector<BaseElemUI *> &paElements, T *pElement)
{
	const char *pName = pElement->m_pWindow->GetWindowName();
	const auto pItem = std::find_if(paElements.begin(), paElements.end(), [pName](const BaseElemUI *p) {
		return str_comp(pName, p->m_pWindow->GetWindowName()) == 0;
	});

	if(pItem != paElements.end())
	{
		delete(*pItem);
		(*pItem) = pElement;
	}
	else
		paElements.push_back(pElement);
}

/* =====================================================================
 * Information	Box                                             |     UI
 * ===================================================================== */
constexpr float s_MessageBoxFontSize = 10.0f;
MessageElemUI *CWindowController::CreateInformationBoxElement(float Width, const char *pMessage) const
{
	const CUIRect *pScreen = UI()->Screen();
	Graphics()->MapScreen(pScreen->x, pScreen->y, pScreen->w, pScreen->h);

	const auto pElement = new MessageElemUI;
	str_copy(pElement->m_aMessageText, pMessage);
	return pElement;
}

void CWindowController::CreateInformationBox(const char *pWindowName, float Width, const char *pMessage, bool *pDepent)
{
	const int LineCount = TextRender()->TextLineCount(nullptr, s_MessageBoxFontSize, pMessage, Width);

	MessageElemUI *pElement = CreateInformationBoxElement(Width, pMessage);
	pElement->m_pWindow = UI()->CreateWindow(pWindowName, vec2(Width, 80.0f + (static_cast<float>(LineCount) * s_MessageBoxFontSize)), pDepent);
	pElement->m_pWindow->Register(WINREGISTER(&CWindowController::CallbackRenderInfoWindow, this));
	pElement->m_pWindow->Open();

	UpdateElement(m_aElements, pElement);
}

void CWindowController::CreateInformationBox(const char *pWindowName, float Width, const char *pMessage, CWindowUI *pWindow)
{
	const int LineCount = TextRender()->TextLineCount(nullptr, s_MessageBoxFontSize, pMessage, Width);

	MessageElemUI *pElement = CreateInformationBoxElement(Width, pMessage);
	pElement->m_pWindow = pWindow->AddChild(pWindowName, vec2(Width, 80.0f + (static_cast<float>(LineCount) * s_MessageBoxFontSize)));
	pElement->m_pWindow->Register(WINREGISTER(&CWindowController::CallbackRenderInfoWindow, this));
	pElement->m_pWindow->Open();

	UpdateElement(m_aElements, pElement);
}

void CWindowController::CallbackRenderInfoWindow(CUIRect MainView, CWindowUI *pCurrentWindow)
{
	const MessageElemUI *pElemPopup = reinterpret_cast<MessageElemUI *>(*std::find_if(m_aElements.begin(), m_aElements.end(), [&pCurrentWindow](const BaseElemUI *p) 
											{ return (p->m_pWindow == pCurrentWindow); }));

	CUIRect Label{}, ButtonOk{};
	MainView.Margin(s_InformationBoxLabelSpace, &Label);
	Label.HSplitBottom(24.0f, &Label, &ButtonOk);
	TextRender()->Text(nullptr, Label.x, Label.y, 10.0f, pElemPopup->m_aMessageText, MainView.w);

	static CButtonContainer s_ButtonOk;
	if(m_pClient->m_Menus.DoButton_Menu(&s_ButtonOk, "Ok", false, &ButtonOk, nullptr))
		pCurrentWindow->Close();
}


/* =====================================================================
 * Popup element                                                |     UI
 * ===================================================================== */
constexpr float s_PopupFontSize = 10.0f;
PopupElemUI *CWindowController::CreatePopupElement(const char *pMessage, PopupWindowCallback Callback) const
{
	const CUIRect *pScreen = UI()->Screen();
	Graphics()->MapScreen(pScreen->x, pScreen->y, pScreen->w, pScreen->h);

	const auto pElement = new PopupElemUI;
	str_copy(pElement->m_aTextPopup, pMessage);
	pElement->m_pCallback = std::move(Callback);
	return pElement;
}

void CWindowController::CreatePopupBox(const char *pWindowName, float Width, const char *pMessage, PopupWindowCallback Callback, bool *pDepent)
{
	const int LineCount = TextRender()->TextLineCount(nullptr, s_PopupFontSize, pMessage, Width);

	PopupElemUI *pElement = CreatePopupElement(pMessage, std::move(Callback));
	pElement->m_pWindow = UI()->CreateWindow(pWindowName, vec2(Width, 80.0f + (static_cast<float>(LineCount) * s_PopupFontSize)), pDepent);
	pElement->m_pWindow->Register(WINREGISTER(&CWindowController::CallbackRenderGuiPopupBox, this));
	pElement->m_pWindow->Open();

	UpdateElement(m_aElements, pElement);
}

void CWindowController::CreatePopupBox(const char *pWindowName, float Width, const char *pMessage, PopupWindowCallback Callback, CWindowUI *pWindow)
{
	const int LineCount = TextRender()->TextLineCount(0, s_PopupFontSize, pMessage, Width);

	PopupElemUI *pElement = CreatePopupElement(pMessage, std::move(Callback));
	pElement->m_pWindow = pWindow->AddChild(pWindowName, vec2(Width, 80.0f + (static_cast<float>(LineCount) * s_PopupFontSize)));
	pElement->m_pWindow->Register(WINREGISTER(&CWindowController::CallbackRenderGuiPopupBox, this));
	pElement->m_pWindow->Open();

	UpdateElement(m_aElements, pElement);
}

void CWindowController::CallbackRenderGuiPopupBox(CUIRect MainView, CWindowUI *pCurrentWindow)
{
	const PopupElemUI *pElemPopup = reinterpret_cast<PopupElemUI *>(*std::find_if(m_aElements.begin(), m_aElements.end(), [&pCurrentWindow](const BaseElemUI *p) 
											{ return (p->m_pWindow == pCurrentWindow); }));

	CUIRect Label{}, ButtonAccept{}, ButtonDeny{};
	MainView.Margin(s_InformationBoxLabelSpace, &Label);
	Label.HSplitBottom(18.0f, &Label, &ButtonAccept);
	TextRender()->Text(nullptr, Label.x, Label.y, 10.0f, pElemPopup->m_aTextPopup, MainView.w);
	ButtonAccept.VSplitLeft(Label.w / 2.0f, &ButtonDeny, &ButtonAccept);

	// buttons yes and no
	static CButtonContainer s_ButtonAccept;
	ButtonAccept.VMargin(5.0f, &ButtonAccept);
	if(m_pClient->m_Menus.DoButton_Menu(&s_ButtonAccept, "Yes", 0, &ButtonAccept))
	{
		if(pElemPopup->m_pCallback)
			pElemPopup->m_pCallback(pCurrentWindow, true);
	}

	static CButtonContainer s_ButtonDeny;
	ButtonDeny.VMargin(5.0f, &ButtonDeny);
	if(m_pClient->m_Menus.DoButton_Menu(&s_ButtonDeny, "No", 0, &ButtonDeny))
	{
		if(pElemPopup->m_pCallback)
			pElemPopup->m_pCallback(pCurrentWindow, false);
	}
}