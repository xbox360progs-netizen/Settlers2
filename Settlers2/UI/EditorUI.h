#ifndef SETTLERS2_UI_EDITORUI_H
#define SETTLERS2_UI_EDITORUI_H

namespace UI { class UIManager; }

class EditorUI {
public:
  EditorUI(UI::UIManager* uiManager);
  ~EditorUI();
  void Render();
  void Update(float dt);
private:
  UI::UIManager* m_uiManager;
};

#endif // SETTLERS2_UI_EDITORUI_H
