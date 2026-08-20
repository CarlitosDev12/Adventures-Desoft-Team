#include <android/native_activity.h>
#include <android/looper.h>
#include <android/input.h>
#include <android/log.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <stdlib.h>
#include <thread>
#include <atomic>
#include <vector>

#define LOG_TAG "AndroidVulkanPuro"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct EstadoApp {
    VkInstance instancia;
    VkPhysicalDevice dispositivoFisico;
    VkDevice dispositivoLogico;
    VkSurfaceKHR superficie;
    VkQueue colaGrafica;
    
    std::atomic<bool> ejecutando;
    std::thread hiloRender;
    ANativeWindow* ventanaActual;
};

void inicializarVulkanPuro(ANativeWindow* ventana, EstadoApp* estado) {
    LOGI("Inicializando Vulkan puro en Android...");

    // 1. Crear Instancia de Vulkan con extensiones obligatorias para Android
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "JuegoVulkanPuro";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    const char* extensiones[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = extensiones;

    if (vkCreateInstance(&createInfo, nullptr, &estado->instancia) != VK_SUCCESS) {
        LOGI("¡Error al crear la VkInstance!");
        return;
    }

    // 2. Crear la Superficie de Android para la ventana nativa
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = ventana;

    if (vkCreateAndroidSurfaceKHR(estado->instancia, &surfaceCreateInfo, nullptr, &estado->superficie) != VK_SUCCESS) {
        LOGI("¡Error al crear la VkSurfaceKHR de Android!");
        return;
    }

    // 3. Seleccionar Dispositivo Físico (la GPU del teléfono)
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(estado->instancia, &deviceCount, nullptr);
    if (deviceCount == 0) {
        LOGI("¡No se encontraron GPUs compatibles con Vulkan en este dispositivo!");
        return;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(estado->instancia, &deviceCount, devices.data());
    estado->dispositivoFisico = devices[0]; // Seleccionamos la primera GPU disponible

    // 4. Encontrar una cola gráfica válida
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(estado->dispositivoFisico, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(estado->dispositivoFisico, &queueFamilyCount, queueFamilies.data());

    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamilyIndex = i;
            break;
        }
    }

    // 5. Crear Dispositivo Lógico (VkDevice)
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(estado->dispositivoFisico, &deviceCreateInfo, nullptr, &estado->dispositivoLogico) != VK_SUCCESS) {
        LOGI("¡Error al crear el VkDevice lógico!");
        return;
    }

    vkGetDeviceQueue(estado->dispositivoLogico, graphicsQueueFamilyIndex, 0, &estado->colaGrafica);

    LOGI("¡Vulkan inicializado limpiamente de forma nativa sin librerías externas!");
}

void limpiarRecursos(EstadoApp* estado) {
    LOGI("Liberando recursos de Vulkan...");
    if (estado->dispositivoLogico != VK_NULL_HANDLE) {
        vkDestroyDevice(estado->dispositivoLogico, nullptr);
    }
    if (estado->superficie != VK_NULL_HANDLE && estado->instancia != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(estado->instancia, estado->superficie, nullptr);
    }
    if (estado->instancia != VK_NULL_HANDLE) {
        vkDestroyInstance(estado->instancia, nullptr);
    }
    LOGI("Recursos liberados con éxito.");
}

void bucleDeRenderizado(EstadoApp* estado) {
    LOGI("Hilo de renderizado iniciado.");
    
    while (estado->ejecutando && estado->ventanaActual == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!estado->ejecutando) return;

    inicializarVulkanPuro(estado->ventanaActual, estado);

    while (estado->ejecutando) {
        // Aquí puedes meter la lógica de dibujo por fotograma en el futuro
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    limpiarRecursos(estado);
    LOGI("Hilo de renderizado finalizado.");
}

// ====================================================================
// CALLBACKS DEL CICLO DE VIDA DE ANDROID
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

static int loopCallback(int fd, int events, void* data) {
    AInputQueue* queue = static_cast<AInputQueue*>(data);
    AInputEvent* evento = nullptr;
    while (AInputQueue_getEvent(queue, &evento) >= 0) {
        if (AInputQueue_preDispatchEvent(queue, evento)) continue;
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

void ANativeActivity_onCreate(ANativeActivity* actividad, void* savedState, size_t savedStateSize) {
    LOGI("Iniciando ANativeActivity con Vulkan Puro...");

    EstadoApp* estado = (EstadoApp*)malloc(sizeof(EstadoApp));
    estado->instancia = VK_NULL_HANDLE;
    estado->dispositivoFisico = VK_NULL_HANDLE;
    estado->dispositivoLogico = VK_NULL_HANDLE;
    estado->superficie = VK_NULL_HANDLE;
    estado->colaGrafica = VK_NULL_HANDLE;
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
