// Главная точка входа: через SceneManager и MenuScene (еволюционная MVP-структура)
#include "stdafx.h"
#include <windows.h>
#include <iostream>

// Подключение к менеджеру сцен и MenuScene
#include "Settlers2/Settlers2/Scene/SceneManager.h"
#include "Settlers2/Settlers2/Scene/MenuScene.h"

using namespace Scene;

class Settlers2App {
public:
  Settlers2App() : m_menu(NULL), m_running(false) {}
  ~Settlers2App() { delete m_menu; }

  bool Initialize() {
    m_menu = new MenuScene();
    m_manager.AddScene(m_menu);
    m_manager.SwitchTo(m_menu->GetName());
    return true;
  }

  void Run() {
    m_running = true;
    DWORD last = GetTickCount();
    while (m_running) {
      DWORD now = GetTickCount();
      float dt = (now - last) / 1000.0f;
      last = now;
      if (dt < 0.001f) dt = 0.016f;
      m_manager.Update(dt);
      m_manager.Render();
      if (m_menu && m_menu->IsExitRequested()) {
        m_running = false;
      }
      Sleep(16);
    }
    m_manager.Clear();
  }

private:
  SceneManager m_manager;
  MenuScene* m_menu;
  bool m_running;
};

int main(int argc, char** argv) {
  (void)argc; (void)argv;
  std::cout << "Settlers2 entrypoint reached. Initializing MenuScene via SceneManager..." << std::endl;
  Settlers2App app;
  if (!app.Initialize()) return 1;
  app.Run();
  return 0;
}
