/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_UI_WINDOW_H
#define GAME_CLIENT_UI_WINDOW_H

#include "ui_rect.h"

#include <functional>
#include <vector>

#define DEFAULT_WORKSPACE_SIZE (-1.f)
#define DEFAULT_BACKGROUND_WINDOW_SHANDOW vec4(0.4f, 0.4f, 0.4f, 0.95f)
#define DEFAULT_BACKGROUND_WINDOW_COLOR vec4(0.085f, 0.085f, 0.085f, 0.50f)
#define WINREGISTER(f, o)  std::bind(f, o, std::placeholders::_1, std::placeholders::_2)

class CWindowUI
{
	friend class CUI;
	friend class CWindowController;

	inline static std::vector<CWindowUI *> ms_aWindows;
	inline static class CUI *m_pUI;
	inline static class CRenderTools *m_pRenderTools;
	inline static CWindowUI *ms_pWindowHelper;

	using RenderWindowCallback = std::function<void(CUIRect, CWindowUI*)>;
	RenderWindowCallback m_pCallback{};
	RenderWindowCallback m_pCallbackHelp{};

	bool m_Openned{};
	vec4 m_BackgroundColor{};
	char m_aWindowName[128]{};
	CUIRect m_WindowRect{};
	CUIRect m_WindowBordure{};
	CUIRect m_WindowRectReserve{};
	CUIRect m_DefaultWindowRect{};
	bool *m_pRenderDependence{};
	std::vector<CWindowUI *> m_paChildrenWindows;

	int m_WindowFlags{};
	bool m_WindowMinimize{};
	bool m_WindowMoving{};

	CWindowUI() = default;
	CWindowUI(const char *pWindowName, vec2 WindowSize, bool *pRenderDependence = nullptr, int WindowFlags = WINDOWFLAG_ALL);

	void RenderWindowWithoutBordure();
	void RenderDefaultWindow();
	void Render();

public:
	enum
	{
		WINDOWFLAG_MINIMIZE = 1 << 0,
		WINDOWFLAG_CLOSE = 1 << 1,
		WINDOWFLAG_ALL = WINDOWFLAG_MINIMIZE | WINDOWFLAG_CLOSE,

		WINDOW_CLOSE_CLICKING_OUTSIDE = 1 << 2,
		WINDOW_WITHOUT_BORDURE = 1 << 3 | WINDOW_CLOSE_CLICKING_OUTSIDE,
	};

	CWindowUI(const CWindowUI& pWindow) = delete;

	/*
	 * Add child for window
	 */
	CWindowUI* AddChild(CWindowUI* pWindow)
	{
		if(!SearchWindowByKeyName(m_paChildrenWindows, pWindow->GetWindowName()))
			m_paChildrenWindows.emplace_back(pWindow);
		return pWindow;
	}

	CWindowUI *AddChild(const char *pChildName, vec2 WindowSize, int WindowFlags = WINDOWFLAG_ALL);

	/*
	 * Get child window
	 */
	CWindowUI *GetChild(const char *pChildName);

	/*
	 * Get fully child window name
	 */
	void GetFullChildWindowName(const char *pChildName, char *aBuf, int Size);

	/*
		Operator ==
	 */
	bool operator==(const CWindowUI &p) const;
	
	/*
		Static function: InitComponents -> void
			- Initializes components.
	*/
	static void InitComponents(class CUI *pUI, class CRenderTools *pRenderTools);

	/*
		Function: IsOpenned -> bool
			- Returns the window status.
	*/
	bool IsOpenned() const;

	/*
		Function: IsActive -> bool
			- Returns whether the window is active.
	*/
	bool IsActive() const;

	/*
		Function: GetRect -> const CUIRect&
			- Returns the window area.
	*/
	const CUIRect& GetRect() const;

	/*
		Function: GetWindowName -> const char
			- Returns the window name.
	*/
	const char* GetWindowName() const { return m_aWindowName; }

	/*
		Function: Open -> void
			- Opens a window.
		Remarks:
			- If window already openned, it will be reopened.
	*/
	void Open();

	/*
		Function: Close -> void
			- Closes the window.
		Remarks:
			- Including all windows dependent on this window.
	*/
	void Close();

	/*
		Function: Reverse -> void
			- If the window is openned it will be closed.
			- If the window is closed it will be open.
	*/
	void Reverse();

	/*
		Function: MinimizeWindow -> void
			- Minimize window or maximize recursive.
	*/
	void MinimizeWindow();

	/*
		Function: Init -> void
			- Initializes the window.
		Parameters:
			- WindowSize - Window size (Width Height).
			- pWindowDependent - Window will depend on transferred pointer.
			- pRenderDependence - If the pointer is true or nullptr the windows will render.
	*/
	void Init(vec2 WindowSize, bool* pRenderDependence = nullptr);

	/*
		Function: Register -> void
			- Registers the callback function for render.
		Parameters:
			- pCallback - Callback function.
			- pCallbackHelp - Callback helppage function.
		Remarks:
			- WINREGISTER(function ref, object) is used to register the callback function.
	*/
	void Register(RenderWindowCallback pCallback, RenderWindowCallback pCallbackHelp = nullptr);

	/*
		Function: SetDefaultWorkspace -> void
			- Set default workspace rect.
	*/
	void SetDefaultWorkspace()
	{
		m_WindowRect.w = m_DefaultWindowRect.w;
		m_WindowRect.h = m_DefaultWindowRect.h;
	}

	/*
		Function: SetBackgroundColor -> void
			- Set background color.
		Parameters:
			- Color - vector4 color.
	*/
	void SetBackgroundColor(vec4 Color)
	{
		if(Color.a > 0.0f)
			m_BackgroundColor = Color;
	}
	
	/*
		Function: MarginWorkspace -> void
			- Update workspace window.
	*/
	void MarginWorkspace(float Width, float Height)
	{
		SetWorkspace({ m_WindowRect.w - Width, m_WindowRect.h - Height });
		
	}

	/*
		Function: UpdateWorkspace -> void
			- Update workspace window.
		Parameters:
			- WorkspaceSize - Window size (Width Height).
	*/
	void SetWorkspace(float WorkspaceX = DEFAULT_WORKSPACE_SIZE, float WorkspaceY = DEFAULT_WORKSPACE_SIZE)
	{
		SetWorkspace({WorkspaceX, WorkspaceY});
	}

	void SetWorkspace(vec2 WorkspaceSize = vec2(DEFAULT_WORKSPACE_SIZE, DEFAULT_WORKSPACE_SIZE));

private:
	static void SetActiveWindow(CWindowUI* pWindow);
	static CWindowUI* GetActiveWindow();
	static CWindowUI *SearchWindowByKeyName(std::vector<CWindowUI *> &pVector, const char *pSearchKeyName);

	bool IsRenderAllowed() const { return m_Openned && m_pCallback && (m_pRenderDependence == nullptr || (m_pRenderDependence && *m_pRenderDependence == true)); }
	bool IsMoving() const { return m_WindowMoving; }

	// tools
	void DrawUIRect(CUIRect *pRect, ColorRGBA Color, int Corner, float Rounding) const;
	void DrawUIRectMonochrome(CUIRect *pRect, ColorRGBA Color, int Corner, float Rounding) const;
};


#endif
