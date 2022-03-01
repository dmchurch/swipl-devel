# Set a default build type if none was specified
# Based on https://blog.kitware.com/cmake-and-the-default-build-type/
if(NOT default_build_type)
  set(default_build_type "Release")
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    set(default_build_type "RelWithDebInfo")
  endif()
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
	       "Debug" "Release" "MinSizeRel" "RelWithDebInfo" "PGO" "DEB")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "DEB")
  message("-- Setting up flags for Debian based distro packaging")

  function(dpkg_buildflags var flags)
    execute_process(COMMAND dpkg-buildflags --get ${flags}
		    OUTPUT_VARIABLE ${var}
		    OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(var ${var} PARENT_SCOPE)
  endfunction()

  dpkg_buildflags(CMAKE_C_FLAGS_DEB		CFLAGS)
  dpkg_buildflags(CMAKE_CXX_FLAGS_DEB           CPPFLAGS)
  dpkg_buildflags(CMAKE_SHARED_LINKER_FLAGS_DEP LDFLAGS)
  dpkg_buildflags(CMAKE_EXE_LINKER_FLAGS_DEP    LDFLAGS)
endif()

# Using gdwarf-2 -g3 allows using macros in gdb, which helps a lot
# when debugging the Prolog internals.
if(CMAKE_COMPILER_IS_GNUCC)
  set(DEBUG_SYMBOL_FLAGS "-gdwarf-2 -g3")
  set(SANITIZE_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
  set(COMPILER_IS_GNULIKE 1)
elseif(EMSCRIPTEN)
  set(DEBUG_SYMBOL_FLAGS "-g3 -gsource-map -fdebug-macro")
  set(COMPILER_NEEDS_FLAGS "-sDEFAULT_TO_CXX=0")
  set(LINKER_NEEDS_FLAGS "-sALLOW_MEMORY_GROWTH=1")

  # CMake doesn't have a single per-config variable for LDFLAGS, so...
  set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "-sASSERTIONS=1 -sSAFE_HEAP=1"
      CACHE STRING "LDFLAGS for a Debug build, library linking" FORCE)
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-sASSERTIONS=1 -sSAFE_HEAP=1"
      CACHE STRING "LDFLAGS for a Debug build, final linking" FORCE)
  set(CMAKE_SHARED_LINKER_FLAGS "${LINKER_NEEDS_FLAGS}"
      CACHE STRING "LDFLAGS for a standard build, library linking" FORCE)
  set(CMAKE_EXE_LINKER_FLAGS "${LINKER_NEEDS_FLAGS}"
      CACHE STRING "LDFLAGS for a standard build, final linking" FORCE)
  set(COMPILER_IS_GNULIKE 1)
elseif(CMAKE_C_COMPILER_ID STREQUAL Clang)
  set(DEBUG_SYMBOL_FLAGS "-g3 -fdebug-macro")
  set(COMPILER_IS_GNULIKE 1)
elseif(CMAKE_C_COMPILER_ID STREQUAL AppleClang)
  set(DEBUG_SYMBOL_FLAGS "-gdwarf-2 -g3")
  set(COMPILER_IS_GNULIKE 1)
endif()

if(COMPILER_IS_GNULIKE)
  set(CMAKE_C_FLAGS_DEBUG "-DO_DEBUG -DO_DEBUG_ATOMGC -O0 ${DEBUG_SYMBOL_FLAGS} ${COMPILER_NEEDS_FLAGS}"
      CACHE STRING "CFLAGS for a Debug build" FORCE)
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 ${DEBUG_SYMBOL_FLAGS} ${COMPILER_NEEDS_FLAGS}"
      CACHE STRING "CFLAGS for a RelWithDebInfo build" FORCE)
  set(CMAKE_C_FLAGS_RELEASE "-O2 ${COMPILER_NEEDS_FLAGS}"
      CACHE STRING "CFLAGS for a Release build" FORCE)
  set(CMAKE_C_FLAGS_PGO "-O2 ${DEBUG_SYMBOL_FLAGS} ${COMPILER_NEEDS_FLAGS}"
      CACHE STRING "CFLAGS for a PGO build" FORCE)
  set(CMAKE_C_FLAGS_SANITIZE
      "-O0 ${DEBUG_SYMBOL_FLAGS} ${SANITIZE_FLAGS} ${COMPILER_NEEDS_FLAGS}"
      CACHE STRING "CFLAGS for a Sanitize build" FORCE)
else()
  message("Unknown C compiler.  ${CMAKE_C_COMPILER_ID}")
endif()
