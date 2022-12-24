#ifndef GAME_CLIENT_COMPONENTS_WINDOWS_H
#define GAME_CLIENT_COMPONENTS_WINDOWS_H

#include <game/client/component.h>

/*
	Windows GUI elements are placed between CMenus and CWindows
	Because Teeworlds does not support a layer-by-layer UI checker.
*/

class CWindowController : public CComponent
{
public:
	~CWindowController() override;

	int Sizeof() const override { return sizeof(*this); }
	void OnRender() override;
	bool OnInput(IInput::CEvent Event) override;

private:
	void Update(bool *pCursor) const;
};

#endif