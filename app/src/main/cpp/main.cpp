#include <android/native_activity.h>
#include <android/looper.h>
#include <android/input.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <stdlib.h>
#include <thread>
#include <atomic>

#define LOG_TAG "AndroidOpenGL"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct EstadoApp {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    
    std::atomic<bool> ejecutando;
    std::thread hiloRender; // Se gestionará su ciclo de vida manualmente
    ANativeWindow* ventanaActual;
};

void inicializarGráficos(ANativeWindow* ventana, EstadoApp* estado) {
    LOGI("Configurando EGL en hilo independiente...");
    
    estado->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(estado->display, nullptr, nullptr);

    const EGLint atributos[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(estado->display, atributos, &config, 1, &numConfigs);

    estado->surface = eglCreateWindowSurface(estado->display, config, (EGLNativeWindowType)ventana, nullptr);
    
    const EGLint contextoAtributos[] = {
      EGL_CONTEXT_CLIENT_VERSION, 3, 
      EGL_NONE
    };
    estado->context = eglCreateContext(estado->display, config, EGL_NO_CONTEXT, contextoAtributos);

    eglMakeCurrent(estado->display, estado->surface, estado->surface, estado->context);
    eglSwapInterval(estado->display, 1); 
    
    LOGI("¡OpenGL ES 3.0 activado con éxito!");
}

void limpiarRecursos(EstadoApp* estado) {
    if (estado->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(estado->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (estado->context != EGL_NO_CONTEXT) eglDestroyContext(estado->display, estado->context);
        if (estado->surface != EGL_NO_SURFACE) eglDestroySurface(estado->display, estado->surface);
        eglTerminate(estado->display);
    }
    estado->display = EGL_NO_DISPLAY;
    estado->context = EGL_NO_CONTEXT;
    estado->surface = EGL_NO_SURFACE;
    LOGI("Recursos gráficos liberados.");
}

void bucleDeRenderizado(EstadoApp* estado) {
    LOGI("Hilo de renderizado lanzado.");
    
    while (estado->ejecutando && estado->ventanaActual == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!estado->ejecutando) return;

    inicializarGráficos(estado->ventanaActual, estado);

    while (estado->ejecutando) {
        // Renderizado básico (Fondo celeste para verificar cambios)
        glClearColor(66.0f, 152.0f, 245.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        if (estado->display != EGL_NO_DISPLAY && estado->surface != EGL_NO_SURFACE) {
            eglSwapBuffers(estado->display, estado->surface);
        }
    }

    limpiarRecursos(estado);
    LOGI("Hilo de renderizado finalizado limpiamente.");
}

// ====================================================================
// CALLBACKS DEL CICLO DE VIDA
// ====================================================================

void onNativeWindowCreated(ANativeActivity* actividad, ANativeWindow* ventana) {
    LOGI("Ventana creada.");
    EstadoApp* estado = (EstadoApp*)actividad->instance;
    
    estado->ventanaActual = ventana;
    estado->ejecutando = true;
    
    // Asegurar que el objeto thread anterior esté limpio antes de reasignar
    if (estado->hiloRender.joinable()) {
        estado->hiloRender.join();
    }
    
    estado->hiloRender = std::thread(bucleDeRenderizado, estado);
}

void onNativeWindowDestroyed(ANativeActivity* actividad, ANativeWindow* ventana) {
    LOGI("Ventana destruida.");
    EstadoApp* estado = (EstadoApp*)actividad->instance;
    
    if (estado->ejecutando) {
        estado->ejecutando = false;
        if (estado->hiloRender.joinable()) {
            estado->hiloRender.join();
        }
    }
    estado->ventanaActual = nullptr;
}

// Liberación absoluta de memoria al cerrar la app
void onDestroy(ANativeActivity* actividad) {
    LOGI("Destruyendo actividad nativa. Liberando estructuras...");
    EstadoApp* estado = (EstadoApp*)actividad->instance;
    
    if (estado) {
        if (estado->hiloRender.joinable()) {
            estado->ejecutando = false;
            estado->hiloRender.join();
        }
        free(estado);
        actividad->instance = nullptr;
    }
}

// ====================================================================
// EVENTOS DE ENTRADA
// ====================================================================
static int loopCallback(int fd, int events, void* data) {
    AInputQueue* queue = static_cast<AInputQueue*>(data);
    AInputEvent* evento = nullptr;

    while (AInputQueue_getEvent(queue, &evento) >= 0) {
        if (AInputQueue_preDispatchEvent(queue, evento)) {
            continue;
        }
        AInputQueue_finishEvent(queue, evento, 0);
    }
    return 1; 
}

void onInputQueueCreated(ANativeActivity* actividad, AInputQueue* queue) {
    AInputQueue_attachLooper(queue, ALooper_forThread(), ALOOPER_POLL_CALLBACK, loopCallback, queue);
}

void onInputQueueDestroyed(ANativeActivity* actividad, AInputQueue* queue) {
    AInputQueue_detachLooper(queue);
}

// ====================================================================
// PUNTO DE ENTRADA
// ====================================================================
void ANativeActivity_onCreate(ANativeActivity* actividad, void* savedState, size_t savedStateSize) {
    LOGI("Iniciando ANativeActivity...");

    EstadoApp* estado = (EstadoApp*)malloc(sizeof(EstadoApp));
    estado->display = EGL_NO_DISPLAY;
    estado->surface = EGL_NO_SURFACE;
    estado->context = EGL_NO_CONTEXT;
    estado->ejecutando = false;
    estado->ventanaActual = nullptr;
    
    // El constructor por defecto de std::thread no inicializa un hilo activo, es seguro.
    new (&estado->hiloRender) std::thread(); 

    actividad->instance = estado;

    actividad->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    actividad->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    actividad->callbacks->onInputQueueCreated = onInputQueueCreated;
    actividad->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
    actividad->callbacks->onDestroy = onDestroy; // Callback asignado para evitar memory leaks
}
