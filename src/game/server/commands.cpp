#include <new>
#include <base/math.h>
#include <engine/shared/config.h>
#include <engine/map.h>
#include <engine/console.h>
#include "gamecontext.h"
#include <game/version.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include "gamemodes/dm.h"
#include "gamemodes/tdm.h"
#include "gamemodes/ctf.h"
#include "gamemodes/mod.h"
#include "gamemodes/lms.h"
#include "gamemodes/lts.h"


void CGameContext::ConInfo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

    int ClientID = pResult->GetClientID();
    if(ClientID < 0 || ClientID > MAX_CLIENTS)
        return;

    char aBuf[128];
    str_format(aBuf, sizeof(aBuf), "Kaizo Insta V2 developed by +KZ and veqi");
	pSelf->SendChatTarget(ClientID, aBuf);
}

void CGameContext::ConRollback(IConsole::IResult *pResult, void *pUser)
{
	CGameContext *pSelf = (CGameContext *)pUser;
	int ClientId = pSelf->m_ChatResponseTargetID;

	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return;

	if(!pSelf->m_pController)
		return;

	if(!g_Config.m_SvRollback)
	{
		pSelf->SendChatTarget(ClientId, "Rollback is not allowed on this server.");
		return;
	}

	if(!pSelf->m_apPlayers[ClientId])
		return;

	if(!pSelf->m_apPlayers[ClientId]->m_RollbackEnabled)
	{
		pSelf->m_apPlayers[ClientId]->m_RollbackEnabled = true;
		pSelf->SendChatTarget(ClientId, "Rollback enabled.");

		if(pSelf->Server()->GetClientVersion(ClientId) >= VERSION_DDNET_ANTIPING_PROJECTILE)
		{
			pSelf->SendChatTarget(ClientId, "DDNet Client detected, for correct rollback experience please set the following Antiping settings:");
			pSelf->SendChatTarget(ClientId, "* Antiping: ON");
			pSelf->SendChatTarget(ClientId, "* Antiping: predict other players: OFF");
			pSelf->SendChatTarget(ClientId, "* Antiping: predict weapons: ON");
			pSelf->SendChatTarget(ClientId, "* Antiping: predict grenade paths: ON");
		}
	}
	else
	{
		pSelf->m_apPlayers[ClientId]->m_RollbackEnabled = false;
		pSelf->SendChatTarget(ClientId, "Rollback disabled.");
	}
}
