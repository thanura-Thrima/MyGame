#pragma once

#include <memory>
#include <functional>

#include <android/asset_manager.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "EventHandler.h"
#include "Renderer/Renderer.h"



struct AAssetManagerDeleter {
    void operator()(AAssetManager *assetManager)
    {
        //Do nothing
        //AAsset(assetManager);
    }
};
class IGfxDevice;
class FileManager;
class GameEngine {
public:
    static std::shared_ptr<GameEngine> getGameEngine();
    virtual ~GameEngine();
    void reset(ANativeWindow *newWindow, AAssetManager *newManager);
    void init(ANativeWindow *newWindow, AAssetManager *newManager);
    void cleanup();
    void run(android_app* app);
private:
    GameEngine();

private:
    static std::shared_ptr<GameEngine> s_GameEngine;

    std::unique_ptr<AAssetManager, AAssetManagerDeleter> m_AssetManager;

    std::shared_ptr<IGfxDevice> m_GfxDevice;
    std::shared_ptr<Renderer> m_Renderer;
    std::shared_ptr<FileManager> m_FileManager;
    std::shared_ptr<EventHandler> m_EventHandler;
    static std::string m_TAG;
};
