#include <android/native_activity.h>
#include <android/looper.h>
#include <android/input.h>
#include <android/log.h>
#include <vulkan/vulkan.h>
#include "VkBootstrap.h" // Nuestra librería auxiliar de Vulkan[span_1](start_span)[span_1](end_span)
#include "VkBootstrapDispatch.h[span_2](start_span)"[span_2](end_span)
// Elimina la línea de VkBootstrapFeatureChain.h
#include <stdlib.h>
#include <thread>
#include <atomic>

#define LOG_TAG "AndroidVulkan"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct EstadoApp {
    // Reemplazamos las variables de EGL por las de Vulkan y vk-bootstrap
    VkInstance instancia;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice dispositivoFisico;
    VkDevice dispositivoLogico;
    VkSurfaceKHR superficie;
    
    std::atomic<bool> ejecutando;
    std::thread hiloRender;
    ANativeWindow* ventanaActual;
};

void inicializarGráficosVulkan(ANativeWindow* ventana, EstadoApp* estado) {
    LOGI("Configurando Vulkan con vk-bootstrap en hilo independiente...");
    
    // 1. Crear Instancia de Vulkan
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("JuegoVulkanNativo")
                           .request_validation_layers(true)
                           .use_default_debug_messenger()
                           .build();
                           
    if (!inst_ret) {
        LOGI("Error al crear la instancia de Vulkan: %s", inst_ret.error().message().c_str());
        return;
    }
    
    vkb::Instance vkb_inst = inst_ret.value();
    estado->instancia = vkb_inst.instance;
    estado->debugMessenger = vkb_inst.debug_messenger;

    // 2. Crear la Superficie de Android para la ventana nativa
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = ventana;

    if (vkCreateAndroidSurfaceKHR(estado->instancia, &surfaceCreateInfo, nullptr, &estado->superficie) != VK_SUCCESS) {
        LOGI("¡Error al crear la superficie Android para Vulkan!");
        return;
    }

    // 3. Seleccionar Dispositivo Físico y crear Dispositivo Lógico con vk-bootstrap
    vkb::PhysicalDeviceSelector phys_device_selector(vkb_inst);
    auto phys_ret = phys_device_selector.set_surface(estado->superficie).select();
    if (!phys_ret) {
        LOGI("Error al seleccionar la tarjeta gráfica: %s", phys_ret.error().message().c_str());
        return;
    }
    
    vkb::PhysicalDevice vkb_physical_device = phys_ret.value();
    estado->dispositivoFisico = vkb_physical_device.physical_device;

    vkb::DeviceBuilder device_builder(vkb_physical_device);
    auto dev_ret = device_builder.build();
    if (!dev_ret) {
        LOGI("Error al crear el dispositivo lógico: %s", dev_ret.error().message().c_str());
        return;
    }
    
    vkb::Device vkb_device = dev_ret.value();
    estado->dispositivoLogico = vkb_device.device;

    LOGI("¡Vulkan inicializado con éxito usando vk-bootstrap!");
}

void limpiarRecursos(EstadoApp* estado) {
    LOGI("Liberando recursos de Vulkan...");
    if (estado->dispositivoLogico != VK_NULL_HANDLE) {
        vkDestroyDevice(estado->dispositivoLogico, nullptr);
    }
    if (estado->superficie != VK_NULL_HANDLE && estado->instancia != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(estado->instancia, estado->superficie, nullptr);
    }
    vkb::destroy_debug_utils_messenger(estado->instancia, estado->debugMessenger);
    if (estado->instancia != VK_NULL_HANDLE) {
        vkDestroyInstance(estado->instancia, nullptr);
    }
    LOGI("Recursos gráficos liberados.");
}

void bucleDeRenderizado(EstadoApp* estado) {
    LOGI("Hilo de renderizado lanzado.");
    
    while (estado->ejecutando && estado->ventanaActual == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!estado->ejecutando) return;

    // Inicializamos Vulkan usando la ventana actual de Android
    inicializarGráficosVulkan(estado->ventanaActual, estado);

    while (estado->ejecutando) {
        // Aquí en el futuro iría la grabación y envío de Command Buffers de Vulkan
        // Por ahora dejamos el hilo corriendo de forma estable
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS simulados
    }

    limpiarRecursos(estado);
    LOGI("Hilo de renderizado finalizado limpiamente.");
}

// ====================================================================
// CALLBACKS DEL CICLO DE VIDA (Idénticos a tu estructura original)
// ====================================================================

void onNativeWindowCreated(ANativeActivity* actividad, ANativeWindow* ventana) {
    LOGI("Ventana creada.");
    EstadoApp* estado = (EstadoApp*)actividad->instance;
    
    estado->ventanaActual = ventana;
    estado->ejecutando = true;
    
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
    LOGI("Iniciando ANativeActivity con Vulkan...");

    EstadoApp* estado = (EstadoApp*)malloc(sizeof(EstadoApp));
    estado->instancia = VK_NULL_HANDLE;
    estado->debugMessenger = VK_NULL_HANDLE;
    estado->dispositivoFisico = VK_NULL_HANDLE;
    estado->dispositivoLogico = VK_NULL_HANDLE;
    estado->superficie = VK_NULL_HANDLE;
    estado->ejecutando = false;
    estado->ventanaActual = nullptr;
    
    new (&estado->hiloRender) std::thread(); 

    actividad->instance = estado;

    actividad->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    actividad->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    actividad->callbacks->onInputQueueCreated = onInputQueueCreated;
    actividad->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
    actividad->callbacks->onDestroy = onDestroy;
}
