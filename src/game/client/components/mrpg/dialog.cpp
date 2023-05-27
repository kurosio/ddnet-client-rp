/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/color.h>
#include <game/client/gameclient.h>

#include "dialog.h"

// .........................
#include <engine/graphics.h>
#include <engine/keys.h>

#include <game/client/animstate.h>
#include <game/generated/protocol.h>

#include "../console.h"
#include "../sounds.h"


bool CDialogRPG::IsActive() const
{
	return m_Dialog.IsActive();
}

void CDialogRPG::OnInit()
{
	m_ScreenWidth = 400 * 3.0f * Graphics()->ScreenAspect();
	m_ScreenHeight = 400 * 3.0f;

	// load dialog font
	TextRender()->LoadFont(&m_pFont, "fonts/EuroStyle.ttf");
}

void CDialogRPG::OnStateChange(int NewState, int OldState)
{
	if(OldState == IClient::STATE_ONLINE || OldState == IClient::STATE_OFFLINE)
		m_Dialog.Clear();
}

void CDialogRPG::OnRender()
{
	if(!IsActive())
		return;

	// set default workspace element
	CUIRect WorkspaceElement{};
	{
		constexpr float h = 180.0f;
		constexpr float w = 1000.0f;
		const float x = m_ScreenWidth / 2 - w / 2;
		const float y = m_ScreenHeight / 2 + h / 2;
		WorkspaceElement = {x, y, w, h};
		Graphics()->MapScreen(0, 0, m_ScreenWidth, m_ScreenHeight);
	}

	// background
	CUIRect BackgroundText{}, BackgroundMain{};
	WorkspaceElement.Margin(0.f, &BackgroundMain);
	BackgroundMain.Margin(4.0f, &BackgroundText);
	BackgroundMain.DrawMonochrome(ColorRGBA(0.29f, 0.25f, 0.21f, 0.85f), IGraphics::CORNER_ALL, 76.0f);
	BackgroundText.DrawMonochrome(ColorRGBA(0.94f, 0.87f, 0.80f, 0.85f), IGraphics::CORNER_ALL, 76.0f);

	// get name and render info by flags
	const char* pLeftNickname = "\0";
	const char* pRightNickname = "\0";
	CTeeRenderInfo LeftRenderTee({});
	CTeeRenderInfo RightRenderTee({});

	const int Flags = m_Dialog.GetFlags();
	if(Flags & TALKED_FLAG_LBOT || Flags & TALKED_FLAG_LPLAYER)
	{
		int ClientID = m_Dialog.m_LeftClientID;
		if(ClientID >= 0 && ClientID < MAX_CLIENTS && m_pClient->m_aClients[ClientID].m_Active)
		{
			pLeftNickname = m_pClient->m_aClients[ClientID].m_aName;
			LeftRenderTee = m_pClient->m_aClients[ClientID].m_RenderInfo;
			LeftRenderTee.m_Size = 116.f;
		}
	}

	if(Flags & TALKED_FLAG_RBOT)
	{
		int ClientID = m_Dialog.m_RightClientID;
		if(ClientID >= 0 && ClientID < MAX_CLIENTS && m_pClient->m_aClients[ClientID].m_Active)
		{
			pRightNickname = m_pClient->m_aClients[ClientID].m_aName;
			RightRenderTee = m_pClient->m_aClients[ClientID].m_RenderInfo;
			RightRenderTee.m_Size = 116.f;
		}
	}

	// set dialog font
	TextRender()->SetCurFont(m_pFont);

	// render tee
	CUIRect WorkspaceOutside{}, LabelNickname{}, LeftRect{}, RightRect{};
	WorkspaceElement.VMargin(64.0f, &WorkspaceOutside);
	WorkspaceOutside.HSplitTop(24.0f, &WorkspaceOutside, 0);
	WorkspaceOutside.VSplitMid(&LeftRect, &RightRect, 0.f);
	{
		ColorRGBA BackgroundNickname(0.1f, 0.1f, 0.1f, 0.2f);
		if((Flags & TALKED_FLAG_REMPTY) == 0 && (Flags & TALKED_FLAG_LEMPTY || Flags & TALKED_FLAG_SPEAK_RIGHT))
			WorkspaceOutside.Draw4({}, BackgroundNickname, {}, BackgroundNickname, IGraphics::CORNER_ALL, 12.0f);
		else if((Flags & TALKED_FLAG_SPEAK_WORLD) == 0 || (Flags & TALKED_FLAG_SPEAK_LEFT && (Flags & TALKED_FLAG_LEMPTY) == 0))
			WorkspaceOutside.Draw4(BackgroundNickname, {}, BackgroundNickname, {}, IGraphics::CORNER_ALL, 12.0f);

		constexpr float Space = 48.f;
		if(((Flags & TALKED_FLAG_LEMPTY) == 0) && LeftRenderTee.m_Size > 8.0f)
		{
			if(Flags & TALKED_FLAG_SPEAK_LEFT)
			{
				Graphics()->TextureSet(GameClient()->m_EmoticonsSkin.m_aSpriteEmoticons[EMOTICON_DOTDOT]);
				Graphics()->QuadsSetSubset(0, 0, 1, 1);
				Graphics()->QuadsBegin();
				constexpr float Size = 116.0f;
				IGraphics::CQuadItem QuadItem(LeftRect.x, LeftRect.y - (Space * 2.0f), Size, Size);
				Graphics()->QuadsDraw(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			RenderTools()->RenderTee(CAnimState::GetIdle(), &LeftRenderTee, EMOTE_NORMAL, vec2(1.0f, 0.33f), vec2(LeftRect.x, LeftRect.y));

			LeftRect.VSplitLeft(48.f, 0, &LabelNickname);
			UI()->DoLabel(&LabelNickname, pLeftNickname, 36.0f, TEXTALIGN_LEFT);
		}
		if(((Flags & TALKED_FLAG_REMPTY) == 0) && RightRenderTee.m_Size > 8.0f)
		{
			if(Flags & TALKED_FLAG_SPEAK_RIGHT)
			{
				Graphics()->TextureSet(GameClient()->m_EmoticonsSkin.m_aSpriteEmoticons[EMOTICON_DOTDOT]);
				Graphics()->QuadsSetSubset(0, 0, 1, 1);
				Graphics()->QuadsBegin();
				constexpr float Size = 116.0f;
				IGraphics::CQuadItem QuadItem((RightRect.x + RightRect.w), RightRect.y - (Space * 2.0f), Size, Size);
				Graphics()->QuadsDraw(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			RenderTools()->RenderTee(CAnimState::GetIdle(), &RightRenderTee, EMOTE_NORMAL, vec2(-1.0f, 0.33f), vec2((RightRect.x + RightRect.w), RightRect.y));
			
			RightRect.VSplitRight(48.f, &LabelNickname, 0);
			UI()->DoLabel(&LabelNickname, pRightNickname, 36.0f, TEXTALIGN_RIGHT);
		}
	}

	// some
	if(Flags & TALKED_FLAG_SPEAK_WORLD)
	{
	}

	// dialog text
	CUIRect LabelText{};
	constexpr float FontSize = 26.0f;
	BackgroundText.Margin(32.f, &LabelText);

	CTextCursor Cursor{};
	int TextContainer = -1;
	TextRender()->SetCursor(&Cursor, LabelText.x, LabelText.y, FontSize, TEXTFLAG_RENDER);
	Cursor.m_LineWidth = LabelText.w;
	Cursor.m_CharCount = 5;
	Cursor.m_MaxLines = static_cast<int>(ceil(LabelText.h / FontSize));
	TextRender()->CreateTextContainer(TextContainer, &Cursor, m_Dialog.m_aTextMsg);
	{
		ColorRGBA Color(0.f, 0.f, 0.f, 0.7f);
		ColorRGBA OutColor(0.f, 0.f, 0.f, 0.05f);
		TextRender()->RenderTextContainer(TextContainer, Color, OutColor);
	}

	// clear dialog font
	TextRender()->SetCurFont(nullptr);
}
void CDialogRPG::OnMessage(int MsgType, void *pRawMsg)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;

	if(MsgType == NETMSGTYPE_SV_DIALOG)
	{
		const auto pMsg = static_cast<CNetMsg_Sv_Dialog *>(pRawMsg);
		m_Dialog.Init(pMsg->m_LeftClientID, pMsg->m_RightClientID, pMsg->m_pText, pMsg->m_Flag);
	}
	else if(MsgType == NETMSGTYPE_SV_CLEARDIALOG)
	{
		m_Dialog.Clear();
	}
}

bool CDialogRPG::OnInput(IInput::CEvent Event)
{
	if(!m_pClient->m_GameConsole.IsClosed() || !IsActive())
		return false;

	if(IsActive() && Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_TAB)
	{
		CNetMsg_Cl_DialogNext Msg;
		Client()->SendPackMsgActive(&Msg, MSGFLAG_VITAL);
		m_pClient->m_Sounds.Play(CSounds::CHN_WORLD, 1 /*todo: add sound*/, 1.0f);
		return true;
	}

	return false;
}
