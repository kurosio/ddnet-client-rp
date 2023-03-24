/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MRPG_DIALOG_H
#define GAME_CLIENT_COMPONENTS_MRPG_DIALOG_H

#include <game/client/component.h>

class CDialogRPG : public CComponent
{
	class CDialogData
	{
		int m_Flags{};

	public:
		int m_TextContainer{-1};
		int m_LeftClientID{};
		int m_RightClientID{};
		char m_aTextMsg[2048]{};

		bool IsActive() const { return m_aTextMsg[0] != '\0'; }
		int GetFlags() const { return m_Flags; }

		CDialogData()
		{
			Clear();
		}

		void Init(int LeftClientID, int RightClientID, const char *pText, int Flag)
		{
			m_LeftClientID = LeftClientID;
			m_RightClientID = RightClientID;
			m_Flags = Flag;
			str_copy(m_aTextMsg, pText, sizeof(m_aTextMsg));
		}

		void Clear()
		{
			m_Flags = -1;
			m_LeftClientID = -1;
			m_aTextMsg[0] = '\0';
		}

	};

	CFont *m_pFont{};
	CDialogData m_Dialog{};
	float m_ScreenWidth{};
	float m_ScreenHeight{};

public:
	int Sizeof() const override { return sizeof(*this); }

	bool IsActive() const;

	void OnInit() override;
	void OnStateChange(int NewState, int OldState) override;
	void OnRender() override;
	void OnMessage(int MsgType, void *pRawMsg) override;
	bool OnInput(IInput::CEvent Event) override;
};

#endif
