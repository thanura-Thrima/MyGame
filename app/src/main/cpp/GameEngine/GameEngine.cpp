#include <android/log.h>

#include "GameEngine.h"
#include "Utils/Definitions.h"
#include "GFX/GfxDevice.h"

std::shared_ptr<GameEngine> GameEngine::s_GameEngine = nullptr;
GameEngine::~GameEngine() {
 LOGD(__func__,"Destructor");
}

GameEngine::GameEngine() {
    LOGD(__func__,"Constructor");
}
std::shared_ptr<GameEngine> GameEngine::getGameEngine() {
    if (s_GameEngine == nullptr)
    {
        s_GameEngine = std::shared_ptr<GameEngine>(new GameEngine());
    }
    return s_GameEngine;
}

void GameEngine::reset(ANativeWindow *newWindow, AAssetManager *newManager)
{
    m_MainWindow.reset(newWindow);
    p_AssetManager = newManager;

    if (m_GfxDevice->isInitialized()) {
        m_GfxDevice->createSurface(newWindow);
        m_GfxDevice->reCreateSwapchain();
    }
}

void GameEngine::init(ANativeWindow *newWindow, AAssetManager *newManager)
{
    m_MainWindow.reset(newWindow);
    p_AssetManager = newManager;

}

void GameEngine::cleanup()
{

}