//
// Created by varjo on 11/25/2023.
//

#ifndef MYGAME_GAMEENGINE_H
#define MYGAME_GAMEENGINE_H
#include <memory>
#include <android/asset_manager.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "GFX/GfxDevice.h"

struct ANativeWindowDeleter {
    void operator()(ANativeWindow *window)
    {
        ANativeWindow_release(window);
    }
};
class GameEngine {
public:
    static std::shared_ptr<GameEngine> getCameEngine();
    virtual ~GameEngine();
    void reset(ANativeWindow *newWindow, AAssetManager *newManager);
    void init();
    void cleanup();
private:
    GameEngine();

private:
    static std::shared_ptr<GameEngine> s_GameEngine;
    std::unique_ptr<ANativeWindow, ANativeWindowDeleter> m_MainWindow;
    AAssetManager * p_AssetManager;

    std::shared_ptr<GfxDevice> m_GfxDevice;
};


#endif //MYGAME_GAMEENGINE_H