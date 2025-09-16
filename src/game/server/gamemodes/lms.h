/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_LMS_H
#define GAME_SERVER_GAMEMODES_LMS_H
#include "base_pvp.h"

class CGameControllerLMS : public CGameControllerBasePvP
{
public:
	CGameControllerLMS(class CGameContext *pGameServer);

	// event
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	// game
	virtual void DoWincheckRound();
};

#endif
