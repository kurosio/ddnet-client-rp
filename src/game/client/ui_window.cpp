/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "ui_window.h"

#include <engine/keys.h>
#include <engine/textrender.h>

#include "game/localization.h"

#include "gameclient.h"
#include "render.h"

CUI* CWindowUI::m_pUI;
CRenderTools* CWindowUI::m_pRenderTools;
CWindowUI* CWindowUI::ms_pWindowHelper;
std::vector<CWindowUI*> CWindowUI::ms_aWindows;

constexpr float s_BackgroundMargin = 2.0f;

// The basic logic
void CWindowUI::RenderWindowWithoutBordure()
{
	CUIRect Workspace{};
	m_WindowRect.Margin(s_BackgroundMargin, &Workspace);

	// background draw
	CUIRect Background{};
	Workspace.Margin(-s_BackgroundMargin, &Background);
	DrawUIRect(&Background, ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 40.0f);
	DrawUIRect(&Workspace, ColorRGBA(m_BackgroundColor), IGraphics::CORNER_ALL, 2.0f);

	// render callback
	if(m_pCallback)
	{
		m_pCallback(Workspace, this);
	}
}

void CWindowUI::RenderDefaultWindow()
{
	/*
	 * Moving window logic
	 */
	static float s_WindowSkipMovingX;
	static float s_WindowSkipMovingY;
	if(m_WindowMoving)
	{
		m_WindowRect.x = m_WindowBordure.x = clamp(m_pUI->MouseX() - s_WindowSkipMovingX, 0.0f, m_pUI->Screen()->w - m_WindowRect.w);
		m_WindowRect.y = m_WindowBordure.y = clamp(m_pUI->MouseY() - s_WindowSkipMovingY, 0.0f, m_pUI->Screen()->h - m_WindowRect.h);
		if(!m_pUI->Input()->KeyIsPressed(KEY_MOUSE_1))
		{
			m_WindowMoving = false;
			s_WindowSkipMovingX = 0.0f;
			s_WindowSkipMovingY = 0.0f;
		}
	}

	/*
	 * Scale bordure
	 */
	constexpr float Rounding = 7.0f;
	constexpr float FontSize = 10.0f;
	constexpr float BordureWidth = 15.0f;

	/*
	 * Set bordure rect
	 */
	CUIRect Workspace{};
	m_WindowRect.HSplitTop(BordureWidth, &m_WindowBordure, &Workspace);

	/*
	 * Background draw
	 */
	CUIRect ShadowBackground{};
	m_WindowRect.Margin(-1.5f, &ShadowBackground);
	DrawUIRect(&ShadowBackground, ColorRGBA(DEFAULT_BACKGROUND_WINDOW_SHANDOW), IGraphics::CORNER_ALL, Rounding);
	
	const bool IsActiveWindow = IsActive();
	if(!m_WindowMinimize)
	{
		const float BackgroundFade = m_pUI->GetFade(&Workspace, IsActiveWindow, 0.4f);
		const vec4 Color = mix(m_BackgroundColor -= vec4(0.02f, 0.02f, 0.02f, 0.f), m_BackgroundColor, BackgroundFade);
		DrawUIRect(&Workspace, ColorRGBA(Color), IGraphics::CORNER_ALL, Rounding);
	}
	
	/*
	 * Bordure draw
	 */
	const float BordureFade = m_pUI->GetFade(&m_WindowBordure, IsActiveWindow);
	const vec4 Color = mix(vec4(0.2f, 0.2f, 0.2f, 1.0f), vec4(0.4f, 0.4f, 0.4f, 1.0f), BordureFade);
	DrawUIRect(&m_WindowBordure, ColorRGBA(Color), m_WindowMinimize ? IGraphics::CORNER_ALL : IGraphics::CORNER_T | IGraphics::CORNER_IB, Rounding);

	/*
	 * Window name
	 */
	CUIRect Label{};
	m_WindowBordure.VSplitLeft(10.0f, 0, &Label);
	m_pUI->DoLabel(&Label, m_aWindowName, FontSize, TEXTALIGN_LEFT);
	
	/*
	 * Button bordure top func
	 */
	bool DissalowWindowMoving = false;
	auto CreateButtonTop = [this, &IsActiveWindow, &FontSize, &Rounding, &DissalowWindowMoving](CUIRect* pButtonRect, const char* pHintStr, vec4 ColorFade1, vec4 ColorFade2, const char* pSymbolUTF) -> bool
	{
		CUIRect Button = *(pButtonRect);
		const vec4 ColorFinal = mix(ColorFade1, ColorFade2, m_pUI->GetFade(&Button, false));
		DrawUIRect(&Button, ColorRGBA(ColorFinal), IGraphics::CORNER_ALL, Rounding);
		m_pUI->DoLabel(&Button, pSymbolUTF, FontSize, TEXTALIGN_CENTER);
		pButtonRect->x -= 24.f;

		bool Active = false;
		if(IsActiveWindow && m_pUI->MouseHovered(&Button))
		{
			const char* HotKeyLabel = Localize(pHintStr);
			const float TextWidth = m_pUI->TextRender()->TextWidth(0, FontSize, HotKeyLabel, -1, -1.0f);
			CUIRect BackgroundKeyPress = { 0.f, 0.f, 10.0f + TextWidth, 20.f };
			m_pUI->MouseRectLimitMapScreen(&BackgroundKeyPress, 12.0f, CUI::RECTLIMITSCREEN_UP | CUI::RECTLIMITSCREEN_SKIP_BORDURE_UP);
			DrawUIRect(&BackgroundKeyPress, vec4(0.1f, 0.1f, 0.1f, 0.5f), IGraphics::CORNER_ALL, 3.0f);

			CUIRect LabelKeyInfo = BackgroundKeyPress;
			LabelKeyInfo.Margin(s_BackgroundMargin, &LabelKeyInfo);
			m_pUI->DoLabel(&LabelKeyInfo, HotKeyLabel, FontSize, TEXTALIGN_CENTER);

			Active = m_pUI->Input()->KeyPress(KEY_MOUSE_1);
		}

		if(Active)
			DissalowWindowMoving = true;

		return Active;
	};
	
	/*
	 * Callback func
	 */
	if(!m_WindowMinimize && m_pCallback)
	{
		Workspace.Margin(s_BackgroundMargin, &Workspace);
		m_pCallback(Workspace, this);
	}

	/*
	 * Buttons
	 */
	{
		CUIRect ButtonTop{};
		m_WindowBordure.VSplitRight(24.0f, nullptr, &ButtonTop);

		// close button
		if(m_WindowFlags & WINDOWFLAG_CLOSE && CreateButtonTop(&ButtonTop, "Left Ctrl + Q - close.", vec4(0.f, 0.f, 0.f, 0.25f), vec4(0.7f, 0.1f, 0.1f, 0.75f), "\xE2\x9C\x95"))  
			Close();

		// minimize button
		if(m_WindowFlags & WINDOWFLAG_MINIMIZE && CreateButtonTop(&ButtonTop, "Left Ctrl + M - minimize.", vec4(0.f, 0.f, 0.f, 0.25f), vec4(0.2f, 0.2f, 0.7f, 0.75f), m_WindowMinimize ? "\xe2\x81\x82" : "\xe2\x80\xbb"))
			MinimizeWindow();

		// helppage button
		if(m_pCallbackHelp)
		{
			CWindowUI* pWindowHelppage = GetChild("Help");
			if(CreateButtonTop(&ButtonTop, "Show attached help.", pWindowHelppage->IsOpenned() ? vec4(0.1f, 0.3f, 0.1f, 0.75f) : vec4(0.f, 0.f, 0.f, 0.25f), vec4(0.2f, 0.5f, 0.2f, 0.75f), "?"))
			{
				pWindowHelppage->Register(m_pCallbackHelp);
				pWindowHelppage->Reverse();
			}
		}
	}

	/*
	 * Bordure moving
	 */
	if(DissalowWindowMoving)
	{
		m_WindowMoving = false;
	}
	else if(m_pUI->Input()->KeyPress(KEY_MOUSE_1) && m_pUI->MouseHovered(&m_WindowBordure))
	{
		m_WindowMoving = true;
		s_WindowSkipMovingX = (m_pUI->MouseX() - m_WindowRect.x);
		s_WindowSkipMovingY = (m_pUI->MouseY() - m_WindowRect.y);
	}
}

void CWindowUI::Render()
{
	// render only if is allowed
	if(IsRenderAllowed())
	{
		// start check only this window
		m_pUI->StartCheckWindow(this);

		// render window types
		if(m_WindowFlags & WINDOW_WITHOUT_BORDURE)
			RenderWindowWithoutBordure();
		else
			RenderDefaultWindow();

		// close window when is unactive
		if(m_WindowFlags & WINDOW_CLOSE_CLICKING_OUTSIDE && !IsActive())
			Close();

		// end check only this window
		m_pUI->FinishCheckWindow();
	}
}

CWindowUI *CWindowUI::SearchWindowByKeyName(std::vector<CWindowUI *> &pVector, const char *pSearchKeyName)
{
	const auto pItem = std::find_if(pVector.begin(), pVector.end(), [pSearchKeyName](const CWindowUI *pItem) 
	{
		return str_comp(pSearchKeyName, pItem->GetWindowName()) == 0;
	});
	return pItem != pVector.end() ? (*pItem) : nullptr;
}

CWindowUI *CWindowUI::AddChild(const char *pChildName, vec2 WindowSize, int WindowFlags)
{
	char aChildNameBuf[64];
	GetFullChildWindowName(pChildName, aChildNameBuf, sizeof(aChildNameBuf));

	CWindowUI *pWindow = SearchWindowByKeyName(ms_aWindows, aChildNameBuf);
	if(!pWindow)
		pWindow = m_pUI->CreateWindow(aChildNameBuf, WindowSize, m_pRenderDependence, WindowFlags);

	return AddChild(pWindow);
}

CWindowUI * CWindowUI::GetChild(const char *pChildName)
{
	char aChildNameBuf[64];
	GetFullChildWindowName(pChildName, aChildNameBuf, sizeof(aChildNameBuf));

	return SearchWindowByKeyName(m_paChildrenWindows, aChildNameBuf);
}

void CWindowUI::GetFullChildWindowName(const char *pChildName, char *aBuf, int Size)
{
	str_format(aBuf, Size, "%s : %s", m_aWindowName, pChildName);
}

bool CWindowUI::operator==(const CWindowUI &p) const
{
	return str_comp(m_aWindowName, p.m_aWindowName) == 0;
}

void CWindowUI::InitComponents(CUI *pUI, CRenderTools *pRenderTools)
{
	m_pUI = pUI;
	m_pRenderTools = pRenderTools;
	if(!ms_pWindowHelper)
		ms_pWindowHelper = m_pUI->CreateWindow("Help", vec2(0, 0), nullptr, WINDOWFLAG_CLOSE);
}

void CWindowUI::MinimizeWindow()
{
	m_WindowMinimize ^= true;
	if(m_WindowMinimize)
	{
		m_WindowRectReserve = m_WindowRect;
		m_WindowRect = m_WindowBordure;
		return;
	}

	m_WindowRectReserve.x = clamp(m_WindowRect.x, 0.0f, m_pUI->Screen()->w - m_WindowRectReserve.w);
	m_WindowRectReserve.y = clamp(m_WindowRect.y, 0.0f, m_pUI->Screen()->h - m_WindowRectReserve.h);
	m_WindowRect = m_WindowRectReserve;
}

// - - -- - -- - -- - -- - -- - -- - -
// Functions for working with windows
void CWindowUI::Init(vec2 WindowSize, bool* pRenderDependence)
{
	m_WindowBordure = { 0, 0, 0, 0 };
	m_WindowRect = { 0, 0, WindowSize.x, WindowSize.y };
	m_DefaultWindowRect = m_WindowRect;
	m_WindowRectReserve = m_WindowRect;
	m_WindowMinimize = false;
	m_WindowMoving = false;
	m_BackgroundColor = DEFAULT_BACKGROUND_WINDOW_COLOR;
	if(pRenderDependence)
		m_pRenderDependence = pRenderDependence;
}

const CUIRect& CWindowUI::GetRect() const
{
	return m_WindowRect;
}

CWindowUI::CWindowUI(const char* pWindowName, vec2 WindowSize, bool* pRenderDependence, int WindowFlags)
{
	m_Openned = false;
	m_WindowFlags = WindowFlags;
	str_copy(m_aWindowName, pWindowName, sizeof(m_aWindowName));
	Init(WindowSize, pRenderDependence);
}

bool CWindowUI::IsOpenned() const
{
	return m_Openned;
}

bool CWindowUI::IsActive() const
{
	return (bool)(m_Openned && GetActiveWindow() == this);
}

void CWindowUI::Open()
{
	CUIRect NewWindowRect = { 0, 0, m_WindowRectReserve.w, m_WindowRectReserve.h };
	m_pUI->MouseRectLimitMapScreen(&NewWindowRect, 6.0f, CUI::RECTLIMITSCREEN_UP | CUI::RECTLIMITSCREEN_ALIGN_CENTER_X);

	m_WindowRect = NewWindowRect;
	m_Openned = true;
	m_WindowMoving = false;
	m_WindowMinimize = false;
	SetActiveWindow(this);
}

void CWindowUI::Close()
{
	m_Openned = false;
	for(auto& p : m_paChildrenWindows)
		p->Close();
}

void CWindowUI::Reverse()
{
	if(m_Openned)
		Close();
	else
		Open();
}

void CWindowUI::Register(RenderWindowCallback pCallback, RenderWindowCallback pCallbackHelp)
{
	m_pCallback = std::move(pCallback);

	if(pCallbackHelp)
	{
		AddChild("Help", {300, 200});
		m_pCallbackHelp = std::move(pCallbackHelp);
	}
}

void CWindowUI::SetWorkspace(vec2 WorkspaceSize)
{
	float Width = round_to_int(WorkspaceSize.x) == round_to_int(DEFAULT_WORKSPACE_SIZE) ? m_WindowRect.w : WorkspaceSize.x;
	float Height = round_to_int(WorkspaceSize.y) == round_to_int(DEFAULT_WORKSPACE_SIZE) ? m_WindowRect.h : WorkspaceSize.y;

	CUIRect NewWindowRect = {0, 0, Width, Height};
	if(round_to_int(NewWindowRect.w) != round_to_int(m_WindowRect.w) || round_to_int(NewWindowRect.h) != round_to_int(m_WindowRect.h))
	{
		m_pUI->MouseRectLimitMapScreen(&NewWindowRect, 6.0f, CUI::RECTLIMITSCREEN_UP | CUI::RECTLIMITSCREEN_ALIGN_CENTER_X);
		m_WindowRect.w = NewWindowRect.w;
		m_WindowRect.h = NewWindowRect.h;
		m_WindowRectReserve.w = NewWindowRect.w;
		m_WindowRectReserve.h = NewWindowRect.h;
	}
}

void CWindowUI::SetActiveWindow(CWindowUI *pWindow)
{
	const auto pItem = std::find_if(ms_aWindows.begin(), ms_aWindows.end(), [pWindow](const CWindowUI* pItem)
	{
		return pWindow == pItem;
	});
	std::rotate(ms_aWindows.begin(), pItem, pItem + 1);
}

CWindowUI* CWindowUI::GetActiveWindow()
{
	return ms_aWindows.empty() ? nullptr : ms_aWindows.front();
}

void CWindowUI::DrawUIRect(CUIRect *pRect, ColorRGBA Color, int Corner, float Rounding)
{
	m_pRenderTools->Graphics()->DrawRect(pRect->x, pRect->y, pRect->w, pRect->h, Color, Corner, Rounding);
}
