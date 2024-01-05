#include <android/log.h>
#include "GameEngine.h"
#include "Utils/Definitions.h"
#include "GFX/IGfxDevice.h"

std::string GameEngine::m_TAG = "GameEngine";
std::shared_ptr<GameEngine> GameEngine::s_GameEngine = nullptr;
GameEngine::~GameEngine() {
 LOGD(m_TAG,__FUNCTION__);
}

GameEngine::GameEngine() {
    LOGD(m_TAG,__FUNCTION__);
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
    m_Renderer->resize(newWindow);
    m_AssetManager.reset(newManager);
}

void GameEngine::init(ANativeWindow *newWindow, AAssetManager *newManager)
{
    m_Renderer->init(Renderer::BackEnd::vulkan,newWindow);
    m_AssetManager.reset(newManager);

}

void GameEngine::cleanup()
{
    m_Renderer->shutdown();
}

void GameEngine::run(android_app *pApp)
{
    pApp->onAppCmd= [](android_app *pApp, int32_t cmd){
        switch (cmd) {
            case APP_CMD_INIT_WINDOW: {
                pApp->userData = GameEngine::getGameEngine().get();
                auto engine = reinterpret_cast<GameEngine *>(pApp->userData);
                engine->init(pApp->window, pApp->activity->assetManager);
                break;
            }
            case APP_CMD_TERM_WINDOW: {
                if (pApp->userData != nullptr) {
                    //
                    auto engine = reinterpret_cast<GameEngine *>(pApp->userData);
                    engine->cleanup();

                }
                break;
            }
            default:
                break;
        }
    };

    while(!pApp->destroyRequested)
    {
        m_EventHandler->handleEvents(pApp);

        //m_
    }
}