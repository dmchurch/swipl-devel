# We could set set linker flags for all excutables if we
# could override them for the browser binary.
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ASSERTIONS=2 -s NODERAWFS=1 -s EXIT_RUNTIME=1")

# AlignOf.cmake doesn't work for Emscripten
set(ALIGNOF_INT64_T 8 CACHE STRING "Alignment for int64_t")
set(ALIGNOF_VOIDP   ${SIZEOF_VOIDP} CACHE STRING "Alignment for pointers")
set(ALIGNOF_DOUBLE  8 CACHE STRING "Alignment for double")

set(SWIPL_PACKAGES OFF CACHE BOOL "Packages unsupported on Emscripten" FORCE)
set(HAVE_DLOPEN "" CACHE INTERNAL "dlopen() unsupported on Emscripten" FORCE)
set(SWIPL_SHARED_LIB OFF)
set(USE_SIGNALS OFF)
set(BUILD_SWIPL_LD OFF)
set(INSTALL_DOCUMENTATION OFF)
# set(MULTI_THREADED OFF)

if(MULTI_THREADED)
# If multithreading is requested, make sure -pthread is passed for EVERY command line, since
# it will have to build an entirely separate set of system libraries.
add_compile_options(-pthread)
add_link_options(-pthread)
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -pthread")
endif()

# fsr this doesn't get included by emscripten
list(APPEND CMAKE_FIND_ROOT_PATH "${EMSCRIPTEN_ROOT_PATH}/cache/sysroot")
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "${EMSCRIPTEN_ROOT_PATH}/cache/sysroot/include")

function(get_emscripten_cflags var)
    execute_process(COMMAND ${CMAKE_C_COMPILER} --cflags
                    OUTPUT_VARIABLE orig_cflags)
    execute_process(COMMAND ${CMAKE_C_COMPILER} --cflags ${ARGN}
                    OUTPUT_VARIABLE new_cflags)

    # Turn these into lists
    if(CMAKE_VERSION VERSION_GREATER 3.9)
        set(NATIVE_COMMAND NATIVE_COMMAND)
    elseif(CMAKE_HOST_WIN32)
        set(NATIVE_COMMAND WINDOWS_COMMAND)
    else()
        set(NATIVE_COMMAND UNIX_COMMAND)
    endif()

    separate_arguments(orig_cflags ${NATIVE_COMMAND} "${orig_cflags}")
    separate_arguments(new_cflags ${NATIVE_COMMAND} "${new_cflags}")
    set(target_cflags)
    foreach(flag IN LISTS new_cflags)
        list(GET orig_cflags 0 orig_flag)
        if(flag STREQUAL orig_flag)
            list(REMOVE_AT orig_cflags 0)
        else()
            list(APPEND target_cflags ${flag})
        endif()
    endforeach()
    set(${var} "${target_cflags}" PARENT_SCOPE)
endfunction()

# Setting this will bypass FindZLIB's check for the file's location
set(ZLIB_LIBRARY -sUSE_ZLIB=1)

# This will also fetch and build zlib, if necessary
get_emscripten_cflags(ZLIB_COMPILE_OPTIONS ${ZLIB_LIBRARY})
