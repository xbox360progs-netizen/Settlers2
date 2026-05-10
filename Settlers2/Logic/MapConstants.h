//Logic/MapConstants.h
#pragma once

#define MAP_WIDTH  20
#define MAP_HEIGHT 20

#define TERRAIN_TILE_WIDTH  238
#define TERRAIN_TILE_HEIGHT 148

#define NODE_TILE_WIDTH   119
#define NODE_TILE_HEIGHT 72

#define ISO_TILE_WIDTH 128.0f
#define ISO_TILE_HEIGHT 64.0f

#define ISO_HALF_W (ISO_TILE_WIDTH * 0.5f)
#define ISO_HALF_H (ISO_TILE_HEIGHT * 0.5f)

// Isometric world dimensions
#define MAP_WORLD_W (MAP_WIDTH  * TERRAIN_TILE_WIDTH)
#define MAP_WORLD_H (MAP_HEIGHT * TERRAIN_TILE_HEIGHT)

#define ISO_MIN_X (-MAP_WORLD_H / ISO_HALF_W)
#define ISO_MAX_X (MAP_WORLD_W / ISO_HALF_W)

#define ISO_MIN_Y (0.0f)
#define ISO_MAX_Y ((MAP_WORLD_W + MAP_WORLD_H) / ISO_HALF_H)

#define ISO_GRID_WIDTH  ((int)ceilf(ISO_MAX_X - ISO_MIN_X))
#define ISO_GRID_HEIGHT ((int)ceilf(ISO_MAX_Y - ISO_MIN_Y))

// Diamond clipping parameters
#define DIAMOND_CENTER_X (ISO_GRID_WIDTH * 0.5f)
#define DIAMOND_CENTER_Y (ISO_GRID_HEIGHT * 0.5f)
#define DIAMOND_RADIUS ((ISO_GRID_WIDTH + ISO_GRID_HEIGHT) * 0.25f)