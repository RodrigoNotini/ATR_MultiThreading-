
add_executable(io_entrada io_entrada.cpp )
target_link_libraries(io_entrada PRIVATE atr_common)
if (WIN32 AND ATR_USE_WIN32)
    target_compile_definitions(io_entrada PRIVATE ATR_WIN32)
endif()
