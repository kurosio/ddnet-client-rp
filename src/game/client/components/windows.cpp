#include <engine/keys.h>
#include <game/client/ui.h>
#include "windows.h"

#include <game/client/gameclient.h>
#include <game/client/render.h>

#include <utility>

#include "menus.h"


/*
	Windows GUI elements are placed between CMenus and CWindows
	Because Teeworlds does not support a layer-by-layer UI checker.
*/

CWindowController::~CWindowController()
{
	// free elements
	for(auto *pElement : m_aElements)
		delete pElement;
	for(auto *pWindow : CWindowUI::ms_aWindows)
		delete pWindow;

	// clear
	m_aElements.clear();
	CWindowUI::ms_aWindows.clear();
}

void CWindowController::OnRender()
{
	// update
	bool RenderCursor = false;
	Update(&RenderCursor);

	// render cursor
	if(RenderCursor)
		RenderTools()->RenderCursor(vec2(UI()->MouseX(), UI()->MouseY()), 24.f);

	// finish all checks buttons logics
	UI()->FinishCheck();
}

bool CWindowController::OnInput(IInput::CEvent Event)
{
	// hotkeys
	if(Input()->KeyIsPressed(KEY_LCTRL))
	{
		if(CWindowUI *pWindowActive = CWindowUI::GetActiveWindow(); pWindowActive)
		{
			// close LCTRL + Q
			if((pWindowActive->m_Flags & CWindowUI::FLAG_CLOSE) && Input()->KeyPress(KEY_Q))
			{
				pWindowActive->Close();
				return true;
			}

			// minimize LCTRL + M
			if((pWindowActive->m_Flags & CWindowUI::FLAG_MINIMIZE) && Input()->KeyPress(KEY_M))
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
	if(const auto pItem = std::find_if(CWindowUI::ms_aWindows.begin(), CWindowUI::ms_aWindows.end(), [this](CWindowUI *p) 
		{ return p->IsRenderAllowed() && (UI()->MouseInside(&p->m_CurrentRect) || p->IsMoving()); }); pItem != CWindowUI::ms_aWindows.end())
		UI()->SetHoveredWindow(*pItem);

	// update screen
	auto [ScreenX, ScreenY, ScreenW, ScreenH] = *UI()->Screen();
	Graphics()->MapScreen(ScreenX, ScreenY, ScreenW, ScreenH);

	// draw in reverse order as they are sorted here
	const std::vector vWindows(CWindowUI::ms_aWindows);
	const bool LeftMousePressed = Input()->KeyPress(KEY_MOUSE_1);
	for(int i = static_cast<int>(vWindows.size()) - 1; i >= 0; i--)
	{
		CWindowUI *pWindow = vWindows[i];
		if(pWindow->IsRenderAllowed())
		{
			// start check only this window
			UI()->StartCheckWindow(pWindow);

			// enable selection
			if(LeftMousePressed)
			{
				if(const bool Hovered = UI()->MouseHovered(&pWindow->m_CurrentRect); CWindowUI::GetActiveWindow() != pWindow && Hovered)
					CWindowUI::SetActiveWindow(pWindow);
				else if(CWindowUI::GetActiveWindow() == pWindow && !Hovered)
					CWindowUI::SetActiveWindow(nullptr);
			}

			// render
			pWindow->Render();
			*pCursor = true;

			// end check only this window
			UI()->FinishCheckWindow();
		}
	}

	// clear hovered active highlighted area
	UI()->SetHoveredWindow(nullptr);
}

/* =====================================================================
 * Window UI Elements                                           |     UI
 * ===================================================================== */
template<class T, std::enable_if_t<std::is_convertible_v<T *, BaseElemUI *>, bool> = true>
static void UpdateElement(std::vector<BaseElemUI *> &paElements, T *pElement)
{
	const char *pName = pElement->m_pWindow->GetWindowName();
	const auto pItem = std::find_if(paElements.begin(), paElements.end(), [pName](const BaseElemUI *p) 
	{
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
static float s_InformationBoxLabelSpace = 8.0f;
MessageElemUI *CWindowController::CreateInformationBoxElement(const char *pMessage) const
{
	const CUIRect *pScreen = UI()->Screen();
	Graphics()->MapScreen(pScreen->x, pScreen->y, pScreen->w, pScreen->h);

	const auto pElement = new MessageElemUI;
	str_copy(pElement->m_aText, pMessage);
	return pElement;
}

void CWindowController::CreateInformationBox(const char *pWindowName, float Width, const char *pMessage, bool *pDepent)
{
	const int LineCount = TextRender()->TextLineCount(s_MessageBoxFontSize, pMessage, Width);

	MessageElemUI *pElement = CreateInformationBoxElement(pMessage);
	pElement->m_pWindow = UI()->CreateWindow(pWindowName, vec2(Width, 70.0f + (static_cast<float>(LineCount) * s_MessageBoxFontSize)), pDepent);
	pElement->m_pWindow->Register(WINREGISTER(&CWindowController::CallbackRenderInfoWindow, this));
	pElement->m_pWindow->Open();

	UpdateElement(m_aElements, pElement);
}

void CWindowController::CreateInformationBox(const char *pWindowName, float Width, const char *pMessage, CWindowUI *pWindow)
{
	const int LineCount = TextRender()->TextLineCount(s_MessageBoxFontSize, pMessage, Width);

	MessageElemUI *pElement = CreateInformationBoxElement(pMessage);
	pElement->m_pWindow = pWindow->AddChild(pWindowName, vec2(Width, 70.0f + (static_cast<float>(LineCount) * s_MessageBoxFontSize)));
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
	Label.HSplitBottom(18.0f, &Label, &ButtonOk);
	TextRender()->Text(Label.x, Label.y, s_MessageBoxFontSize, pElemPopup->m_aText, MainView.w);

	ButtonOk.w = MainView.w / 2.f;
	ButtonOk.x = MainView.x + ButtonOk.w / 2.f;
	if(m_pClient->m_Menus.DoButton_Menu(pElemPopup->m_pButtonOk.get(), "OK", false, &ButtonOk, nullptr))
		pCurrentWindow->Close();
}


/* =====================================================================
 * Popup element                                                |     UI
 * ===================================================================== */
constexpr float s_PopupFontSize = 11.0f;
PopupElemUI *CWindowController::CreatePopupElement(const char *pMessage, PopupWindowCallback Callback) const
{
	const CUIRect *pScreen = UI()->Screen();
	Graphics()->MapScreen(pScreen->x, pScreen->y, pScreen->w, pScreen->h);

	const auto pElement = new PopupElemUI;
	str_copy(pElement->m_aText, pMessage);
	pElement->m_pRenderCallback = std::move(Callback);
	return pElement;
}

void CWindowController::CreatePopupBox(int WindowFlags, const char *pWindowName, float Width, float AppendHeight, const char *pMessage, PopupWindowCallback Callback, bool *pDepent)
{
	const int LineCount = TextRender()->TextLineCount(s_PopupFontSize, pMessage, Width);

	PopupElemUI *pElement = CreatePopupElement(pMessage, std::move(Callback));
	pElement->m_pWindow = UI()->CreateWindow(pWindowName, vec2(Width, 70.0f + (static_cast<float>(LineCount) * s_PopupFontSize) + AppendHeight), pDepent, WindowFlags);
	pElement->m_pWindow->Register(WINREGISTER(&CWindowController::CallbackRenderGuiPopupBox, this));
	pElement->m_pWindow->Open();

	UpdateElement(m_aElements, pElement);
}

void CWindowController::CreatePopupBox(int WindowFlags, const char *pWindowName, float Width, float AppendHeight, const char *pMessage, PopupWindowCallback Callback, CWindowUI *pWindow)
{
	const int LineCount = TextRender()->TextLineCount(s_PopupFontSize, pMessage, Width);

	PopupElemUI *pElement = CreatePopupElement(pMessage, std::move(Callback));
	pElement->m_pWindow = pWindow->AddChild(pWindowName, vec2(Width, 70.0f + (static_cast<float>(LineCount) * s_PopupFontSize) + AppendHeight), WindowFlags);
	pElement->m_pWindow->Register(WINREGISTER(&CWindowController::CallbackRenderGuiPopupBox, this));
	pElement->m_pWindow->Open();

	UpdateElement(m_aElements, pElement);
}

void CWindowController::CallbackRenderGuiPopupBox(CUIRect MainView, CWindowUI *pCurrentWindow)
{
	const PopupElemUI *pElemPopup = reinterpret_cast<PopupElemUI *>(*std::find_if(m_aElements.begin(), m_aElements.end(), [&pCurrentWindow](const BaseElemUI *p) 
											{ return (p->m_pWindow == pCurrentWindow); }));

	CUIRect Label{}, ButtonAccept{}, ButtonDeny{}, Buttons{};
	const int TextLines = TextRender()->TextLineCount(s_PopupFontSize, pElemPopup->m_aText, MainView.w);
	MainView.Margin(s_InformationBoxLabelSpace, &MainView);
	MainView.HSplitTop(static_cast<float>(TextLines) * s_PopupFontSize, &Label, &MainView);
	MainView.HSplitBottom(18.0f, &MainView, &Buttons);
	MainView.HMargin(3.0f, &MainView);
	TextRender()->Text(Label.x, Label.y, s_PopupFontSize, pElemPopup->m_aText, MainView.w);

	// buttons yes and no
	Buttons.VSplitLeft(MainView.w / 2.f, &ButtonDeny, &ButtonAccept);
	PopupEvent State = PopupEvent::RENDER;
	ButtonAccept.VMargin(5.0f, &ButtonAccept);
	if(m_pClient->m_Menus.DoButton_Menu(pElemPopup->m_pButtonYes.get(), "Yes", 0, &ButtonAccept))
		State = PopupEvent::YES;

	ButtonDeny.VMargin(5.0f, &ButtonDeny);
	if(m_pClient->m_Menus.DoButton_Menu(pElemPopup->m_pButtonNo.get(), "No", 0, &ButtonDeny))
		State = PopupEvent::NO;

	pElemPopup->m_pRenderCallback(MainView, pCurrentWindow, State);
}