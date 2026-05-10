#include "stdafx.h"
#include "MapSerializer.h"
#include "Map.h"
#include <string>
bool MapSerializer::Save(const World::Map& map, const std::string& path) { (void)map; (void)path; return false; }
bool MapSerializer::Load(World::Map& map, const std::string& path) { (void)map; (void)path; return false; }
