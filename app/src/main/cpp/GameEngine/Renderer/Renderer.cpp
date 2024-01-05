//
// Created by varjo on 1/4/2024.
//
#include <exception>

#include "Renderer.h"
#include "../GFX/vulkan/GfxDevice.h"

std::string Renderer::m_TAG = "Renderer";

std::string Renderer::toStringBackend(BackEnd backend)
{
    switch(backend){
        case BackEnd::vulkan: return "vulkan";
        case BackEnd::opegl_es: return "Opengl_es";
        default: return std::to_string(static_cast<int>(backend));
    }
}

void Renderer::init(Renderer::BackEnd backend, ANativeWindow *window)
{
    m_MainWindow.reset(window);

    switch (backend) {
        case BackEnd::vulkan:{
            DeviceConfig config{};
            m_Device = std::make_shared<GfxDevice>(config);
            m_Device->createSurface(m_MainWindow.get());
            m_Device->init();
            break;
        }
        case BackEnd::opegl_es:{
            throw std::runtime_error("Opengl es not supported currently");
        }
        default:{
            LOGE(m_TAG,"Could not find Renderer backend %s",Renderer::toStringBackend(backend).c_str());
            throw std::runtime_error("unsupported backend");
        }
    }
}

void Renderer::resize(ANativeWindow *newWindow)
{
    if (m_Device->isInitialized()) {
        m_Device->createSurface(newWindow);
        m_Device->reCreateSwapchain();
    }else{
        throw std::runtime_error("TODO implementation for non init resize");
    }
}

void Renderer::shutdown() {
    m_Device->deInit();
}

