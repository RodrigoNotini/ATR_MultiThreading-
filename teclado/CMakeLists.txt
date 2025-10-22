
add_executable(teclado teclado.cpp)
target_link_libraries(teclado PRIVATE atr_common)
if (WIN32 AND ATR_USE_WIN32)
    target_compile_definitions(teclado PRIVATE ATR_WIN32)
endif()
