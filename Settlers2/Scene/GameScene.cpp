#include "stdafx.h"
#include "GameScene.h"
#include "../Graphics/RenderQueue.h"

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

void GameScene::Render(RenderQueue* renderQueue)
{
    (void)renderQueue;
}

} // namespace Scene
