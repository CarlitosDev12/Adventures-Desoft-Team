cmake_minimum_required(VERSION 3.22.1)
project("terminale")

# Crea una librería compartida llamada "terminale" a partir de tu main.cpp
add_library(
    terminale
    SHARED
    main.cpp
)

# Encuentra e incluye las librerías nativas del NDK de Android (OpenGL ES, EGL, Logs y Android nativo)
find_library(log-lib log)
find_library(glesv3-lib GLESv3)
find_library(egl-lib EGL)
find_library(android-lib android)

# Enlaza todo para que funcione correctamente en el APK final
target_link_libraries(
    terminale
    ${log-lib}
    ${glesv3-lib}
    ${egl-lib}
    ${android-lib}
)

