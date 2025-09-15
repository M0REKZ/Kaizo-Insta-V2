/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <math.h>
#include <engine/map.h>
#include <engine/kernel.h>

#include <game/layers.h>
#include <game/collision.h>
#include <game/mapitems.h>

CCollision::CCollision()
{
	m_pTiles = 0;
	m_Width = 0;
	m_Height = 0;
	m_pLayers = 0;
}

void CCollision::Init(class CLayers *pLayers)
{
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));
	m_pTele = 0;
	m_pSpeedup = 0;
	m_pFront = 0;

	if(m_pLayers->SpeedupLayer())
		m_pSpeedup = static_cast<CSpeedupTile *>(m_pLayers->Map()->GetData(m_pLayers->SpeedupLayer()->m_Speedup));
	if(m_pLayers->FrontLayer())
	    m_pFront = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->FrontLayer()->m_Front));
	if(m_pLayers->TeleLayer())
	{
    	m_pTele = static_cast<CTeleTile *>(m_pLayers->Map()->GetData(m_pLayers->TeleLayer()->m_Tele));

    	for(int i = 0; i < m_Width * m_Height; i++)
    	{
    		int Number = m_pTele[i].m_Number;
    		int Type = m_pTele[i].m_Type;
    		if(Number > 0)
    		{
    			if(Type == TILE_TELEOUT)
    			{
    				m_TeleOuts[Number].emplace_back(i % m_Width * 32.0f + 16.0f, i / m_Width * 32.0f + 16.0f);
    			}
    			else if(Type == TILE_TELECHECKOUT)
    			{
    				m_TeleCheckOuts[Number].emplace_back(i % m_Width * 32.0f + 16.0f, i / m_Width * 32.0f + 16.0f);
    			}
    		}
    	}
	}
}

int CCollision::GetTile(int x, int y)
{
	int Nx = clamp(x/32, 0, m_Width-1);
	int Ny = clamp(y/32, 0, m_Height-1);

	return m_pTiles[Ny*m_Width+Nx].m_Index > 128 ? 0 : m_pTiles[Ny*m_Width+Nx].m_Index;
}

int CCollision::GetTileF(int x, int y)
{
	if(!m_pFront)
		return 0;

	int Nx = clamp(x/32, 0, m_Width-1);
	int Ny = clamp(y/32, 0, m_Height-1);

	return m_pFront[Ny*m_Width+Nx].m_Index > 128 ? 0 : m_pFront[Ny*m_Width+Nx].m_Index;
}

bool CCollision::IsTileSolid(int x, int y)
{
	int Tile = GetTile(x, y);
	return Tile == TILE_SOLID || Tile == TILE_NOHOOK;
}

// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, int WhoChecks)
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance+1);
	vec2 Last = Pos0;

	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
		vec2 Pos = mix(Pos0, Pos1, a);
		// Start hooktrough
		int ix = round_to_int(Pos.x);
		int iy = round_to_int(Pos.y);

		int hit = GetCollisionAt(ix, iy);
		int fhit = GetCollisionAtFront(ix, iy);
		if(CheckPoint(ix, iy))
		{
			if(WhoChecks == 1 /* 1=Hook */ && (fhit == TILE_THROUGH || fhit == TILE_THROUGHOLD))
				hit = 0;
		}
		if(hit)
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return hit;
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

// TODO: OPT: rewrite this smarter!
void CCollision::MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces)
{
	if(pBounces)
		*pBounces = 0;

	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	if(CheckPoint(Pos + Vel))
	{
		int Affected = 0;
		if(CheckPoint(Pos.x + Vel.x, Pos.y))
		{
			pInoutVel->x *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(CheckPoint(Pos.x, Pos.y + Vel.y))
		{
			pInoutVel->y *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(Affected == 0)
		{
			pInoutVel->x *= -Elasticity;
			pInoutVel->y *= -Elasticity;
		}
	}
	else
	{
		*pInoutPos = Pos + Vel;
	}
}

bool CCollision::TestBox(vec2 Pos, vec2 Size)
{
	Size *= 0.5f;
	if(CheckPoint(Pos.x-Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x-Size.x, Pos.y+Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y+Size.y))
		return true;
	return false;
}

void CCollision::MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity)
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	float Distance = length(Vel);
	int Max = (int)Distance;

	if(Distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float Fraction = 1.0f/(float)(Max+1);
		for(int i = 0; i <= Max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;

			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice

			if(TestBox(vec2(NewPos.x, NewPos.y), Size))
			{
				int Hits = 0;

				if(TestBox(vec2(Pos.x, NewPos.y), Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}

				if(TestBox(vec2(NewPos.x, Pos.y), Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
				}
			}

			Pos = NewPos;
		}
	}

	*pInoutPos = Pos;
	*pInoutVel = Vel;
}

int CCollision::GetPureMapIndex(float x, float y) const
{
	int Nx = clamp(round_to_int(x) / 32, 0, m_Width - 1);
	int Ny = clamp(round_to_int(y) / 32, 0, m_Height - 1);
	return Ny * m_Width + Nx;
}


int CCollision::GetMapIndex(vec2 Pos) const
{
	int Nx = clamp((int)Pos.x / 32, 0, m_Width - 1);
	int Ny = clamp((int)Pos.y / 32, 0, m_Height - 1);
	int Index = Ny * m_Width + Nx;
	return Index;
}

int CCollision::IsSpeedup(int Index) const
{
	if(Index < 0 || !m_pSpeedup)
		return 0;

	if(m_pSpeedup[Index].m_Force > 0)
		return Index;

	return 0;
}

int CCollision::GetSpeedup(int Index, vec2 *Dir, int *Force, int *MaxSpeed) const
{
	if(!m_pSpeedup || Index < 0 )
		return -1;
	float Angle = m_pSpeedup[Index].m_Angle * (pi / 180.0f);
	*Force = m_pSpeedup[Index].m_Force;
	*Dir = vec2(cos(Angle), sin(Angle));
	if(MaxSpeed)
		*MaxSpeed = m_pSpeedup[Index].m_MaxSpeed;
	return m_pSpeedup[Index].m_Type;
}

const std::vector<vec2> &CCollision::TeleOuts(int Number) const
{
	static const std::vector<vec2> sEmptyVector;
	return m_TeleOuts.contains(Number) ? m_TeleOuts.at(Number) : sEmptyVector;
}

const std::vector<vec2> &CCollision::TeleCheckOuts(int Number) const
{
	static const std::vector<vec2> sEmptyVector;
	return m_TeleCheckOuts.contains(Number) ? m_TeleCheckOuts.at(Number) : sEmptyVector;
}
