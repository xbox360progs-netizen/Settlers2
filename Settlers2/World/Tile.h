#pragma once

#include "TileType.h"
#include <d3dx9math.h>

namespace World {

struct Tile
{
    TileType type;
    int x;
    int y;
    float height;
    D3DXVECTOR2 uvOffset;
    bool walkable;
    bool buildable;
    int regionIndex;
    float u0, v0, u1, v1;
    std::string atlasName;

    Tile()
        : type(None)
        , x(0), y(0)
        , height(0.0f)
        , uvOffset(0.0f, 0.0f)
        , walkable(true)
        , buildable(true)
        , regionIndex(-1)
        , u0(0.0f), v0(0.0f), u1(0.0f), v1(0.0f)
    {
    }

    Tile(int tileX, int tileY, TileType tileType = None)
        : type(tileType)
        , x(tileX)
        , y(tileY)
        , height(0.0f)
        , uvOffset(0.0f, 0.0f)
        , walkable(true)
        , buildable(true)
        , regionIndex(0)
        , u0(0.0f), v0(0.0f), u1(0.0f), v1(0.0f)
    {
        UpdateProperties();
    }

	void UpdateProperties()
	{
		switch (type)
		{
		case Mountain:
		case MountainOnWater:
			walkable = false;
			buildable = false;
			height = 1.0f;
			break;
		case Building:
			walkable = false;
			buildable = false;
			height = 1.0f;
			break;
		case Tree:
		case Rock:
		case Decoration:
			walkable = true;
			buildable = false; // ������ ������� ��� �������
			height = 0.5f;     // ������ ��� �����
			break;
		default:
			walkable = true;
			buildable = true;
			height = 0.0f;
			break;
		}
	}
};

} // namespace World
