#pragma once
#include <string>
#include "../Core/CommandBus.h"
#include "../Core/EventBus.h"
#include "../Core/Vector2i.h"
#include "../Input/InputManager.h"
#include "../World/FlagManager.h"
#include "PlacementController.h"
#include "RoadController.h"

namespace World {
    class Map;
}

class GridMenu;
class UIMenu;

namespace Scene {

    // GameScene implements this to receive actions that require
    // complex game-state manipulation (deletion, geologist).
    class IInputHost {
    public:
        virtual ~IInputHost() {}
        // User confirmed deletion of flag+building at (tx, ty)
        virtual void DeleteFlagAt(int tileX, int tileY) = 0;
        // User pressed A on an un-surveyed mountain tile
        virtual void OnMountainTileAction(int tileX, int tileY) = 0;
        // User pressed B while geologist confirm is showing
        virtual void CancelGeologist() = 0;
    };

    class InputController {
    public:
        InputController(Input::InputManager* inputManager,
                        Core::CommandBus& commandBus,
                        Core::EventBus* eventBus,
                        World::Map* map,
                        World::FlagManager* flagManager,
                        PlacementController* placement,
                        RoadController* roadController,
                        GridMenu* buildMenu,
                        UIMenu* flagMenu);

        void SetHost(IInputHost* host) { m_host = host; }

        // Called once per frame from GameScene::Update
        void Tick(float dt);
        void HandleGamepadInput();
        void HandleInput();

        // Status text (read by GameScene for banner/render)
        const std::string& GetStatusText() const { return m_statusText; }
        float GetStatusTextTimer() const { return m_statusTextTimer; }
        void SetStatusText(const char* text, float timer);

        // Cursor position (read/write by GameScene for camera → tile mapping)
        int GetCursorTileX() const { return m_cursorTileX; }
        int GetCursorTileY() const { return m_cursorTileY; }
        void SetCursorTile(int x, int y) { m_cursorTileX = x; m_cursorTileY = y; }
        bool GetCursorOnTownHall() const { return m_cursorOnTownHall; }
        void SetCursorOnTownHall(bool v) { m_cursorOnTownHall = v; }

        // Gamepad cursor (read by GameScene for rendering)
        bool IsGamepadActive() const { return m_gamepadActive; }
        const Vector2i& GetGamepadCursor() const { return m_gamepadCursor; }
        float GetGamepadCursorCooldown() const { return m_gamepadCursorCooldown; }
        void SetGamepadCursorCooldown(float v) { m_gamepadCursorCooldown = v; }

        // Menu state flags (read by GameScene for camera/render gating)
        bool IsMenuActive() const { return m_menuActive; }
        bool IsFlagMenuActive() const { return m_flagMenuActive; }
        bool IsRoadMenuActive() const { return m_roadMenuActive; }
        bool IsGeologistMenuActive() const { return m_geologistMenuActive; }
        bool IsTownHallPanelOpen() const { return m_townHallPanelOpen; }
        bool IsLogisticsDebug() const { return m_logisticsDebug; }
        void SetMenuActive(bool v) { m_menuActive = v; }
        void SetFlagMenuActive(bool v) { m_flagMenuActive = v; }
        void SetRoadMenuActive(bool v) { m_roadMenuActive = v; }
        void SetGeologistMenuActive(bool v) { m_geologistMenuActive = v; }
        void SetTownHallPanelOpen(bool v) { m_townHallPanelOpen = v; }
        void ToggleLogisticsDebug() { m_logisticsDebug = !m_logisticsDebug; }

        // Called by GameScene when geologist survey completes
        void OnGeologistComplete() { m_geologistMenuActive = false; }

    private:
        void HandlePlaceAtCursor();
        void HandleConfirmFreeFlag();

        Input::InputManager* m_inputManager;
        Core::CommandBus& m_commandBus;
        Core::EventBus* m_eventBus;
        World::Map* m_map;
        World::FlagManager* m_flagManager;
        PlacementController* m_placement;
        RoadController* m_roadController;
        GridMenu* m_buildMenu;
        UIMenu* m_flagMenu;
        IInputHost* m_host;

        // Cursor
        int m_cursorTileX;
        int m_cursorTileY;
        Vector2i m_gamepadCursor;
        float m_gamepadCursorCooldown;
        bool m_gamepadActive;
        bool m_cursorOnTownHall;

        // Menu state
        bool m_menuActive;
        bool m_roadMenuActive;
        bool m_flagMenuActive;
        bool m_geologistMenuActive;
        bool m_townHallPanelOpen;
        bool m_logisticsDebug;

        // Status text
        std::string m_statusText;
        float m_statusTextTimer;
    };

}
