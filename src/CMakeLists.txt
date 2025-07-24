set(IMGUI_SOURCE_DIR ../third_party/imgui)
set(IMGUI_SOURCE_FILES 
        ${IMGUI_SOURCE_DIR}/imgui.cpp 
        ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
        ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
        ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
)

set(IMGUIFILEDIALOG_SOURCE_DIR ../third_party/ImGuiFileDialog)
set(IMGUIFILEDIALOG_SOURCE_FILES 
        ${IMGUIFILEDIALOG_SOURCE_DIR}/ImGuiFileDialog.cpp 
)

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

find_library(OpenGL_LIBS OpenGL)
set(IMGUI_LIBS ${OpenGL_LIBS} ${FREETYPE_LIBRARIES} sdl3)

#find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
find_library(APPKIT_FRAMEWORK AppKit)
find_library(SECURITY_FRAMEWORK Security)

set(TARGET_SOURCE_FILES main.cpp
        llm.cpp 
        server.cpp 
        tools.cpp 
        ${IMGUI_SOURCE_FILES}
        ${IMGUIFILEDIALOG_SOURCE_FILES}
)
add_executable(chat-llm ${TARGET_SOURCE_FILES})

target_link_libraries(chat-llm 
        boost_program_options
        boost_process
        ${IMGUI_LIBS}
        crypto
        ssl
        pdfium
        ${APPKIT_FRAMEWORK}
        ${SECURITY_FRAMEWORK}
)
