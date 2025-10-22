
add_executable(launcher launcher.cpp )
target_link_libraries(launcher PRIVATE atr_common)
if (WIN32 AND ATR_USE_WIN32)
    target_compile_definitions(launcher PRIVATE ATR_WIN32)
endif()
