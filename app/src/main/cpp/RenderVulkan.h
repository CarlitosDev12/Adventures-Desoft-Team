#pragma once
#include <android/native_window.h>
#include <vulkan/vulkan.h>
#include <vector>

class RendererVulkan {
public:
    void inicializar(ANativeWindow* ventana);
    void dibujarFrame();
    void limpiar();

private:
    VkInstance instancia = VK_NULL_HANDLE;
    VkPhysicalDevice dispositivoFisico = VK_NULL_HANDLE;
    VkDevice dispositivoLogico = VK_NULL_HANDLE;
    VkSurfaceKHR superficie = VK_NULL_HANDLE;
    VkQueue colaGrafica = VK_NULL_HANDLE;
    
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;
    VkExtent2D swapchainExtent = {};
};
