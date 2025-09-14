#include <engine/shared/config.h>

#include <game/mapitems.h>

#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "base_pvp.h"

CGameControllerBasePvP::CGameControllerBasePvP(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "BasePvP!";
	m_GameFlags = 0;

	if(g_Config.m_SvInstagibWeapon[0] == 'l' || g_Config.m_SvInstagibWeapon[0] == 'L')
		m_InstagibWeapon = WEAPON_LASER;
	else if(g_Config.m_SvInstagibWeapon[0] == 'g' || g_Config.m_SvInstagibWeapon[0] == 'G')
		m_InstagibWeapon = WEAPON_GRENADE;
}

void CGameControllerBasePvP::OnCharacterSpawn(CCharacter *pChr)
{
    // default health
    pChr->IncreaseHealth(10);

    if(m_InstagibWeapon == -1)
    {
        // give default weapons
        pChr->GiveWeapon(WEAPON_HAMMER, -1);
        pChr->GiveWeapon(WEAPON_GUN, 10);
    }
    else
    {
        // give instagib weapon
        pChr->GiveWeapon(m_InstagibWeapon, -1);
        pChr->SetWeapon(m_InstagibWeapon);
    }
}

bool CGameControllerBasePvP::OnCharacterTakeDamage(CCharacter *pChar, vec2 Force, int Dmg, int From, int Weapon)
{
    if(m_InstagibWeapon == -1)
    {
        return false;
    }
    else
    {
        while (true)
        {
            // check for death

            if (pChar->GetPlayer()->GetCID() == From)
                break;

            if (Dmg < g_Config.m_SvInstagibMinDamage)
                break;

            pChar->Die(From, Weapon);

            if(From >= 0 && From != pChar->GetPlayer()->GetCID() && GameServer()->m_apPlayers[From])
            {
                int Mask = CmaskOne(From);
                for(int i = 0; i < MAX_CLIENTS; i++)
                {
                    if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
                        Mask |= CmaskOne(i);
                }
                GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
            
                // set attacker's face to happy (taunt!)
            
                CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
                if (pChr)
                {
                    pChr->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed());
                }
            }
            break;
        }

        pChar->Core().m_Vel += Force;

        return true;
    }
}

bool CGameControllerBasePvP::OnEntity(int Index, vec2 Pos)
{
    if(m_InstagibWeapon != -1)
    {
        if(Index == ENTITY_ARMOR_1 || Index == ENTITY_HEALTH_1 || Index == ENTITY_WEAPON_GRENADE || Index == ENTITY_WEAPON_RIFLE || Index == ENTITY_WEAPON_SHOTGUN || Index == ENTITY_POWERUP_NINJA)
            return true;
    }

    return IGameController::OnEntity(Index,Pos);
}