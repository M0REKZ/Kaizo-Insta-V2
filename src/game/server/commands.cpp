#include "gamecontext.h"
#include <engine/console.h>

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
