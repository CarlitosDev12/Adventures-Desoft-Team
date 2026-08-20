#include <android/native_activity.h>
#include <thread>
#include <atomic>
#include "RendererVulkan.h"

// Variables globales de control para el ciclo de vida de Android
std::atomic<bool> g_ejecutando{false};
std::thread g_hiloRender;
RendererVulkan g_renderer;

void iniciarRenderizado(ANativeWindow* ventana) {
    g_ejecutando = true;
    g_hiloRender = std::thread([ventana]() {
        g_renderer.inicializar(ventana);
        while (g_ejecutando) {
            g_renderer.dibujarFrame();
        }
        g_renderer.limpiar();
    });
}

void detenerRenderizado() {
    g_ejecutando = false;
    if (g_hiloRender.joinable()) {
        g_hiloRender.join();
    }
}

void onWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
    iniciarRenderizado(window);
}

void onWindowDestroyed(ANativeActivity* activity, ANativeWindow* window) {
    detenerRenderizado();
}

void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize) {
    activity->callbacks->onNativeWindowCreated = onWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = onWindowDestroyed;
}
