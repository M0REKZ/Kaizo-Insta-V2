#include <engine/shared/config.h>

#include <game/mapitems.h>

#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "block.h"

CGameControllerBLOCK::CGameControllerBLOCK(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "Block        BW";
	m_GameFlags = 0;
}

void CGameControllerBLOCK::OnCharacterSpawn(CCharacter *pChr)
{
    // default health
    pChr->IncreaseHealth(10);
    pChr->IncreaseArmor(10);

    // give default weapons
    pChr->GiveWeapon(WEAPON_HAMMER, -1);
    pChr->GiveWeapon(WEAPON_GUN, 10);
}

bool CGameControllerBLOCK::OnCharacterTakeDamage(CCharacter *pChar, vec2 Force, int Dmg, int From, int Weapon)
{
    pChar->Core().m_Vel += Force;

    if(Weapon == WEAPON_HAMMER && pChar->IsFrozen())
        pChar->Freeze(0);
    return true;
}

bool CGameControllerBLOCK::OnEntity(int Index, vec2 Pos)
{
    if(Index == ENTITY_WEAPON_SHOTGUN)
        return true; // Create custom ddnet weapon (when KZ adds custom weapons)

    return IGameController::OnEntity(Index,Pos);
}