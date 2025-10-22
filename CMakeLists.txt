cmake_minimum_required(VERSION 3.20)
project(ATR_FaseA LANGUAGES CXX)

# Padrão C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# (Opcional) flags úteis no MSVC
if(MSVC)
  add_definitions(-DUNICODE -D_UNICODE)
  add_compile_options(/W4 /permissive-)
endif()

# ===== Saídas unificadas =====
# Multi-config (Visual Studio): coloca tudo em out/build/bin/<Config>  e out/build/lib/<Config>
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_CONFIGURATION_TYPES) # VS, Xcode…
  foreach(cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${cfg}" cfgU)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${cfgU} ${CMAKE_BINARY_DIR}/lib/${cfg})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfgU} ${CMAKE_BINARY_DIR}/bin/${cfg})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfgU} ${CMAKE_BINARY_DIR}/bin/${cfg})
  endforeach()
else() # single-config (Ninja/Makefiles) – útil se você trocar de gerador
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  endif()
endif()

# (Opcional) organizar targets por pastas no VS
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Opções do projeto
option(ATR_USE_WIN32 "Build with Win32 definitions" ON)

# Subprojetos
add_subdirectory(common)
add_subdirectory(launcher)
add_subdirectory(teclado)
add_subdirectory(io_entrada)
add_subdirectory(captura)
add_subdirectory(exibicao)

