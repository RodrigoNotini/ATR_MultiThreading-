
add_executable(captura captura.cpp )
target_link_libraries(captura PRIVATE atr_common)
if (WIN32 AND ATR_USE_WIN32)
    target_compile_definitions(captura PRIVATE ATR_WIN32)
endif()
