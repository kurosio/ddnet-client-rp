#include <engine/keys.h>

#include "windows.h"

#include <game/client/render.h>
#include <game/client/ui.h>


/*
	Windows GUI elements are placed between CMenus and CWindows
	Because Teeworlds does not support a layer-by-layer UI checker.
*/

CWindowController::~CWindowController()
{
	for(auto *pWindow : CWindowUI::ms_aWindows)
		delete pWindow;

	CWindowUI::ms_aWindows.clear();
}

void CWindowController::OnRender()
{
	// update
	bool ShowCursor = false;
	Update(&ShowCursor);

	// hotkeys
	if(CWindowUI *pWindowActive = CWindowUI::GetActiveWindow())
	{
		if((pWindowActive->m_WindowFlags & CWindowUI::WINDOWFLAG_CLOSE) && Input()->KeyIsPressed(KEY_LCTRL) && Input()->KeyPress(KEY_Q))
			pWindowActive->Close();
		if((pWindowActive->m_WindowFlags & CWindowUI::WINDOWFLAG_MINIMIZE) && Input()->KeyIsPressed(KEY_LCTRL) && Input()->KeyPress(KEY_M))
			pWindowActive->MinimizeWindow();
	}

	// render cursor
	if(ShowCursor)
		RenderTools()->RenderCursor(vec2(UI()->MouseX(), UI()->MouseY()), 24.f);

	// finish all checks button logics
	UI()->FinishCheck();
}

void CWindowController::Update(bool* pCursor) const
{
	// update hovered the active highlighted area
	if(auto pHovered = std::find_if(CWindowUI::ms_aWindows.begin(), CWindowUI::ms_aWindows.end(), [this](CWindowUI *p) 
		{ return p->IsRenderAllowed() && (UI()->MouseInside(&p->m_WindowRect) || p->m_WindowMoving); }); pHovered != CWindowUI::ms_aWindows.end())
		UI()->SetHoveredWindow(*pHovered);

	// draw in reverse order as they are sorted here
	auto [ScreenX, ScreenY, ScreenW, ScreenH] = *UI()->Screen();
	Graphics()->MapScreen(ScreenX, ScreenY, ScreenW, ScreenH);
	for(auto it = CWindowUI::ms_aWindows.rbegin(); it != CWindowUI::ms_aWindows.rend(); ++it)
	{
		if((*it)->IsRenderAllowed())
		{
			// start check only this window
			UI()->StartCheckWindow(*it);

			if(CWindowUI::GetActiveWindow() != (*it) && Input()->KeyPress(KEY_MOUSE_1) && UI()->MouseHovered(&(*it)->m_WindowRect))
				CWindowUI::SetActiveWindow((*it));

			// end check only this window
			UI()->FinishCheckWindow();

			(*it)->Render();
			*pCursor = true;
		}
	}

	// clear hovered active highlighted area
	UI()->SetHoveredWindow(nullptr);
}