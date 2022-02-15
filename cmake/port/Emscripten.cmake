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
set(VMI_FUNCTIONS ON) # The WebAssembly VM needs to use the functions implementation, at least with the current codebase

if(MULTI_THREADED)
# If multithreading is requested, make sure -pthread is passed for EVERY command line, since
# it will have to build an entirely separate set of system libraries.
add_compile_options(-pthread)
add_link_options(-pthread)
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -pthread")
endif()

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

function(check_node_feature js_expression var)
    if(DEFINED "HAVE_${var}")
        return()
    endif()
    if(ARGN)
        set(description "to use ${ARGN} in Node")
    else()
        set(description "for Node feature ${var}")
    endif()
    set(node_script "process.exit((${js_expression})?0:1)")
    message(STATUS "Checking for flags required ${description}")
    set(success 0)
    set(output)
    foreach(node_opt "" "--wasm-staging" "--wasm-staging --experimental-wasm-threads" "--experimental-wasm-threads")
        if(NOT node_opt)
            set(node_opt_desc "(no extra flags)")
        else()
            set(node_opt_desc "${node_opt}")
        endif()
        message(STATUS "Checking flag ${node_opt_desc}...")
        execute_process(COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} ${node_opt} -e "${node_script}"
                        TIMEOUT 10
                        RESULT_VARIABLE retval
                        OUTPUT_VARIABLE out
                        ERROR_VARIABLE out)
        set(output "${output}> ${CMAKE_CROSSCOMPILING_EMULATOR} ${node_opt} -e \"${node_script}\"\nRetval ${retval}, output:\n${out}\n")
        if(retval EQUAL 0)
            set(success 1)
            set(feature_opt "${node_opt}")
            set(feature_opt_desc "${node_opt_desc}")
            break()
        endif()
    endforeach()
    if(success)
        message(STATUS "Checking for flags required ${description} - ${feature_opt_desc}")
        set(${var} "${feature_opt}" CACHE INTERNAL "Node flags for feature ${var}")
        set("HAVE_${var}" "1" CACHE INTERNAL "Have Node feature ${var}")
        file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
            "Determining the flags to use ${var} in Node "
            "passed with the following output:\n"
            "${output}\n\n")
    else()
        message(STATUS "Checking for flags required ${description} - not found")
        set("HAVE_${var}" "" CACHE INTERNAL "Have Node feature ${var}")
        file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
            "Determining the flags to use ${var} in Node "
            "failed with the following output:\n"
            "${output}\n\n")
    endif()
endfunction()

if(MULTI_THREADED AND NOT SWIPL_NATIVE_FRIEND)
    check_node_feature("new WebAssembly.Memory({initial:1,maximum:1,shared:true}).buffer instanceof SharedArrayBuffer" NODE_WASM_THREADS "WebAssembly threads")
    if (NOT HAVE_NODE_WASM_THREADS)
        message(FATAL_ERROR "Error: Cannot build a multithreaded WASM implementation without a recent version of Node or the SWIPL_NATIVE_FRIEND option")
    endif()
    set(CMAKE_CROSSCOMPILING_EMULATOR_FLAGS ${CMAKE_CROSSCOMPILING_EMULATOR_FLAGS} ${NODE_WASM_THREADS})
endif()

# Setting this will bypass FindZLIB's check for the file's location
set(ZLIB_LIBRARY -sUSE_ZLIB=1)

# This will also fetch and build zlib, if necessary
get_emscripten_cflags(ZLIB_COMPILE_OPTIONS ${ZLIB_LIBRARY})
