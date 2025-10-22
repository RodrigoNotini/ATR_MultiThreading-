
add_library(atr_common
    src/globals.cpp
    src/utils.cpp
    src/messages.cpp
)

target_include_directories(atr_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (WIN32 AND ATR_USE_WIN32)
    target_compile_definitions(atr_common PUBLIC ATR_WIN32)
endif()
