cmake_minimum_required(VERSION 3.10)

project(chat.llm)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(
    third_party/imgui
    third_party/imgui/backends
    third_party/imgui/misc/freetype
    third_party/ImGuiFileDialog
    third_party/openai.cpp/include
    third_party/pdfium/include
    third_party/utfcpp/source
)

link_directories(
    third_party/pdfium/lib
)

add_subdirectory(src)