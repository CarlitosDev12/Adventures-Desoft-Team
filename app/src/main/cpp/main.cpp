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

// Estructura contenedora para gestionar el estado y el hilo de la App
struct EstadoApp {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    
    // Control del hilo de renderizado
    std::atomic<bool> ejecutando;
    std::thread hiloRender;
    ANativeWindow* ventanaActual;
};

// 1. FUNCIÓN PARA ENLAZAR LA TARJETA DE VIDEO (EGL)
void inicializarGráficos(ANativeWindow* ventana, EstadoApp* estado) {
    LOGI("Configurando EGL para pantalla completa en hilo independiente...");
    
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
      EGL_CONTEXT_CLIENT_VERSION, 3, // OpenGL ES 3.0
      EGL_NONE
    };
    estado->context = eglCreateContext(estado->display, config, EGL_NO_CONTEXT, contextoAtributos);

    // Activar el contexto gráfico en este hilo específico
    eglMakeCurrent(estado->display, estado->surface, estado->surface, estado->context);
    eglSwapInterval(estado->display, 1); // Sincronización VSync (60Hz / 120Hz)
    
    LOGI("¡OpenGL ES 3.0 activado con éxito en el hilo de renderizado!");
}

// 3. LIBERAR MEMORIA AL SALIR
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

// 2. EL BUCLE DE RENDERIZADO (Corre en su propio hilo de forma fluida)
void bucleDeRenderizado(EstadoApp* estado) {
    LOGI("Hilo de renderizado lanzado.");
    
    // Inicializamos EGL de forma segura dentro del hilo dedicado
    inicializarGráficos(estado->ventanaActual, estado);

    // El Loop de verdad: Se ejecuta de manera continua y eficiente
    while (estado->ejecutando) {
        // --- AQUÍ IRÁ TU LÓGICA DE DIBUJO Y JUEGO ---
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // eglSwapBuffers pausa inteligentemente este hilo hasta el siguiente ciclo de VSync
        eglSwapBuffers(estado->display, estado->surface);
    }

    // Limpieza al salir del loop
    limpiarRecursos(estado);
    LOGI("Hilo de renderizado finalizado correctamente.");
}

// ====================================================================
// CALLBACKS DEL CICLO DE VIDA DE ANDROID
// ====================================================================

void onNativeWindowCreated(ANativeActivity* actividad, ANativeWindow* ventana) {
    EstadoApp* estado = (EstadoApp*)actividad->instance;
    
    estado->ventanaActual = ventana;
    estado->ejecutando = true;
    
    // Lanzamos el hilo de renderizado en paralelo al hilo principal de Android
    estado->hiloRender = std::thread(bucleDeRenderizado, estado);
}

void onNativeWindowDestroyed(ANativeActivity* actividad, ANativeWindow* ventana) {
    EstadoApp* estado = (EstadoApp*)actividad->instance;
    
    if (estado->ejecutando) {
        // Ordenamos detener el bucle
        estado->ejecutando = false;
        
        // Esperamos a que el hilo termine de forma limpia antes de destruir la ventana
        if (estado->hiloRender.joinable()) {
            estado->hiloRender.join();
        }
    }
    estado->ventanaActual = nullptr;
}

// ====================================================================
// CALLBACKS DE ENTRADA (Eventos táctiles)
// ====================================================================
static int loopCallback(int fd, int events, void* data) {
    AInputQueue* queue = static_cast<AInputQueue*>(data);
    AInputEvent* evento = nullptr;

    while (AInputQueue_getEvent(queue, &evento) >= 0) {
        if (AInputQueue_preDispatchEvent(queue, evento)) {
            continue;
        }
        int handled = 0;
        // Aquí puedes procesar toques si lo deseas en el futuro
        AInputQueue_finishEvent(queue, evento, handled);
    }
    return 1; 
}

void onInputQueueCreated(ANativeActivity* actividad, AInputQueue* queue) {
    LOGI("Cola de eventos de entrada creada.");
    AInputQueue_attachLooper(queue, ALooper_forThread(), ALOOPER_POLL_CALLBACK, loopCallback, queue);
}

void onInputQueueDestroyed(ANativeActivity* actividad, AInputQueue* queue) {
    LOGI("Cola de eventos de entrada destruida.");
    AInputQueue_detachLooper(queue);
}

// ====================================================================
// PUNTO DE ENTRADA PRINCIPAL
// ====================================================================
void ANativeActivity_onCreate(ANativeActivity* actividad, void* savedState, size_t savedStateSize) {
    LOGI("Iniciando actividad nativa de Android...");

    EstadoApp* estado = (EstadoApp*)malloc(sizeof(EstadoApp));
    estado->display = EGL_NO_DISPLAY;
    estado->surface = EGL_NO_SURFACE;
    estado->context = EGL_NO_CONTEXT;
    estado->ejecutando = false;
    estado->ventanaActual = nullptr;

    actividad->instance = estado;

    actividad->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    actividad->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    actividad->callbacks->onInputQueueCreated = onInputQueueCreated;
    actividad->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
}
