
add_executable(exibicao exibicao.cpp )
target_link_libraries(exibicao PRIVATE atr_common)
if (WIN32 AND ATR_USE_WIN32)
    target_compile_definitions(exibicao PRIVATE ATR_WIN32)
endif()
