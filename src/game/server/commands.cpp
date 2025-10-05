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
#include <game/version.h>
#include <game/layers.h>

void CGameContext::ConInfo(IConsole::IResult *pResult, void *pUserData)
{
    CGameContext *pSelf = (CGameContext *)pUserData;
    char aBuf[128];
    
    str_format(aBuf, sizeof(aBuf), "Server's Infrastructure dates to %s", pSelf->BuildDate());
    pSelf->SendChatTarget(pResult->GetClientID(), aBuf);
    pSelf->SendChatTarget(pResult->GetClientID(), "Developed by veqi, +KZ");
    
    // Get map info with safety checks
    CMapItemInfo *pMapInfo = pSelf->m_Layers.GetMapInfo();
    if(!pMapInfo)
        return;

    if(!pSelf->m_Layers.Map())
        return;
    
    if(pMapInfo->m_Author >= 0)
    {
        int DataSize = 0;
        void *pData = pSelf->m_Layers.Map()->GetData(pMapInfo->m_Author);
        const char *pAuthor = (const char *)pData;
        
        if(pAuthor && pAuthor[0] != '\0')
        {
            str_format(aBuf, sizeof(aBuf), "Map Author: %s", pAuthor);
            pSelf->SendChatTarget(pResult->GetClientID(), aBuf);
        }
    }
    if(pMapInfo->m_License >= 0)
    {
        void *pData = pSelf->m_Layers.Map()->GetData(pMapInfo->m_License);
        const char *pLicense = (const char *)pData;
        
        if(pLicense && pLicense[0] != '\0')
        {
            str_format(aBuf, sizeof(aBuf), "Map License: %s", pLicense);
            pSelf->SendChatTarget(pResult->GetClientID(), aBuf);
        }
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

	char Username[512];
    char Password[512];
    str_copy(Username, pResult->GetString(0), sizeof(Username));
    str_copy(Password, pResult->GetString(1), sizeof(Password));

    pSelf->Sql()->Login(Username, Password, pResult->GetClientID());
}

void CGameContext::ConLogout(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

    pSelf->Sql()->Logout(pResult->GetClientID());
}


void CGameContext::ConCreateClan(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	char Username[512];
    str_copy(Username, pResult->GetString(0), sizeof(Username));

    pSelf->Sql()->CreateClan(Username, pResult->GetClientID());
}

void CGameContext::ConCreateTables(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

    pSelf->Sql()->CreateTables();
}