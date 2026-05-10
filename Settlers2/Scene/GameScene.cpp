#include "stdafx.h"
#include "GameScene.h"

namespace Scene {

GameScene::GameScene()
    : Scene("Game")
{
}

GameScene::~GameScene()
{
}

void GameScene::Load()
{
    m_loaded = true;
}

void GameScene::Unload()
{
    m_loaded = false;
}

void GameScene::Update(float deltaTime)
{
    // TODO: Игровая логика
}

void GameScene::Render()
{
    // TODO: Рендеринг игрового мира
}

} // namespace Scene
