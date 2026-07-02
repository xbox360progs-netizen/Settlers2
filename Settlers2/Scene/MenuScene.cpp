#include "stdafx.h"
#include "MenuScene.h"
#include <iostream>
#include "SceneManager.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/Texture.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/RenderLayers.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/RenderCommandBuilder.h"

using namespace Scene;
using namespace UI;
#include <d3dx9.h>

static std::string WToA(const std::wstring& w) {
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), NULL, 0, NULL, NULL);
    if (sizeNeeded <= 0) return std::string();
    std::string out(sizeNeeded, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &out[0], sizeNeeded, NULL, NULL);
    return out;
}

static std::wstring ToWideString(const std::string& s) {
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    if (sizeNeeded <= 0) return std::wstring();
    std::wstring out(sizeNeeded - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], sizeNeeded);
    return out;
}

static const UI::MenuItem MAIN_ITEMS[4] = {
    UI::MenuItem(MSG_MENU_NEW_GAME,   UI::UiAction(UI_CMD_NEW_GAME)),
    UI::MenuItem(MSG_MENU_MAP_EDITOR, UI::UiAction(UI_CMD_MAP_EDITOR)),
    UI::MenuItem(MSG_MENU_SETTINGS,   UI::UiAction(UI_CMD_SETTINGS)),
    UI::MenuItem(MSG_MENU_EXIT,       UI::UiAction(UI_CMD_EXIT)),
};

static const UI::MenuItem SIZE_ITEMS[MAP_SIZE_COUNT] = {
    UI::MenuItem(MSG_NONE, UI::UiAction(UI_CMD_SELECT, 20)),
    UI::MenuItem(MSG_NONE, UI::UiAction(UI_CMD_SELECT, 40)),
    UI::MenuItem(MSG_NONE, UI::UiAction(UI_CMD_SELECT, 60)),
    UI::MenuItem(MSG_NONE, UI::UiAction(UI_CMD_SELECT, 100)),
};

MenuScene::MenuScene()
  : SceneBase("MenuScene"), m_backgroundPath(""),
    m_dispatcher(NULL), m_device(NULL), m_spriteRenderer(NULL), m_renderer(NULL), m_gamepad(NULL),
    m_textManager(NULL), m_binFileManager(NULL), m_textureLoader(NULL), m_stickTimer(0.0f),
    m_menuState(MENU_STATE_MAIN), m_prevMainSelection(0) {
}

MenuScene::~MenuScene() {
}

void MenuScene::Load() {
  m_loaded = true;
  OutputDebugStringA("[MenuScene] Load() called\n");

  if (!m_spriteRenderer) {
    OutputDebugStringA("[MenuScene] WARNING: m_spriteRenderer is NULL in Load()\n");
  } else {
    OutputDebugStringA("[MenuScene] m_spriteRenderer is valid in Load()\n");
  }
}

void MenuScene::ResetTextureState() {
    OutputDebugStringA("[MenuScene::ResetTextureState] START\n");
    TextureRegistry& registry = TextureRegistry::instance();
    OutputDebugStringA("[MenuScene::ResetTextureState] Got registry\n");

    LPDIRECT3DTEXTURE9 tex = registry.getTextureOrLoad("menu_background");
    OutputDebugStringA("[MenuScene::ResetTextureState] Got texture\n");

    if (tex) {
        m_backgroundTexture.SetTexture(tex);
        if (m_spriteRenderer) {
            m_spriteRenderer->SetTextureSlot(0, tex);
        }
        OutputDebugStringA("[MenuScene::ResetTextureState] Texture set OK\n");
    } else {
        OutputDebugStringA("[MenuScene::ResetTextureState] ERROR no texture\n");
    }
    OutputDebugStringA("[MenuScene::ResetTextureState] END\n");
}

void MenuScene::LoadTextures() {
    OutputDebugStringA("[MenuScene::LoadTextures] START\n");
    ResetTextureState();
    OutputDebugStringA("[MenuScene::LoadTextures] END\n");
}

void MenuScene::Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer) {
  m_device = device;
  m_spriteRenderer = spriteRenderer;

  std::cout << "[MenuScene] Initialize called" << std::endl;

    if (!spriteRenderer) {
        OutputDebugStringA("[MenuScene::Initialize] ERROR: spriteRenderer is NULL!\n");
    }

  LoadTextures();
}

void MenuScene::Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer, Renderer* renderer, Input::Gamepad* gamepad, TextureLoader* textureLoader) {
  m_device = device;
  m_spriteRenderer = spriteRenderer;
  m_renderer = renderer;
  m_gamepad = gamepad;
  m_textureLoader = textureLoader;

  char buf[256];
  sprintf(buf, "[MenuScene::Initialize] EXTENDED this=%p, m_device=%p, m_spriteRenderer=%p, m_renderer=%p\n", this, m_device, m_spriteRenderer, m_renderer);
  OutputDebugStringA(buf);
  OutputDebugStringA("[MenuScene] Calling LoadTextures...\n");
  LoadTextures();
  OutputDebugStringA("[MenuScene] LoadTextures returned\n");
}

void MenuScene::Unload() {
  m_loaded = false;
}

void MenuScene::OnEnter() {
  OutputDebugStringA("[MenuScene::OnEnter] START\n");
  ClearExitRequest();
  ShowMainMenu();
  ResetTextureState();
  OutputDebugStringA("[MenuScene::OnEnter] END\n");
}

void MenuScene::OnExit() {
}

void MenuScene::ShowMainMenu() {
    m_menuState = MENU_STATE_MAIN;
    m_menuModel.SetItems(MAIN_ITEMS, 4);
    if (m_prevMainSelection >= 0 && m_prevMainSelection < m_menuModel.GetItemCount())
        m_menuModel.SetSelected(m_prevMainSelection);
}

void MenuScene::ShowSizeSelect() {
    m_prevMainSelection = m_menuModel.GetSelected();
    m_menuState = MENU_STATE_SIZE_SELECT;
    m_menuModel.SetItems(SIZE_ITEMS, MAP_SIZE_COUNT);
    m_menuModel.SetSelected(0);
}

void MenuScene::ProcessInput(float deltaTime) {
  if (!m_gamepad) return;

  if (m_gamepad->IsButtonPressed(Input::GP_DPadUp)) {
    m_menuModel.SelectPrevious();
  }
  if (m_gamepad->IsButtonPressed(Input::GP_DPadDown)) {
    m_menuModel.SelectNext();
  }

  float lx, ly;
  m_gamepad->GetLeftStick(lx, ly);
  if (fabsf(ly) > 0.5f) {
    m_stickTimer -= deltaTime;
    if (m_stickTimer <= 0.0f) {
      if (ly > 0) {
        m_menuModel.SelectNext();
      } else {
        m_menuModel.SelectPrevious();
      }
      m_stickTimer = 0.2f;
    }
  } else {
    m_stickTimer = 0.0f;
  }

  if (m_gamepad->IsButtonPressed(Input::GP_A)) {
    ExecuteMenuItem();
  }
  if (m_gamepad->IsButtonPressed(Input::GP_B)) {
    if (m_menuState == MENU_STATE_SIZE_SELECT) {
      ShowMainMenu();
    }
  }
}

void MenuScene::ExecuteMenuItem() {
  UI::UiAction action = m_menuModel.GetSelectedAction();

  char buf[256];
  sprintf(buf, "[MenuScene] ExecuteMenuItem called, cmd=%d, value=%d\n",
          (int)action.command, action.value);
  OutputDebugStringA(buf);

  switch (action.command) {
    case UI_CMD_MAP_EDITOR:
      ShowSizeSelect();
      break;
    default:
      if (m_dispatcher) {
        m_dispatcher->Dispatch(action);
      }
      break;
  }
}

void MenuScene::Update(float deltaTime) {
  ProcessInput(deltaTime);
}

void MenuScene::Render(RenderQueue* renderQueue) {
    if (!renderQueue) {
        OutputDebugStringA("[MenuScene::Render] renderQueue=NULL\n");
        return;
    }

    if (!m_backgroundTexture.GetTexture()) {
        OutputDebugStringA("[MenuScene::Render] texture NULL - nothing to render\n");
        return;
    }

    Graphics::RenderCommandBuilder()
        .Position(0.0f, 0.0f)
        .Size(1280.0f, 720.0f)
        .UV(0.0f, 0.0f, 1.0f, 1.0f)
        .Texture(0)
        .Shader(SHADER_SPRITE)
        .Layer(LAYER_UI)
        .Depth(900)
        .Submit(renderQueue);

    TextureRegistry& reg = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
    LPDIRECT3DTEXTURE9 uiTex = uiAtlas ? uiAtlas->GetTexture() : NULL;
    if (uiTex && m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(5, uiTex);
    }

    if (uiAtlas) {
        uint32_t gridIdx = uiAtlas->GetIndex("menu_Grid");
        if (gridIdx != 0xFFFFFFFF) {
            const SpriteRegion* gridReg = uiAtlas->GetRegion(gridIdx);
            if (gridReg) {
                Graphics::RenderCommandBuilder()
                    .UIElement(100.0f, 170.0f, 400.0f, 340.0f, gridReg->u0, gridReg->v0, gridReg->u1, gridReg->v1, 5, 899)
                    .Submit(renderQueue);
            }
        }
    }

    if (m_textManager && m_spriteRenderer) {
        // ── Draw title for size select ──────────────────────────────
        if (m_menuState == MENU_STATE_SIZE_SELECT) {
            const char* title = m_loc.Get(MSG_MENU_SIZE_SELECT_TITLE);
            m_textManager->DrawString(title, 125.0f, 208.0f, 0xFFFFD700, 0.30f);
        }

        // ── Draw menu items ─────────────────────────────────────────
        float startY = (m_menuState == MENU_STATE_SIZE_SELECT) ? 278.0f : 208.0f;
        float spacingY = 70.0f;
        int itemCount = m_menuModel.GetItemCount();

        for (int i = 0; i < itemCount; ++i) {
            const UI::MenuItem* item = m_menuModel.GetItem(i);
            if (!item || !item->visible) continue;

            char itemText[64];
            if (item->labelId == MSG_NONE) {
                sprintf_s(itemText, "%d x %d", item->action.value, item->action.value);
            } else {
                const char* resolved = m_loc.Get(item->labelId);
                strcpy_s(itemText, resolved ? resolved : "");
            }

            bool isSelected = (i == m_menuModel.GetSelected());
            DWORD itemColor = isSelected ? 0xFFFFD700 : 0xFFFFFFFF;
            float itemX = isSelected ? 180.0f : 140.0f;
            m_textManager->DrawString(itemText, itemX, startY + (i * spacingY), itemColor, 0.3f);

            if (isSelected && uiAtlas) {
                uint32_t btnIdx = uiAtlas->GetIndex("button_A");
                if (btnIdx != 0xFFFFFFFF) {
                    const SpriteRegion* btnReg = uiAtlas->GetRegion(btnIdx);
                    if (btnReg) {
                        Graphics::RenderCommandBuilder()
                            .UIElement(140.0f, startY + (i * spacingY) - 4.0f, 32.0f, 32.0f, btnReg->u0, btnReg->v0, btnReg->u1, btnReg->v1, 5, 898)
                            .Submit(renderQueue);
                    }
                }
            }
        }

        // ── Button hints at bottom ──────────────────────────────────
        if (uiAtlas) {
            uint32_t backIdx = uiAtlas->GetIndex("button_back");
            uint32_t startIdx = uiAtlas->GetIndex("button_start");
            if (backIdx != 0xFFFFFFFF) {
                const SpriteRegion* backReg = uiAtlas->GetRegion(backIdx);
                if (backReg) {
                    Graphics::RenderCommandBuilder()
                        .UIElement(20.0f, 660.0f, 28.0f, 28.0f, backReg->u0, backReg->v0, backReg->u1, backReg->v1, 5, 898)
                        .Submit(renderQueue);
                }
            }
            const char* backText = m_loc.Get(MSG_MENU_HINT_BACK);
            m_textManager->DrawString(backText, 52.0f, 664.0f, 0xFF888888, 0.22f);

            if (startIdx != 0xFFFFFFFF) {
                const SpriteRegion* startReg = uiAtlas->GetRegion(startIdx);
                if (startReg) {
                    Graphics::RenderCommandBuilder()
                        .UIElement(120.0f, 660.0f, 40.0f, 28.0f, startReg->u0, startReg->v0, startReg->u1, startReg->v1, 5, 898)
                        .Submit(renderQueue);
                }
            }
            const char* selectText = m_loc.Get(MSG_MENU_HINT_SELECT);
            m_textManager->DrawString(selectText, 165.0f, 664.0f, 0xFF888888, 0.22f);
        }
    }
}

void MenuScene::SetBackground(const std::string& path) {
  m_backgroundPath = path;
}

void MenuScene::SetRenderer(SpriteRenderer* spriteRenderer, Renderer* renderer) {
    char buf[256];
    sprintf(buf, "[MenuScene::SetRenderer] old sprite=%p, new sprite=%p, renderer=%p\n",
            m_spriteRenderer, spriteRenderer, renderer);
    OutputDebugStringA(buf);
    m_renderer = renderer;
    m_spriteRenderer = spriteRenderer;
}
