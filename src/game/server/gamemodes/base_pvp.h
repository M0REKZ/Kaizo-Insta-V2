/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_BASE_PVP_H
#define GAME_SERVER_GAMEMODES_BASE_PVP_H
#include <game/server/gamecontroller.h>

class CGameControllerBasePvP : public IGameController
{
public:
	CGameControllerBasePvP(class CGameContext *pGameServer);

    virtual void OnCharacterSpawn(class CCharacter *pChr);
    virtual bool OnCharacterTakeDamage(class CCharacter *pChar, vec2 Force, int Dmg, int From, int Weapon);

    virtual bool OnEntity(int Index, vec2 Pos);

    int m_InstagibWeapon = -1;
};
#endif