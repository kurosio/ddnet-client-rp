/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "ui_window.h"

#include <engine/keys.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/localization.h>

#include "render.h"
#include "ui.h"

constexpr float s_BackgroundMargin = 2.0f;
constexpr float s_Rounding = 7.0f;
constexpr float s_FontSize = 12.0f;
constexpr float s_BordureWidth = 18.0f;

// The basic logic
void CWindowUI::RenderWindowWithoutBordure()
{
	CUIRect Workspace{};
	m_MainRect.Margin(s_BackgroundMargin, &Workspace);

	// background draw
	CUIRect Background{};
	Workspace.Margin(-s_BackgroundMargin, &Background);
	Background.DrawMonochrome(m_ColorTone / 1.5f, IGraphics::CORNER_ALL, s_Rounding);

	const float BackgroundFade = m_pUI->GetFade(&Workspace, IsActive(), 0.4f);
	const vec4 Color = mix(m_ColorTone, m_ColorTone + vec4(0.02f, 0.02f, 0.02f, 0.f), BackgroundFade);
	Workspace.Draw(ColorRGBA(Color), IGraphics::CORNER_ALL, s_Rounding);

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
	if(m_Moving)
	{
		m_MainRect.x = m_BordureRect.x = clamp(m_pUI->MouseX() - s_WindowSkipMovingX, 0.0f, m_pUI->Screen()->w - m_MainRect.w);
		m_MainRect.y = m_BordureRect.y = clamp(m_pUI->MouseY() - s_WindowSkipMovingY, 0.0f, m_pUI->Screen()->h - m_MainRect.h);
		if(!m_pUI->Input()->KeyIsPressed(KEY_MOUSE_1))
		{
			m_Moving = false;
			s_WindowSkipMovingX = 0.0f;
			s_WindowSkipMovingY = 0.0f;
		}
	}

	/*
	 * Set bordure rect
	 */
	CUIRect Workspace{};
	m_MainRect.HSplitTop(s_BordureWidth, &m_BordureRect, &Workspace);

	/*
	 * Background draw
	 */
	const bool IsActiveWindow = IsActive();
	{
		CUIRect ShadowBackground{};
		m_MainRect.Margin(-1.5f, &ShadowBackground);
		ShadowBackground.DrawMonochrome(m_ColorTone / 1.5f, IGraphics::CORNER_ALL, s_Rounding);

		if(!m_Minimized)
		{
			const float BackgroundFade = m_pUI->GetFade(&Workspace, IsActiveWindow, 0.4f);
			const vec4 Color = mix(m_ColorTone, m_ColorTone + vec4(0.02f, 0.02f, 0.02f, 0.f), BackgroundFade);
			Workspace.Draw(ColorRGBA(Color), IGraphics::CORNER_ALL, s_Rounding);
		}
	}

	/*
	 * Bordure background
	 */
	{
		const float BordureFade = m_pUI->GetFade(&m_BordureRect, IsActiveWindow);
		const vec4 Color = mix(m_ColorTone, m_ColorTone + vec4(0.2f, 0.2f, 0.2f, 0.f), BordureFade);
		m_BordureRect.DrawMonochrome(ColorRGBA(Color), m_Minimized ? IGraphics::CORNER_ALL : IGraphics::CORNER_T | IGraphics::CORNER_IB, s_Rounding);
	}

	/*
	 * Callback func
	 */
	if(!m_Minimized && m_pCallback)
	{
		Workspace.Margin(s_BackgroundMargin, &Workspace);
		m_pCallback(Workspace, this);
	}
		
	/*
	 * Bordure top
	 */
	bool DissalowWindowMoving = false;
	{
		// Lambda Bordure buttons function
		auto CreateButtonTop = [&](int *pCounter, CUIRect *pButtonRect, const char *pHintStr, vec4 ColorFade1, vec4 ColorFade2, const char *pSymbolUTF) -> bool {
			CUIRect Button = *(pButtonRect);
			const vec4 ColorFinal = mix(ColorFade1, ColorFade2, m_pUI->GetFade(&Button, false));
			Button.DrawMonochrome(ColorRGBA(ColorFinal), IGraphics::CORNER_ALL, s_Rounding);
			m_pUI->DoLabel(&Button, pSymbolUTF, s_FontSize, TEXTALIGN_CENTER);
			pButtonRect->x -= 24.f;
			if(pCounter)
				*pCounter += 1;

			bool Active = false;
			if(IsActiveWindow && m_pUI->MouseHovered(&Button))
			{
				constexpr float FontSizeHint = 8.0f;
				const char *HotKeyLabel = Localize(pHintStr);
				const float TextWidth = m_pUI->TextRender()->TextWidth(FontSizeHint, HotKeyLabel, -1, -1.0f);

				CUIRect BackgroundKeyPress = {m_pUI->MouseX(), m_pUI->MouseY(), 6.0f + TextWidth, FontSizeHint + s_BackgroundMargin};
				m_pUI->RectLimitMapScreen(&BackgroundKeyPress, 12.0f, CUI::RECTLIMITSCREEN_UP | CUI::RECTLIMITSCREEN_SKIP_BORDURE_UP);
				BackgroundKeyPress.Draw(vec4(0.1f, 0.1f, 0.1f, 0.5f), IGraphics::CORNER_ALL, 3.0f);

				CUIRect LabelKeyInfo = BackgroundKeyPress;
				LabelKeyInfo.Margin(s_BackgroundMargin, &LabelKeyInfo);
				m_pUI->DoLabel(&LabelKeyInfo, HotKeyLabel, FontSizeHint, TEXTALIGN_CENTER);

				Active = m_pUI->Input()->KeyPress(KEY_MOUSE_1);
			}

			if(Active)
				DissalowWindowMoving = true;

			return Active;
		};

		// Bordure top
		int ButtonsNum = 0;
		CUIRect ButtonTop{};
		m_BordureRect.VSplitRight(24.0f, nullptr, &ButtonTop);

		// close button
		if(m_Flags & FLAG_CLOSE && CreateButtonTop(&ButtonsNum, &ButtonTop, "Left Ctrl + Q - close.", vec4(0.f, 0.f, 0.f, 0.25f), vec4(0.7f, 0.1f, 0.1f, 0.75f), "\xE2\x9C\x95"))  
			Close();

		// minimize button
		if(m_Flags & FLAG_MINIMIZE && CreateButtonTop(&ButtonsNum, &ButtonTop, "Left Ctrl + M - minimize.", vec4(0.f, 0.f, 0.f, 0.25f), vec4(0.2f, 0.2f, 0.7f, 0.75f), m_Minimized ? "\xe2\x81\x82" : "\xe2\x80\xbb"))
			MinimizeWindow();

		// helppage button
		if(m_pCallbackHelp)
		{
			CWindowUI* pWindowHelppage = GetChild("Help");
			if(CreateButtonTop(&ButtonsNum, &ButtonTop, "Show attached help.", pWindowHelppage->IsOpenned() ? vec4(0.1f, 0.3f, 0.1f, 0.75f) : vec4(0.f, 0.f, 0.f, 0.25f), vec4(0.2f, 0.5f, 0.2f, 0.75f), "?"))
			{
				pWindowHelppage->Register(m_pCallbackHelp);
				pWindowHelppage->Reverse();
			}
		}

		// Window name
		CUIRect Label{};
		dbg_msg("test", "%d", ButtonsNum);
		m_BordureRect.VSplitRight(ButtonTop.w * static_cast<float>(ButtonsNum), &Label, 0);
		m_pUI->DoLabel(&Label, m_aName, s_FontSize, TEXTALIGN_LEFT);
	}

	/*
	 * Bordure moving
	 */
	if(DissalowWindowMoving)
	{
		m_Moving = false;
	}
	else if(m_pUI->Input()->KeyPress(KEY_MOUSE_1) && m_pUI->MouseHovered(&m_BordureRect))
	{
		m_Moving = true;
		s_WindowSkipMovingX = (m_pUI->MouseX() - m_MainRect.x);
		s_WindowSkipMovingY = (m_pUI->MouseY() - m_MainRect.y);
	}
}

void CWindowUI::Render()
{
	// render only if is allowed
	if(IsRenderAllowed())
	{
		// render window types
		if(m_Flags & FLAG_WITHOUT_BORDURE)
			RenderWindowWithoutBordure();
		else
			RenderDefaultWindow();

		// close window when clicking outside
		if(m_Flags & FLAG_CLOSE_CLICKING_OUTSIDE && !m_pUI->MouseHovered(&m_MainRect) && m_pUI->Input()->KeyPress(KEY_MOUSE_1))
			Close();
	}
}

CWindowUI *CWindowUI::SearchWindowByKeyName(std::vector<CWindowUI *> &pVector, const char *pSearchKeyName)
{
	const auto pItem = std::find_if(pVector.begin(), pVector.end(), [pSearchKeyName](const CWindowUI *p) 
	{
		return str_comp(pSearchKeyName, p->GetWindowName()) == 0;
	});
	return pItem != pVector.end() ? (*pItem) : nullptr;
}

CWindowUI *CWindowUI::AddChild(const char *pName, vec2 WindowSize, int WindowFlags)
{
	char aChildNameBuf[64];
	GetFullChildWindowName(pName, aChildNameBuf, sizeof(aChildNameBuf));

	CWindowUI *pWindow = SearchWindowByKeyName(ms_aWindows, aChildNameBuf);
	if(!pWindow)
	{
		pWindow = m_pUI->CreateWindow(aChildNameBuf, WindowSize, m_pRenderDependence, WindowFlags);
		pWindow->m_Parent = this;
	}
	return AddChild(pWindow);
}

CWindowUI * CWindowUI::GetChild(const char *pName)
{
	char aChildNameBuf[64];
	GetFullChildWindowName(pName, aChildNameBuf, sizeof(aChildNameBuf));

	return SearchWindowByKeyName(m_paChildrens, aChildNameBuf);
}

void CWindowUI::GetFullChildWindowName(const char *pName, char *aBuf, int Size)
{
	str_format(aBuf, Size, "%s - %s", m_aName, pName);
}

bool CWindowUI::operator==(const CWindowUI &p) const
{
	return str_comp(m_aName, p.m_aName) == 0;
}

void CWindowUI::InitComponents(CUI *pUI, CRenderTools *pRenderTools)
{
	m_pUI = pUI;
	m_pRenderTools = pRenderTools;
	m_pActiveWindow = nullptr;
}

void CWindowUI::MinimizeWindow()
{
	m_Minimized ^= true;
	if(m_Minimized)
	{
		m_ReserveRect = m_MainRect;
		m_MainRect = m_BordureRect;
		return;
	}

	m_ReserveRect.x = clamp(m_MainRect.x, 0.0f, m_pUI->Screen()->w - m_ReserveRect.w);
	m_ReserveRect.y = clamp(m_MainRect.y, 0.0f, m_pUI->Screen()->h - m_ReserveRect.h);
	m_MainRect = m_ReserveRect;
}

// - - -- - -- - -- - -- - -- - -- - -
// Functions for working with windows
void CWindowUI::Init(vec2 WindowSize, bool* pRenderDependence)
{
	m_BordureRect = { 0, 0, 0, 0 };
	m_MainRect = { 0, 0, WindowSize.x, WindowSize.y };
	m_DefaultRect = m_MainRect;
	m_ReserveRect = m_MainRect;
	m_Minimized = false;
	m_Moving = false;
	if(pRenderDependence)
		m_pRenderDependence = pRenderDependence;

	SetColorTone(ColorHSLA(g_Config.m_UiDefaultWindowColor, true));
}

const CUIRect& CWindowUI::GetRect() const
{
	return m_MainRect;
}

CWindowUI::CWindowUI(const char *pWindowName, vec2 WindowSize, bool *pRenderDependence, int WindowFlags)
{
	m_Parent = nullptr;
	m_Openned = false;
	m_Flags = WindowFlags;
	str_copy(m_aName, pWindowName, sizeof(m_aName));
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

void CWindowUI::Open(float X, float Y)
{
	for(const auto &p : m_paChildrens)
		p->Close();

	CUIRect NewWindowRect = {X <= 0.f ? m_pUI->MouseX() : X, Y <= 0.f ? m_pUI->MouseY() : Y, m_ReserveRect.w, m_ReserveRect.h };
	m_pUI->RectLimitMapScreen(&NewWindowRect, 6.0f, CUI::RECTLIMITSCREEN_UP | CUI::RECTLIMITSCREEN_ALIGN_CENTER_X);
	if(m_Flags & FLAG_POSITION_CENTER)
	{
		NewWindowRect.x = (m_pUI->Screen()->w / 2.0f) - (m_ReserveRect.w / 2.0f);
		NewWindowRect.y = (m_pUI->Screen()->h / 2.0f) - (m_ReserveRect.h / 2.0f);
	}

	m_MainRect = NewWindowRect;
	m_Openned = true;
	m_Moving = false;
	m_Minimized = false;
	SetActiveWindow(this);
}

void CWindowUI::Close()
{
	if(m_pActiveWindow == this)
		m_pActiveWindow = nullptr;
	m_Openned = false;
	for(const auto &p : m_paChildrens)
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
	float Width = round_to_int(WorkspaceSize.x) == round_to_int(DEFAULT_WORKSPACE_SIZE) ? m_MainRect.w : WorkspaceSize.x;
	float Height = round_to_int(WorkspaceSize.y) == round_to_int(DEFAULT_WORKSPACE_SIZE) ? m_MainRect.h : WorkspaceSize.y;

	CUIRect NewWindowRect = {0, 0, Width, Height};
	if(round_to_int(NewWindowRect.w) != round_to_int(m_MainRect.w) || round_to_int(NewWindowRect.h) != round_to_int(m_MainRect.h))
	{
		m_pUI->RectLimitMapScreen(&NewWindowRect, 6.0f, CUI::RECTLIMITSCREEN_UP | CUI::RECTLIMITSCREEN_ALIGN_CENTER_X);
		m_MainRect.w = NewWindowRect.w;
		m_MainRect.h = NewWindowRect.h;
		m_ReserveRect.w = NewWindowRect.w;
		m_ReserveRect.h = NewWindowRect.h;
	}
}

void CWindowUI::SetActiveWindow(CWindowUI *pWindow)
{
	if(pWindow)
	{
		const auto pItem = std::find_if(ms_aWindows.begin(), ms_aWindows.end(), [pWindow](const CWindowUI* p)
		{
			return pWindow == p;
		});
		std::rotate(ms_aWindows.begin(), pItem, pItem + 1);
	}

	m_pActiveWindow = pWindow;
}

CWindowUI* CWindowUI::GetActiveWindow()
{
	return m_pActiveWindow;
}