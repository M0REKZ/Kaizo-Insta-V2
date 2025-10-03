/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_BLOCK_H
#define GAME_SERVER_GAMEMODES_BLOCK_H
#include <game/server/gamecontroller.h>

class CGameControllerBLOCK : public IGameController
{
public:
	CGameControllerBLOCK(class CGameContext *pGameServer);

    virtual void OnCharacterSpawn(class CCharacter *pChr);
    virtual bool OnCharacterTakeDamage(class CCharacter *pChar, vec2 Force, int Dmg, int From, int Weapon);

    virtual bool OnEntity(int Index, vec2 Pos);
};
#endif