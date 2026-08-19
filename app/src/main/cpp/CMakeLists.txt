cmake_minimum_required(VERSION 3.22.1)
project("adventures")

# Añadimos main.cpp y los archivos de vk-bootstrap a la compilación
add_library(
    adventures
    SHARED
    main.cpp
    VkBootstrap.cpp # Asegúrate de colocar la librería en tu carpeta del proyecto
)

# Forzar al enlazador a conservar el punto de entrada de NativeActivity
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# Encontrar las librerías nativas del NDK necesarias para Vulkan y Android
find_library(log-lib log)
find_library(vulkan-lib vulkan)     # <-- Reemplazamos GLESv3 y EGL por Vulkan
find_library(android-lib android)

# Enlazar las librerías con tu proyecto
target_link_libraries(
    adventures
    ${log-lib}
    ${vulkan-lib}    # <-- Enlazamos Vulkan en lugar de OpenGL
    ${android-lib}
)
