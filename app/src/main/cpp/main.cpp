#include <android/native_activity.h>
#include <thread>
#include <atomic>
#include "RenderVulkan.h"

// Variables globales de control para el ciclo de vida de Android
std::atomic<bool> g_ejecutando{false};
std::thread g_hiloRender;
RendererVulkan g_renderer;

void iniciarRenderizado(ANativeWindow* ventana) {
    if (g_ejecutando) return; // Evita iniciar el hilo dos veces
    g_ejecutando = true;
    
    g_hiloRender = std::thread([ventana]() {
        g_renderer.inicializar(ventana);
        
        // Bucle de renderizado seguro
        while (g_ejecutando) {
            g_renderer.dibujarFrame();
        }
        
        // Limpiamos los recursos de Vulkan al salir del bucle
        g_renderer.limpiar();
    });
}

void detenerRenderizado() {
    if (!g_ejecutando) return;
    
    // 1. Apagamos la bandera para que el hilo salga del while
    g_ejecutando = false;
    
    // 2. Esperamos a que el hilo termine de forma ordenada
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
