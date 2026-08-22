cmake_minimum_required(VERSION 3.22.1)
project("adventures")

# Declaramos nuestra librería compartida con los archivos fuente
add_library(
    adventures
    SHARED
    main.cpp
    RenderVulkan.cpp
)

# Incluimos el directorio del código de native_app_glue que viene con el NDK
# (Android Studio lo enlaza automáticamente si usamos la variable o el target de app glue)
add_library(app-glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_include_directories(adventures PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
)

# Encontramos las librerías nativas del sistema
find_library(log-lib log)
find_library(vulkan-lib vulkan)
find_library(android-lib android)

# Vinculamos todas las librerías, incluyendo app-glue
target_link_libraries(
    adventures
    app-glue
    ${log-lib}
    ${vulkan-lib}
    ${android-lib}
)

