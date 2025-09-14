/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "dm.h"
#include <game/generated/protocol.h>


CGameControllerDM::CGameControllerDM(class CGameContext *pGameServer)
: CGameControllerBasePvP(pGameServer)
{
	switch (m_InstagibWeapon)
	{
	case WEAPON_GRENADE:
		m_pGameType = "gDM!";
		break;
	case WEAPON_LASER:
		m_pGameType = "iDM!";
		break;
	default:
		m_pGameType = "DM!";
		break;
	}
}

void CGameControllerDM::Tick()
{
	CGameControllerBasePvP::Tick();
}
