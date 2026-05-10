//-------------------------------------------------------------------------------------
// Settlers 2 - Entry Point
//-------------------------------------------------------------------------------------
#include "stdafx.h"

#include "Core\GameEngine.h"
#include <iostream>

//-------------------------------------------------------------------------------------
// Main Entry Point
//-------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    GameEngine engine;

    if (!engine.Initialize())
    {
        std::cerr << "[Settlers2] Failed to initialize game engine" << std::endl;
        return 1;
    }

    engine.Run();

    return 0;
}
