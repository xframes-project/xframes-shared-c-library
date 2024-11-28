cmake_minimum_required(VERSION 3.8)

set(CMAKE_BUILD_TYPE Release FORCE)

set(CMAKE_GENERATOR "Ninja")
set(VCPKG_ROOT "./vcpkg/")
set(CMAKE_TOOLCHAIN_FILE "./vcpkg/scripts/buildsystems/vcpkg.cmake")

set(VCPKG_TARGET_ARCHITECTURE "x64")

# Project name and language
project(xframesshared LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

# Specify the library version
set(MYLIBRARY_VERSION_MAJOR 0)
set(MYLIBRARY_VERSION_MINOR 0)
set(MYLIBRARY_VERSION_PATCH 1)

# Enable position-independent code for shared libraries
# set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT VCPKG_TARGET_TRIPLET)
    if(WIN32)
        set(VCPKG_TARGET_TRIPLET "x64-windows")
    elseif(APPLE)
        set(VCPKG_TARGET_TRIPLET "x64-osx")
    elseif(DEFINED ENV{ARM64_LINUX})
        set(VCPKG_TARGET_TRIPLET "arm64-linux")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-linux")
    endif()
endif()

set(DEPS "./xframes/packages/dear-imgui/cpp/deps")
set(APP "./xframes/packages/dear-imgui/cpp/app")

file(GLOB YOGA_SRC CONFIGURE_DEPENDS
        ${DEPS}/yoga/yoga/*.cpp
        ${DEPS}/yoga/yoga/**/*.cpp)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(ada CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Source files
set(SOURCES
    ${DEPS}/css-color-parser-cpp/csscolorparser.hpp
    ${DEPS}/css-color-parser-cpp/csscolorparser.cpp

    ${DEPS}/imgui/imgui.cpp
    ${DEPS}/imgui/imgui_draw.cpp
    ${DEPS}/imgui/imgui_widgets.cpp
    ${DEPS}/imgui/imgui_tables.cpp
    ${DEPS}/imgui/backends/imgui_impl_opengl3.cpp
    ${DEPS}/imgui/backends/imgui_impl_glfw.cpp

    ${DEPS}/implot/implot.cpp
    ${DEPS}/implot/implot_items.cpp

    ${YOGA_SRC}

    ${APP}/src/shared.cpp
    ${APP}/src/color_helpers.cpp
    ${APP}/src/yoga_helpers.cpp
    ${APP}/src/imgui_helpers.cpp

    ${APP}/src/element/layout_node.cpp
    ${APP}/src/element/element.cpp

    ${APP}/src/widget/widget.cpp
    ${APP}/src/widget/styled_widget.cpp

    ${APP}/src/xframes.cpp
    ${APP}/src/imgui_renderer.cpp
    ${APP}/src/implot_renderer.cpp

    ${APP}/src/widget/button.cpp
    ${APP}/src/widget/checkbox.cpp
    ${APP}/src/widget/child.cpp
    ${APP}/src/widget/clipped_multi_line_text_renderer.cpp
    ${APP}/src/widget/collapsing_header.cpp
    ${APP}/src/widget/combo.cpp
    ${APP}/src/widget/group.cpp
    ${APP}/src/widget/image.cpp
    ${APP}/src/widget/input_text.cpp
    ${APP}/src/widget/item_tooltip.cpp
    ${APP}/src/widget/multi_slider.cpp
    ${APP}/src/widget/plot_candlestick.cpp
    ${APP}/src/widget/plot_line.cpp
    ${APP}/src/widget/separator.cpp
    ${APP}/src/widget/separator_text.cpp
    ${APP}/src/widget/slider.cpp
    ${APP}/src/widget/table.cpp
    ${APP}/src/widget/tabs.cpp
    ${APP}/src/widget/text.cpp
    ${APP}/src/widget/text_wrap.cpp
    ${APP}/src/widget/tree_node.cpp
    ${APP}/src/widget/window.cpp

    src/xframes-runner.cpp
    src/xframes-wrapper.cpp
)

# Create a shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${APP}/include

        ${Stb_INCLUDE_DIR}

        ${DEPS}/IconFontCppHeaders
        ${DEPS}/ReactivePlusPlus/src/rpp
        ${DEPS}/css-color-parser-cpp
        ${DEPS}/imgui
        ${DEPS}/imgui/backends
        ${DEPS}/implot
        ${DEPS}/json/include
        ${DEPS}/yoga

        ./src
)

# Set the output name of the library
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${MYLIBRARY_VERSION_MAJOR}.${MYLIBRARY_VERSION_MINOR}.${MYLIBRARY_VERSION_PATCH}
    SOVERSION ${MYLIBRARY_VERSION_MAJOR}
    OUTPUT_NAME "xframesshared"
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        ada::ada
        fmt::fmt
        glfw
        OpenGL::GL
)

# Export C symbols for shared library
# target_compile_definitions(${PROJECT_NAME} PRIVATE EXPORT_API=__declspec(dllexport))  # For Windows
# if(UNIX)
    # target_compile_definitions(${PROJECT_NAME} PRIVATE EXPORT_API=__attribute__((visibility("default"))))
# endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
