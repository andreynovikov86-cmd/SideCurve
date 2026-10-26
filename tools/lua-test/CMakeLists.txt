cmake_minimum_required(VERSION 3.25)
project(SideCurveInstallerTests LANGUAGES C)
include(FetchContent)
FetchContent_Declare(lua
 URL https://www.lua.org/ftp/lua-5.4.9.tar.gz
 URL_HASH SHA256=2335b6c582a52654f94612bf10d2f4672805d05329aa6568b1d8cd9e5c6fb8e6
 DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(lua)
file(GLOB lua_sources "${lua_SOURCE_DIR}/src/*.c")
list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/src/luac.c")
add_executable(sidecurve_lua ${lua_sources})
set_property(TARGET sidecurve_lua PROPERTY C_STANDARD 99)
if(APPLE)
 target_compile_definitions(sidecurve_lua PRIVATE LUA_USE_MACOSX)
elseif(UNIX)
 target_compile_definitions(sidecurve_lua PRIVATE LUA_USE_LINUX)
 target_link_libraries(sidecurve_lua PRIVATE m ${CMAKE_DL_LIBS})
endif()
