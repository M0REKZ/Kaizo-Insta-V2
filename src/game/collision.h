/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>
#include <base/tl/array.h>
#include <base/math.h>
#include <game/mapitems.h>

#include <map>
#include <vector>


class CCollision
{
	class CTile *m_pTiles;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;

	bool IsTileSolid(int x, int y);
	int GetTile(int x, int y);
	int GetTileF(int x, int y);

	int GetPureMapIndex(float x, float y) const;
	int GetPureMapIndex(vec2 Pos) const { return GetPureMapIndex(Pos.x, Pos.y); }

public:
	enum
	{
		WHOCHECKS_HOOK=1,
	};
	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y) { return IsTileSolid(round_to_int(x), round_to_int(y)); }
	bool CheckPoint(vec2 Pos) { return CheckPoint(Pos.x, Pos.y); }
	int GetCollisionAt(float x, float y) { return GetTile(round_to_int(x), round_to_int(y)); }
	int GetCollisionAtFront(float x, float y) { return GetTileF(round_to_int(x), round_to_int(y)); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, int WhoChecks = 0);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity);
	bool TestBox(vec2 Pos, vec2 Size);

	CTeleTile *TeleLayer() { return m_pTele; }
	const std::vector<vec2> &TeleOuts(int Number) const;
	const std::vector<vec2> &TeleCheckOuts(int Number) const;

	int IsSpeedup(int Index) const;
	int GetMapIndex(vec2 Pos) const;
	void GetSpeedup(int Index, vec2 *Dir, int *Force, int *MaxSpeed) const;
	
	private:
	CTile *m_pFront;
	CTeleTile *m_pTele;
	CSpeedupTile *m_pSpeedup;
	std::map<int, std::vector<vec2>> m_TeleOuts;
	std::map<int, std::vector<vec2>> m_TeleCheckOuts;
};

#endif
