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
	m_CurrentRect.Margin(s_BackgroundMargin, &Workspace);

	// background draw
	CUIRect Background{};
	Workspace.Margin(-s_BackgroundMargin, &Background);
	Background.DrawMonochrome(m_ColorTone / 1.5f, IGraphics::CORNER_ALL, s_Rounding);

	const float BackgroundFade = m_pUI->GetFade(&Workspace, IsActive(), 0.4f);
	const vec4 Color = mix(m_ColorTone, m_ColorTone + vec4(0.02f, 0.02f, 0.02f, 0.f), BackgroundFade);
	Workspace.Draw(ColorRGBA(Color), IGraphics::CORNER_ALL, s_Rounding);

	// render callback
	if(m_pRenderCallback)
	{
		m_pRenderCallback(Workspace, this);
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
		m_CurrentRect.x = m_BordureRect.x = clamp(m_pUI->MouseX() - s_WindowSkipMovingX, 0.0f, m_pUI->Screen()->w - m_CurrentRect.w);
		m_CurrentRect.y = m_BordureRect.y = clamp(m_pUI->MouseY() - s_WindowSkipMovingY, 0.0f, m_pUI->Screen()->h - m_CurrentRect.h);
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
	m_CurrentRect.HSplitTop(s_BordureWidth, &m_BordureRect, &Workspace);

	/*
	 * Background draw
	 */
	const bool IsActiveWindow = IsActive();
	{
		CUIRect ShadowBackground{};
		m_CurrentRect.Margin(-1.5f, &ShadowBackground);
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
	if(!m_Minimized && m_pRenderCallback)
	{
		Workspace.Margin(s_BackgroundMargin, &Workspace);
		m_pRenderCallback(Workspace, this);
	}
		
	/*
	 * Bordure top
	 */
	bool DissalowWindowMoving = false;
	{
		enum class ButtonStateEventL { DEFAULT, CLICKED, HOVERED };

		// Lambda Bordure buttons function
		auto CreateButtonTop = [&](int *pCounter, CUIRect *pButtonRect, const char *pHintStr, vec4 ColorFade1, vec4 ColorFade2, const char *pSymbolUTF) -> ButtonStateEventL
		{
			CUIRect Button = *(pButtonRect);
			const vec4 ColorFinal = mix(ColorFade1, ColorFade2, m_pUI->GetFade(&Button, false));
			Button.DrawMonochrome(ColorRGBA(ColorFinal), IGraphics::CORNER_ALL, s_Rounding);
			m_pUI->DoLabel(&Button, pSymbolUTF, s_FontSize, TEXTALIGN_CENTER);
			pButtonRect->x -= 24.f;
			if(pCounter)
				*pCounter += 1;

			auto StateEvent = ButtonStateEventL::DEFAULT;
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

				if(m_pUI->Input()->KeyPress(KEY_MOUSE_1))
					StateEvent = ButtonStateEventL::CLICKED;
				else
					StateEvent = ButtonStateEventL::HOVERED;
			}

			if(StateEvent == ButtonStateEventL::CLICKED)
				DissalowWindowMoving = true;

			return StateEvent;
		};

		// Bordure top
		int ButtonsNum = 0;
		CUIRect ButtonTop{};
		m_BordureRect.VSplitRight(24.0f, nullptr, &ButtonTop);

		// close button
		if(m_Flags & FLAG_CLOSE)
		{
			ButtonStateEventL Event = CreateButtonTop(&ButtonsNum, &ButtonTop, "Left Ctrl + Q - close.", vec4(0.f, 0.f, 0.f, 0.25f), vec4(0.7f, 0.1f, 0.1f, 0.75f), "\xE2\x9C\x95");
			if(Event == ButtonStateEventL::CLICKED)
				Close();
		}

		// minimize button
		if(m_Flags & FLAG_MINIMIZE)
		{
			ButtonStateEventL Event = CreateButtonTop(&ButtonsNum, &ButtonTop, "Left Ctrl + M - minimize.", vec4(0.f, 0.f, 0.f, 0.25f), vec4(0.2f, 0.2f, 0.7f, 0.75f), m_Minimized ? "\xe2\x81\x82" : "\xe2\x80\xbb");
			if(Event == ButtonStateEventL::CLICKED)
				MinimizeWindow();
		}

		// Window name
		CUIRect Label{};
		m_BordureRect.VSplitRight(ButtonTop.w * static_cast<float>(ButtonsNum), &Label, 0);
		Label.VMargin(3.0f, &Label);
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
		s_WindowSkipMovingX = (m_pUI->MouseX() - m_CurrentRect.x);
		s_WindowSkipMovingY = (m_pUI->MouseY() - m_CurrentRect.y);
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
		if(m_Flags & FLAG_CLOSE_CLICKING_OUTSIDE && !m_pUI->MouseHovered(&m_CurrentRect) && m_pUI->Input()->KeyPress(KEY_MOUSE_1))
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
		m_LastRectAfterChange = m_CurrentRect;
		m_CurrentRect = m_BordureRect;
		return;
	}

	m_LastRectAfterChange.x = clamp(m_CurrentRect.x, 0.0f, m_pUI->Screen()->w - m_LastRectAfterChange.w);
	m_LastRectAfterChange.y = clamp(m_CurrentRect.y, 0.0f, m_pUI->Screen()->h - m_LastRectAfterChange.h);
	m_CurrentRect = m_LastRectAfterChange;
}

// - - -- - -- - -- - -- - -- - -- - -
// Functions for working with windows
void CWindowUI::Init(vec2 WindowSize, bool* pRenderDependence)
{
	m_BordureRect = { 0, 0, 0, 0 };
	m_CurrentRect = { 0, 0, WindowSize.x, WindowSize.y };
	m_DefaultRect = m_CurrentRect;
	m_LastRectAfterChange = m_CurrentRect;
	m_Minimized = false;
	m_Moving = false;
	if(pRenderDependence)
		m_pRenderDependence = pRenderDependence;

	SetColorTone(ColorHSLA(g_Config.m_UiDefaultWindowColor, true));
}

const CUIRect& CWindowUI::GetRect() const
{
	return m_CurrentRect;
}

CWindowUI::CWindowUI(const char *pWindowName, vec2 WindowSize, bool *pRenderDependence, int WindowFlags)
{
	m_Parent = nullptr;
	m_Open = false;
	m_Flags = WindowFlags;
	str_copy(m_aName, pWindowName, sizeof(m_aName));
	Init(WindowSize, pRenderDependence);
}

bool CWindowUI::IsOpenned() const
{
	return m_Open;
}

bool CWindowUI::IsActive() const
{
	return (bool)(m_Open && GetActiveWindow() == this);
}

void CWindowUI::Open(float PosX, float PosY)
{
	for(const auto &p : m_paChildrens)
		p->Close();

	CUIRect NewWindowRect = {PosX <= 0.f ? m_pUI->MouseX() : PosX, PosY <= 0.f ? m_pUI->MouseY() : PosY, m_LastRectAfterChange.w, m_LastRectAfterChange.h };
	m_pUI->RectLimitMapScreen(&NewWindowRect, 6.0f, CUI::RECTLIMITSCREEN_UP | CUI::RECTLIMITSCREEN_ALIGN_CENTER_X);
	if(m_Flags & FLAG_POSITION_CENTER)
	{
		NewWindowRect.x = (m_pUI->Screen()->w / 2.0f) - (m_LastRectAfterChange.w / 2.0f);
		NewWindowRect.y = (m_pUI->Screen()->h / 2.0f) - (m_LastRectAfterChange.h / 2.0f);
	}

	m_CurrentRect = NewWindowRect;
	m_Open = true;
	m_Moving = false;
	m_Minimized = false;
	SetActiveWindow(this);
}

void CWindowUI::Close()
{
	if(m_pActiveWindow == this)
		m_pActiveWindow = nullptr;
	m_Open = false;
	for(const auto &p : m_paChildrens)
		p->Close();
}

void CWindowUI::Reverse()
{
	if(m_Open)
		Close();
	else
		Open();
}

void CWindowUI::Register(RenderWindowCallback pCallback)
{
	m_pRenderCallback = std::move(pCallback);
}

void CWindowUI::SetWorkspace(vec2 WorkspaceSize)
{
	float Width = round_to_int(WorkspaceSize.x) == round_to_int(DEFAULT_WORKSPACE_SIZE) ? m_CurrentRect.w : WorkspaceSize.x;
	float Height = round_to_int(WorkspaceSize.y) == round_to_int(DEFAULT_WORKSPACE_SIZE) ? m_CurrentRect.h : WorkspaceSize.y;

	CUIRect NewWindowRect = {0, 0, Width, Height};
	if(round_to_int(NewWindowRect.w) != round_to_int(m_CurrentRect.w) || round_to_int(NewWindowRect.h) != round_to_int(m_CurrentRect.h))
	{
		m_pUI->RectLimitMapScreen(&NewWindowRect, 6.0f, CUI::RECTLIMITSCREEN_UP | CUI::RECTLIMITSCREEN_ALIGN_CENTER_X);
		m_CurrentRect.w = NewWindowRect.w;
		m_CurrentRect.h = NewWindowRect.h;
		m_LastRectAfterChange.w = NewWindowRect.w;
		m_LastRectAfterChange.h = NewWindowRect.h;
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