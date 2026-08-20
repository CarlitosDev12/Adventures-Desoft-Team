cmake_minimum_required(VERSION 3.22.1)
project("adventures")

# Declaramos nuestra librería compartida e incluimos los dos archivos fuente
add_library(
    adventures
    SHARED
    main.cpp
    RenderVulkan.cpp
)

# Forzamos a que el enlazador reconozca el punto de entrada de Android (ANativeActivity)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# Encontramos las librerías nativas del sistema que Android y Vulkan necesitan
find_library(log-lib log)
find_library(vulkan-lib vulkan)
find_library(android-lib android)

# Vinculamos las librerías con nuestro proyecto
target_link_libraries(
    adventures
    ${log-lib}
    ${vulkan-lib}
    ${android-lib}
)
