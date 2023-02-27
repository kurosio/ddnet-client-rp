/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_UI_WINDOW_H
#define GAME_CLIENT_UI_WINDOW_H

#include "ui_rect.h"

#include <functional>
#include <vector>

#define DEFAULT_WORKSPACE_SIZE (-1.f)
#define WINREGISTER(f, o)  std::bind(f, o, std::placeholders::_1, std::placeholders::_2)

class CWindowUI
{
	friend class CUI;
	friend class CWindowController;

	inline static std::vector<CWindowUI *> ms_aWindows;
	inline static class CUI *m_pUI;
	inline static class CRenderTools *m_pRenderTools;
	inline static CWindowUI *m_pActiveWindow;

	using RenderWindowCallback = std::function<void(CUIRect, CWindowUI*)>;
	RenderWindowCallback m_pCallback{};
	RenderWindowCallback m_pCallbackHelp{};

	bool m_Openned{};
	vec4 m_ColorTone{};
	char m_aName[128]{};
	CUIRect m_MainRect{};
	CUIRect m_BordureRect{};
	CUIRect m_ReserveRect{};
	CUIRect m_DefaultRect{};
	bool *m_pRenderDependence{};
	CWindowUI *m_Parent{};
	std::vector<CWindowUI *> m_paChildrens;

	int m_Flags{};
	bool m_Minimized{};
	bool m_Moving{};

	CWindowUI() = default;
	~CWindowUI() = default;
	CWindowUI(const char *pWindowName, vec2 WindowSize, bool *pRenderDependence = nullptr, int WindowFlags = FLAG_DEFAULT);

	void RenderWindowWithoutBordure();
	void RenderDefaultWindow();
	void Render();

public:
	enum
	{
		FLAG_POSITION_CENTER = 1 << 0, // default it's mouse position

		FLAG_MINIMIZE = 1 << 1,
		FLAG_CLOSE = 1 << 2,
		FLAG_DEFAULT = FLAG_MINIMIZE | FLAG_CLOSE,
		FLAG_DEFAULT_CENTER = FLAG_DEFAULT | FLAG_POSITION_CENTER,

		FLAG_CLOSE_CLICKING_OUTSIDE = 1 << 3,
		FLAG_WITHOUT_BORDURE = 1 << 4 | FLAG_CLOSE_CLICKING_OUTSIDE,
	};

	CWindowUI(const CWindowUI& pWindow) = delete;

	/*
	 * Add child for window
	 */
	CWindowUI* AddChild(CWindowUI* pWindow)
	{
		if(!SearchWindowByKeyName(m_paChildrens, pWindow->GetWindowName()))
			m_paChildrens.emplace_back(pWindow);
		return pWindow;
	}

	CWindowUI *AddChild(const char *pName, vec2 WindowSize, int WindowFlags = FLAG_DEFAULT);

	/*
	 * Get child window
	 */
	CWindowUI *GetChild(const char *pName);

	/*
	 * Get fully child window name
	 */
	void GetFullChildWindowName(const char *pName, char *aBuf, int Size);

	/*
	 * Get parent
	 */
	CWindowUI *GetParent() const { return m_Parent; }

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
	const char* GetWindowName() const { return m_aName; }

	/*
		Function: Open -> void
			- Opens a window.
		Parameters:
			- X, Y (vec2 Position) - Window position if not set it's will be openned near mouse.
		Remarks:
			- If window already openned, it will be reopened.
	*/
	void Open(float X = 0.f, float Y = 0.f);
	void Open(vec2 Position) { Open(Position.x, Position.y); }

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
		m_MainRect.w = m_DefaultRect.w;
		m_MainRect.h = m_DefaultRect.h;
	}

	/*
		Function: SetColorTone -> void
			- Set tone color.
		Parameters:
			- Color - vector4 color.
	*/
	void SetColorTone(vec4 Color)
	{
		if(Color.a > 0.0f)
			m_ColorTone = Color;
	}
	
	/*
		Function: MarginWorkspace -> void
			- Update workspace window.
	*/
	void MarginWorkspace(float Width, float Height)
	{
		SetWorkspace({ m_MainRect.w - Width, m_MainRect.h - Height });
		
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
	bool IsMoving() const { return m_Moving; }
};


#endif
