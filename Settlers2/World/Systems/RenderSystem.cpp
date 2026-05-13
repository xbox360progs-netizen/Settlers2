#include "stdafx.h"
#include "RenderSystem.h"
#include "../EntityManager.h"
#include "../Components/IComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/SpriteComponent.h"
// #include "../../Renderer/IRenderer.h" // TODO: IRenderer not implemented yet
// #include "../../Renderer/RenderTypes.h" // TODO: RenderTypes not implemented yet

namespace World {

RenderSystem::RenderSystem()
{
}

RenderSystem::~RenderSystem()
{
}

void RenderSystem::Update(EntityManager* entityManager, Renderer* renderer, float deltaTime)
{
    // TODO: Re-enable when IRenderer is implemented
    /*
    if (!entityManager || !renderer)
    {
        return;
    }

    // Получаем все сущности
    std::vector<EntityID> entityIDs;
    entityManager->GetAllEntityIDs(entityIDs);

    // Рендерим каждую сущность со спрайтом
    for (std::vector<EntityID>::iterator it = entityIDs.begin(); it != entityIDs.end(); ++it)
    {
        RenderSprite(entityManager, renderer, *it);
    }
    */
}

void RenderSystem::RenderSprite(EntityManager* entityManager, Renderer* renderer, EntityID entityID)
{
    // TODO: Re-enable when IRenderer is implemented
    /*
    // Получаем компоненты
    TransformComponent* transform = entityManager->GetComponent<TransformComponent>(entityID);
    SpriteComponent* sprite = entityManager->GetComponent<SpriteComponent>(entityID);

    if (!transform || !sprite)
    {
        return;
    }

    if (!sprite->visible)
    {
        return;
    }

    // Конвертируем в Renderer типы
    Renderer::Transform renderTransform;
    renderTransform.position = transform->position;
    renderTransform.rotation = transform->rotation;
    renderTransform.scale = transform->scale;

    Renderer::Sprite renderSprite;
    renderSprite.texture = sprite->texture;
    renderSprite.uvMin = sprite->uvMin;
    renderSprite.uvMax = sprite->uvMax;
    renderSprite.size = sprite->size;
    renderSprite.color = sprite->color;

    renderer->DrawSprite(&renderSprite, renderTransform);
    */
}

} // namespace World
