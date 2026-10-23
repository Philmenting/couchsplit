# mGBA is built as an isolated static core: no replacement Qt frontend,
# debugger, network service, updater or runtime download.
set(LIBMGBA_ONLY ON)
set(M_CORE_GBA ON)
set(M_CORE_GB OFF)
set(USE_DEBUGGERS OFF)
set(ENABLE_SCRIPTING OFF)
set(BINARY_NAME couchsplit-mgba-core)
if(NOT COUCHSPLIT_MGBA_SOURCE)
    include(FetchContent)
    FetchContent_Declare(couchsplit_mgba
        GIT_REPOSITORY https://github.com/mgba-emu/mgba.git
        GIT_TAG 26b7884bc25a5933960f3cdcd98bac1ae14d42e2)
    FetchContent_GetProperties(couchsplit_mgba)
    if(NOT couchsplit_mgba_POPULATED)
        FetchContent_Populate(couchsplit_mgba)
    endif()
    set(COUCHSPLIT_MGBA_SOURCE "${couchsplit_mgba_SOURCE_DIR}")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
# Reproducible local overlay; retain the pristine pinned source for packaging/build checks.
file(SHA256 "${CMAKE_SOURCE_DIR}/patches/mgba-xrom.patch" xrom_patch_hash)
string(SUBSTRING "${xrom_patch_hash}" 0 16 xrom_patch_id)
set(COUCHSPLIT_MGBA_XROM_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/mgba-xrom-${xrom_patch_id}")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare-mgba-xrom.py"
    "${COUCHSPLIT_MGBA_SOURCE}" "${COUCHSPLIT_MGBA_XROM_SOURCE}"
    RESULT_VARIABLE xrom_prepare_result OUTPUT_QUIET)
if(NOT xrom_prepare_result EQUAL 0)
    message(FATAL_ERROR "Cannot prepare pinned mGBA XROM extension")
endif()
# mGBA 0.10.5 declares CMake 3.1. CMake 4 removed policy compatibility below
# 3.5; apply that policy floor only while configuring the pinned dependency.
set(xrom_saved_policy_minimum "${CMAKE_POLICY_VERSION_MINIMUM}")
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()
add_subdirectory("${COUCHSPLIT_MGBA_XROM_SOURCE}" mgba EXCLUDE_FROM_ALL)
set(CMAKE_POLICY_VERSION_MINIMUM "${xrom_saved_policy_minimum}")
add_executable(gba-worker Worker.cpp)
target_include_directories(gba-worker PRIVATE ../display ../saves ../launcher)
target_link_libraries(gba-worker PRIVATE couchsplit-mgba-core Qt6::Core ${COOP_SDL_TARGET})
target_compile_definitions(gba-worker PRIVATE M_CORE_GBA)
target_compile_options(gba-worker PRIVATE -Wall -Wextra -Wpedantic)
install(TARGETS gba-worker RUNTIME DESTINATION bin)
