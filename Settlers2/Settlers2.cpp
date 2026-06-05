//-------------------------------------------------------------------------------------
// Settlers 2 - Entry Point
//-------------------------------------------------------------------------------------
#include "stdafx.h"

#include "Core\GameEngine.h"

//-------------------------------------------------------------------------------------
// Main Entry Point
//-------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    OutputDebugStringA("[Settlers2::main] START\n");

    GameEngine engine;

    OutputDebugStringA("[Settlers2::main] Initializing engine\n");
    if (!engine.Initialize())
    {
        OutputDebugStringA("[Settlers2::main] FAILED to initialize game engine\n");
        return 1;
    }

    OutputDebugStringA("[Settlers2::main] Engine initialized, running\n");
    engine.Run();

    OutputDebugStringA("[Settlers2::main] Engine stopped\n");
    return 0;
}
