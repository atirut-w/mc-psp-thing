cmake_minimum_required(VERSION 3.30)
project(GLTest)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(3rd/json)

add_executable(gltest.elf
    src/main.cpp
    src/model.cpp
    src/texture_manager.cpp
    src/renderer.cpp
    src/font.cpp
)
target_include_directories(gltest.elf PRIVATE
    include
)
target_link_libraries(gltest.elf PRIVATE
    GL
    glut
    pspgu
    pspge
    pspdisplay
    pspvfpu
    pspctrl
    png
    z
    nlohmann_json
)

add_custom_target(fixup-imports
    ALL
    DEPENDS gltest.elf
    COMMAND psp-fixup-imports gltest.elf
)
