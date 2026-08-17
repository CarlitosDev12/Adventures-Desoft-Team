#include <android/native_activity.h>
#include <android/looper.h>
#include <android/input.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <stdlib.h>
#include <chrono>

#define LOG_TAG "AndroidOpenGL"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Estructura contenedora para gestionar el estado gráfico de la App
struct EstadoApp {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    bool activa;
};

// 1. FUNCIÓN PARA ENLAZAR LA TARJETA DE VIDEO (EGL)
void inicializarGráficos(ANativeWindow* ventana, EstadoApp* estado) {
    LOGI("Configurando EGL para pantalla completa...");
    
    estado->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(estado->display, nullptr, nullptr);

    // Definir formato de color nativo del celular (RGBA de 8 bits por canal)
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

    // Forzar a que el buffer se adapte al tamaño real de la pantalla del móvil
    eglMakeCurrent(estado->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    // CORRECCIÓN: Casteo explícito a EGLNativeWindowType
    estado->surface = eglCreateWindowSurface(estado->display, config, (EGLNativeWindowType)ventana, nullptr);
    const EGLint contextoAtributos[] = {
      EGL_CONTEXT_CLIENT_VERSION, 3, // OpenGL ES 3.0
      EGL_NONE
    };
    estado->context = eglCreateContext(estado->display, config, EGL_NO_CONTEXT, contextoAtributos);

    // Activar el lienzo en el hilo de la aplicación
    eglMakeCurrent(estado->display, estado->surface, estado->surface, estado->context);
    eglSwapInterval(estado->display, 1);
    LOGI("¡OpenGL ES 3.0 activado con éxito a pantalla completa!");
}

// 2. FUNCIÓN DE RENDERIZADO (El motor gráfico)
void dibujarPantallaBlanca(EstadoApp* estado) {
    if (estado->display == EGL_NO_DISPLAY || estado->surface == EGL_NO_SURFACE) return;

    // Comando de OpenGL puro: Definir color de limpieza a BLANCO absoluto (1.0f)
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Limpiar el lienzo aplicando el color blanco
    glClear(GL_COLOR_BUFFER_BIT);

    // Intercambiar los buffers de video para mostrar el cuadro en el teléfono
    eglSwapBuffers(estado->display, estado->surface);
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
}

// Bucle activo que procesa los eventos del sistema y dibuja a 60 FPS
void procesarEventosYDibujar(ANativeActivity* actividad) {
    EstadoApp* estado = (EstadoApp*)actividad->instance;
    
    // 1. Procesar eventos de la cola sin bloquear (timeout = 0)
    int ident;
    int events;
    void* source;
    
    // Esto lee los eventos táctiles y del sistema al vuelo para evitar el ANR
    while ((ident = ALooper_pollAll(0, nullptr, &events, &source)) >= 0) {
        if (ident == ALOOPER_POLL_CALLBACK) {
            // Manejo de callbacks si los hubiera
        }
    }

    // 2. Renderizado a 60 FPS si la app está activa
    if (estado && estado->activa) {
        static auto ultimoTiempo = std::chrono::high_resolution_clock::now();
        
        auto ahora = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> duracion = ahora - ultimoTiempo;

        if (duracion.count() >= 16.6f) {
            dibujarPantallaBlanca(estado);
            ultimoTiempo = ahora; 
        }
    }
}

// ====================================================================
// CALLBACKS DEL CICLO DE VIDA DE ANDROID
// ====================================================================

// Se ejecuta cuando Android le asigna una ventana física a la aplicación
void onNativeWindowCreated(ANativeActivity* actividad, ANativeWindow* ventana) {
    EstadoApp* estado = (EstadoApp*)actividad->instance;
    inicializarGráficos(ventana, estado);
    estado->activa = true;
    
    // Dibujar el cuadro blanco inmediatamente al abrirse
    dibujarPantallaBlanca(estado);
}

// Se ejecuta si el usuario minimiza el juego o bloquea el celular
void onNativeWindowDestroyed(ANativeActivity* actividad, ANativeWindow* ventana) {
    EstadoApp* estado = (EstadoApp*)actividad->instance;
    estado->activa = false;
    limpiarRecursos(estado);
}

// ====================================================================
// CALLBACKS DE ENTRADA Y BUCLE DE EVENTOS (Corregido para evitar ANR)
// ====================================================================
static int loopCallback(int fd, int events, void* data) {
    ANativeActivity* actividad = static_cast<ANativeActivity*>(data);
    
    // LÍNEA MODIFICADA: Llamada limpia sin variables inexistentes
    procesarEventosYDibujar(actividad);
    
    return 1; // Mantener activo el looper
}

void onInputQueueCreated(ANativeActivity* actividad, AInputQueue* queue) {
    LOGI("Cola de eventos de entrada creada correctamente.");
    // Esto conecta la cola al Looper del hilo principal para evitar el timeout de entrada
    AInputQueue_attachLooper(queue, ALooper_forThread(), ALOOPER_POLL_CALLBACK, loopCallback, actividad);
}

void onInputQueueDestroyed(ANativeActivity* actividad, AInputQueue* queue) {
    LOGI("Cola de eventos de entrada destruida.");
    AInputQueue_detachLooper(queue);
}

// ====================================================================
// PUNTO DE ENTRADA PRINCIPAL DE LA APP DE ANDROID
// ====================================================================
void ANativeActivity_onCreate(ANativeActivity* actividad, void* savedState, size_t savedStateSize) {
    LOGI("Iniciando ciclo de vida nativo de la aplicacion...");

    // Asignar memoria para guardar nuestro estado gráfico
    EstadoApp* estado = (EstadoApp*)malloc(sizeof(EstadoApp));
    estado->display = EGL_NO_DISPLAY;
    estado->surface = EGL_NO_SURFACE;
    estado->context = EGL_NO_CONTEXT;
    estado->activa = false;

    // Guardar la estructura dentro de la instancia de la actividad de Android
    actividad->instance = estado;

    // Registrar los eventos obligatorios que escuchará nuestro código C++
    actividad->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    actividad->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    actividad->callbacks->onInputQueueCreated = onInputQueueCreated;
    actividad->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
}
