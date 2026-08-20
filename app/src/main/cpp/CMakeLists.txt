cmake_minimum_required(VERSION 3.22.1)
project("adventures")

# Añadimos únicamente main.cpp a la compilación
add_library(
    adventures
    SHARED
    main.cpp
)

# Forzar al enlazador a conservar el punto de entrada de NativeActivity
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# Encontrar las librerías nativas del NDK necesarias para Vulkan y Android
find_library(log-lib log)
find_library(vulkan-lib vulkan)
find_library(android-lib android)

# Enlazar las librerías con tu proyecto
target_link_libraries(
    adventures
    ${log-lib}
    ${vulkan-lib}
    ${android-lib}
)
