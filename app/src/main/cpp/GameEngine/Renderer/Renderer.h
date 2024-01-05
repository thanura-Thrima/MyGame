#pragma once
#include <memory>
#include <string>

#include <android/native_window.h>

#include "../GFX/IGfxDevice.h"
#include "../Utils/Definitions.h"

struct ANativeWindowDeleter {
    void operator()(ANativeWindow *window)
    {
        ANativeWindow_release(window);
    }
};

class Renderer {
public:
    enum class BackEnd:int{
        vulkan,
        opegl_es
    };

    Renderer();
    virtual ~Renderer();

    void init(BackEnd backend,ANativeWindow* window);
    void shutdown();

    void beginFrame();
    void endFrame();

    void resize(ANativeWindow *newWindow);

private:
    static std::string toStringBackend(BackEnd backend);
private:
    std::shared_ptr<IGfxDevice> m_Device;
    std::unique_ptr<ANativeWindow, ANativeWindowDeleter> m_MainWindow;
    static std::string m_TAG;
};
