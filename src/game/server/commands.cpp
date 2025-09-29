#include <new>
#include <base/math.h>
#include <engine/shared/config.h>
#include <engine/map.h>
#include <engine/console.h>
#include <base/crypt.h>
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
    char aBuf[128];
    str_format(aBuf, sizeof(aBuf), "Kaizo Insta V2 developed by +KZ and veqi");
	pSelf->SendChatTarget(pResult->GetClientID(), aBuf);
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

void CGameContext::ConSpec(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

    int ClientID = pResult->GetClientID();
    if(ClientID < 0 || ClientID > MAX_CLIENTS)
        return;

	if(pSelf->m_apPlayers[ClientID]->GetTeam() == TEAM_RED)
		pSelf->m_apPlayers[ClientID]->SetTeam(TEAM_SPECTATORS, false, false);
	else 
		pSelf->m_apPlayers[ClientID]->SetTeam(TEAM_RED, false, false);
}

#ifdef CONF_SQL

void CGameContext::ConRegister(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
    int ClientID = pResult->GetClientID();

	char Username[512];
    char Password[512];
    str_copy(Username, pResult->GetString(0), sizeof(Username));
    str_copy(Password, pResult->GetString(1), sizeof(Password));
    // char aHash[64]; //Result
	// mem_zero(aHash, sizeof(aHash));
	// Crypt(Password, (const unsigned char*) "d9", 1, 14, aHash);

    pSelf->Sql()->CreateAccount(Username, Password, pResult->GetClientID());
}

void CGameContext::ConLogin(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

    int ClientID = pResult->GetClientID();

	char Username[512];
    char Password[512];
    str_copy(Username, pResult->GetString(0), sizeof(Username));
    str_copy(Password, pResult->GetString(1), sizeof(Password));

    pSelf->Sql()->Login(Username, Password, pResult->GetClientID());
}

void CGameContext::ConCreateClan(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

    int ClientID = pResult->GetClientID();

	char Username[512];
    str_copy(Username, pResult->GetString(0), sizeof(Username));

    pSelf->Sql()->CreateClan(Username, pResult->GetClientID());
}

void CGameContext::ConCreateTables(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

    pSelf->Sql()->CreateTables();
}



#endif