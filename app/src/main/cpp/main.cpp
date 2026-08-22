#include <jni.h>
#include <android_native_app_glue.h>
#include "RenderVulkan.h"

// Estructura para manejar el estado de la app si lo necesitas
struct EstadoApp {
    RendererVulkan renderer;
    bool ventanaCreada = false;
};

// Callback para procesar los eventos de Android (toques, ciclo de vida, ventana)
static void procesarComandoApp(struct android_app* app, int32_t cmd) {
    EstadoApp* state = (EstadoApp*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            // La ventana de Android ha sido creada, inicializamos Vulkan
            if (app->window != nullptr) {
                state->renderer.inicializar(app->window);
                state->ventanaCreada = true;
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // La ventana se va a destruir, limpiamos Vulkan
            state->renderer.limpiar();
            state->ventanaCreada = false;
            break;
        case APP_CMD_PAUSE:
            // La app pasa a segundo plano
            break;
        case APP_CMD_RESUME:
            // La app vuelve a primer plano
            break;
    }
}

// Punto de entrada principal requerido por native_app_glue
void android_main(struct android_app* state) {
    EstadoApp appState;
    state->userData = &appState;
    state->onAppCmd = procesarComandoApp;

    int ident;
    int events;
    android_poll_source* source;

    // Bucle principal de la aplicación gestionado de forma segura por Android
    while (true) {
        // ALooper_pollOnce procesa los eventos sin bloquear la CPU infinitamente
        // Si el valor de timeout es 0, corre de forma fluida (modo juego)
        while ((ident = ALooper_pollOnce(appState.ventanaCreada ? 0 : -1, nullptr, &events, (void**)&source)) >= 0) {
            
            if (source != nullptr) {
                source->process(state, source);
            }

            // Comprobamos si la app debe destruirse
            if (state->destroyRequested != 0) {
                appState.renderer.limpiar();
                return;
            }
        }

        // Si la ventana está creada y activa, dibujamos los frames paso a paso
        if (appState.ventanaCreada) {
            appState.renderer.dibujarFrame();
        }
    }
}
