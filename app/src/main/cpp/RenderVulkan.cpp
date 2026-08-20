#include "RenderVulkan.h"

void RendererVulkan::inicializar(ANativeWindow* ventana) {
    // 1. Instancia de Vulkan
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
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

    vkCreateInstance(&createInfo, nullptr, &instancia);

    // 2. Superficie de la ventana de Android
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = ventana;
    vkCreateAndroidSurfaceKHR(instancia, &surfaceCreateInfo, nullptr, &superficie);

    // 3. Dispositivo físico (GPU)
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instancia, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instancia, &deviceCount, devices.data());
    dispositivoFisico = devices[0];

    // 4. Buscar la familia de colas gráficas
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dispositivoFisico, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dispositivoFisico, &queueFamilyCount, queueFamilies.data());

    queueFamilyIndex = 0;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndex = i;
            break;
        }
    }

    // 5. Dispositivo lógico
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    vkCreateDevice(dispositivoFisico, &deviceCreateInfo, nullptr, &dispositivoLogico);
    vkGetDeviceQueue(dispositivoLogico, queueFamilyIndex, 0, &colaGrafica);

    // 6. Dimensiones de la pantalla
    int32_t ancho = ANativeWindow_getWidth(ventana);
    int32_t alto = ANativeWindow_getHeight(ventana);
    swapchainExtent = { (uint32_t)ancho, (uint32_t)alto };
    VkFormat swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    // 7. Swapchain (Gestor de imágenes de pantalla)
    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = superficie;
    swapchainInfo.minImageCount = 2;
    swapchainInfo.imageFormat = swapchainImageFormat;
    swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageExtent = swapchainExtent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;

    vkCreateSwapchainKHR(dispositivoLogico, &swapchainInfo, nullptr, &swapchain);

    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(dispositivoLogico, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(dispositivoLogico, swapchain, &imageCount, swapchainImages.data());

    // 8. Image Views
    swapchainImageViews.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(dispositivoLogico, &viewInfo, nullptr, &swapchainImageViews[i]);
    }

    // 9. Render Pass (Configurado para limpiar la pantalla)
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    vkCreateRenderPass(dispositivoLogico, &renderPassInfo, nullptr, &renderPass);

    // 10. Framebuffers
    framebuffers.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++) {
        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &swapchainImageViews[i];
        fbInfo.width = swapchainExtent.width;
        fbInfo.height = swapchainExtent.height;
        fbInfo.layers = 1;

        vkCreateFramebuffer(dispositivoLogico, &fbInfo, nullptr, &framebuffers[i]);
    }

    // 11. Command Pool y Buffer
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    vkCreateCommandPool(dispositivoLogico, &poolInfo, nullptr, &commandPool);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(dispositivoLogico, &allocInfo, &commandBuffer);
}

void RendererVulkan::dibujarFrame() {
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(dispositivoLogico, swapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) return;

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Color BLANCO total (R=1, G=1, B=1, A=1)
    VkClearValue clearValue = {};
    clearValue.color = { {1.0f, 1.0f, 1.0f, 1.0f} };

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapchainExtent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    // Inicia el pase de renderizado que pinta de blanco y termina inmediatamente
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(colaGrafica, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(colaGrafica);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(colaGrafica, &presentInfo);
}

void RendererVulkan::limpiar() {
    if (dispositivoLogico != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(dispositivoLogico);
        if (commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(dispositivoLogico, commandPool, nullptr);
        for (auto fb : framebuffers) vkDestroyFramebuffer(dispositivoLogico, fb, nullptr);
        if (renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(dispositivoLogico, renderPass, nullptr);
        for (auto view : swapchainImageViews) vkDestroyImageView(dispositivoLogico, view, nullptr);
        if (swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(dispositivoLogico, swapchain, nullptr);
        vkDestroyDevice(dispositivoLogico, nullptr);
    }
    if (superficie != VK_NULL_HANDLE && instancia != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instancia, superficie, nullptr);
    }
    if (instancia != VK_NULL_HANDLE) {
        vkDestroyInstance(instancia, nullptr);
    }
}

