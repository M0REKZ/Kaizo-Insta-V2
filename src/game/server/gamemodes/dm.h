/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_DM_H
#define GAME_SERVER_GAMEMODES_DM_H
#include "base_pvp.h"

class CGameControllerDM : public CGameControllerBasePvP
{
public:
	CGameControllerDM(class CGameContext *pGameServer);
	virtual void Tick();
};
#endif
