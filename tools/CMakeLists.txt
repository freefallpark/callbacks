add_library(CallbackTools INTERFACE)
target_include_directories(CallbackTools INTERFACE ${CMAKE_SOURCE_DIR}/tools)
target_compile_options(CallbackTools INTERFACE -O3)