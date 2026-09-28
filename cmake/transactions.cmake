cmake_minimum_required(VERSION 3.21)
set(XFRAMES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/xframes" CACHE PATH "Current XFrames checkout")
if(NOT EXISTS "${XFRAMES_SOURCE_DIR}/packages/dear-imgui/cpp/app/src/xframes_commit.cpp")
    message(FATAL_ERROR "The XFrames checkout needs the schema-2 publication API. Update its submodule or set XFRAMES_SOURCE_DIR.")
endif()
project(xframesshared VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(DEPS "${XFRAMES_SOURCE_DIR}/packages/dear-imgui/cpp/deps")
set(APP "${XFRAMES_SOURCE_DIR}/packages/dear-imgui/cpp/app")
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(ada CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(qjs CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)

set(JANET_ROOT "${DEPS}/janet")
file(GLOB JANET_CORE CONFIGURE_DEPENDS "${JANET_ROOT}/src/core/*.c")
file(GLOB JANET_BOOT CONFIGURE_DEPENDS "${JANET_ROOT}/src/boot/*.c")
set(JANET_FLAGS JANET_NO_EV JANET_NO_FFI JANET_NO_NET JANET_NO_DYNAMIC_MODULES)
add_executable(janet_boot ${JANET_CORE} ${JANET_BOOT})
target_include_directories(janet_boot PRIVATE "${JANET_ROOT}/src/include" "${JANET_ROOT}/src/conf")
target_compile_definitions(janet_boot PRIVATE JANET_BOOTSTRAP ${JANET_FLAGS})
set(JANET_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/janet_core_image.c")
add_custom_command(OUTPUT "${JANET_IMAGE}"
    COMMAND ${CMAKE_COMMAND} -DJANET_EXECUTABLE=$<TARGET_FILE:janet_boot> -DJANET_ROOT=${JANET_ROOT}
            -DOUTPUT_FILE=${JANET_IMAGE} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/janet-image.cmake"
    DEPENDS janet_boot "${JANET_ROOT}/src/boot/boot.janet" VERBATIM)
add_library(janet_core STATIC ${JANET_CORE} "${JANET_IMAGE}")
target_include_directories(janet_core PUBLIC "${JANET_ROOT}/src/include" "${JANET_ROOT}/src/conf")
target_compile_definitions(janet_core PRIVATE ${JANET_FLAGS} JANET_SINGLE_THREADED)
if(WIN32)
    target_compile_definitions(janet_boot PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(janet_core PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(janet_boot PRIVATE ws2_32)
    target_link_libraries(janet_core PRIVATE ws2_32)
else()
    target_link_libraries(janet_boot PRIVATE m)
    target_link_libraries(janet_core PUBLIC m)
endif()

file(GLOB YOGA CONFIGURE_DEPENDS "${DEPS}/yoga/yoga/*.cpp" "${DEPS}/yoga/yoga/**/*.cpp")
file(GLOB APP_SOURCES CONFIGURE_DEPENDS "${APP}/src/*.cpp" "${APP}/src/element/*.cpp" "${APP}/src/widget/*.cpp")
file(GLOB LEPTONICA CONFIGURE_DEPENDS "${DEPS}/osm-static-map-generator/cpp/deps/leptonica/src/*.c")
# Keep generated headers in this build, leaving upstream dependency checkouts untouched.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
include(TestBigEndian)
test_big_endian(XF_BIG_ENDIAN)
if(XF_BIG_ENDIAN)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/endianness.h" "#define L_BIG_ENDIAN\n")
else()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/endianness.h" "#define L_LITTLE_ENDIAN\n")
endif()
add_library(xframesshared SHARED
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xframes-transaction.cpp"
    ${APP_SOURCES} ${YOGA} ${LEPTONICA}
    "${DEPS}/css-color-parser-cpp/csscolorparser.cpp"
    "${DEPS}/imgui/imgui.cpp" "${DEPS}/imgui/imgui_draw.cpp"
    "${DEPS}/imgui/imgui_widgets.cpp" "${DEPS}/imgui/imgui_tables.cpp"
    "${DEPS}/imgui/misc/cpp/imgui_stdlib.cpp"
    "${DEPS}/imgui/backends/imgui_impl_opengl3.cpp" "${DEPS}/imgui/backends/imgui_impl_glfw.cpp"
    "${DEPS}/implot/implot.cpp" "${DEPS}/implot/implot_items.cpp"
    "${DEPS}/osm-static-map-generator/cpp/shared.cpp"
    "${DEPS}/osm-static-map-generator/cpp/tiledownloader.cpp"
    "${DEPS}/osm-static-map-generator/cpp/mapgenerator.cpp"
    "${DEPS}/osm-static-map-generator/cpp/tilecache.cpp")
target_compile_definitions(xframesshared PRIVATE XFRAMES_SHARED_BUILD HAVE_LIBJPEG HAVE_LIBTIFF HAVE_LIBPNG NOMINMAX _USE_MATH_DEFINES)
target_include_directories(xframesshared PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated" "${APP}/include" ${Stb_INCLUDE_DIR} ${LUA_INCLUDE_DIR}
    "${DEPS}/IconFontCppHeaders" "${DEPS}/ReactivePlusPlus/src/rpp" "${DEPS}/css-color-parser-cpp"
    "${DEPS}/imgui" "${DEPS}/imgui/backends" "${DEPS}/imgui/misc/cpp" "${DEPS}/implot"
    "${DEPS}/yoga" "${DEPS}/osm-static-map-generator/cpp"
    "${DEPS}/osm-static-map-generator/cpp/deps/leptonica/src")
target_include_directories(xframesshared PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>" "$<INSTALL_INTERFACE:include>")
target_link_libraries(xframesshared PRIVATE ada::ada fmt::fmt nlohmann_json::nlohmann_json
    JPEG::JPEG PNG::PNG TIFF::TIFF CURL::libcurl glfw OpenGL::GL qjs ${LUA_LIBRARIES} sol2::sol2 janet_core)
install(TARGETS xframesshared RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/xframes-transaction.h" DESTINATION include)
