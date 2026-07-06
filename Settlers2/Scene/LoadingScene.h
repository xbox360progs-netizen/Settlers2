#pragma once

#include "Scene.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/BinFileManager.h"
#include "../Graphics/Texture.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/ShaderManager.h"
#include <vector>
#include <functional>

using Graphics::SpriteRenderer;
using Graphics::ShaderManager;

namespace Scene {

class LoadingScene : public SceneBase
{
public:
    LoadingScene();
    virtual ~LoadingScene();

    virtual void Load();
    virtual void Unload();
    virtual void OnExit();
    virtual void Update(float deltaTime);
    virtual void Render(Graphics::RenderQueue* renderQueue) override;

    void SetTargetScene(const std::string& sceneName);
    void SetTextureLoader(TextureLoader* loader) { m_textureLoader = loader; }
    void SetRenderer(Renderer* renderer) { m_renderer = renderer; }
    void SetSpriteRenderer(SpriteRenderer* spriteRenderer) { m_spriteRenderer = spriteRenderer; }
    void SetShaderManager(class ShaderManager* shaderManager) { m_shaderManager = shaderManager; }
    void SetBinFileManager(BinFileManager* binFileManager) { m_binFileManager = binFileManager; }

    void AddLoadTask(std::function<void()> task, const std::string& name, float weight);
    float GetTotalProgress() const;

    void SetLoadProgress(float progress) { m_loadProgress = progress; }
    void SetStatusText(const std::string& text) { m_statusText = text; }

    const std::string& GetCurrentTaskName() const;
    const std::string& GetStatusText() const { return m_statusText; }

private:
    void ExecuteCurrentTask();
    void CreateNextScene();
	void SetupLoadTasks();
    void LoadAtlasOrTexture(const char* name, const char* pngPath);
    void StartAsyncLoading();

    void AsyncLoadResources();

    struct LoadTask {
        std::function<void()> taskFunc;
        std::string name;
        float weight;
    };

    std::string m_targetScene;
    TextureLoader* m_textureLoader;
    Renderer* m_renderer;
    SpriteRenderer* m_spriteRenderer;
    ShaderManager* m_shaderManager;
    BinFileManager* m_binFileManager;
    Texture m_backgroundTexture;
    LPDIRECT3DTEXTURE9 m_progressTexture;
    std::vector<LoadTask> m_loadTasks;
    float m_screenW;
    float m_screenH;
    size_t m_currentTaskIndex;
    float m_completedWeight;
    float m_totalWeight;
    float m_currentTaskProgress;
    float m_loadProgress;
    std::string m_statusText;
    bool m_loadingComplete;
    bool m_loadStarted;

    volatile long m_targetProgressPercentage;
    volatile long m_isLoadComplete;
    float m_currentRenderProgress;
};

}
