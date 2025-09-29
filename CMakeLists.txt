cmake_minimum_required(VERSION 3.10)

file(STRINGS src/game/version.h VERSION_LINE
  LIMIT_COUNT 1
  REGEX GAME_RELEASE_VERSION
)

if(VERSION_LINE MATCHES "\"([0-9]+)\\.([0-9]+)\\.([0-9]+)\"")
  set(VERSION_MAJOR ${CMAKE_MATCH_1})
  set(VERSION_MINOR ${CMAKE_MATCH_2})
  set(VERSION_PATCH ${CMAKE_MATCH_3})
elseif(VERSION_LINE MATCHES "\"([0-9]+)\\.([0-9]+)\"")
  set(VERSION_MAJOR ${CMAKE_MATCH_1})
  set(VERSION_MINOR ${CMAKE_MATCH_2})
  set(VERSION_PATCH "0")
else()
  message(FATAL_ERROR "Couldn't parse version from src/game/version.h")
endif()

if(POLICY CMP0048)
  cmake_policy(SET CMP0148 NEW)
  if(VERSION_PATCH STREQUAL "0")
    project(teeworlds VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
  else()
    project(teeworlds VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
  endif()
else()
  project(teeworlds)
  set(PROJECT_VERSION_MAJOR ${VERSION_MAJOR})
  set(PROJECT_VERSION_MINOR ${VERSION_MINOR})
  set(PROJECT_VERSION_PATCH ${VERSION_PATCH})
  if(VERSION_PATCH STREQUAL "0")
    set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
  else()
    set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
  endif()
endif()

set(ORIGINAL_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
set(ORIGINAL_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(ORIGINAL_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
set(OWN_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${OWN_CMAKE_MODULE_PATH})

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(TARGET_BITS "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(TARGET_BITS "32")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(TARGET_OS "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(TARGET_OS "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(TARGET_OS "mac")
endif()

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckSymbolExists)

check_symbol_exists(__i386 "" TARGET_ARCH_X86_i386)
if(TARGET_ARCH_X86_i386)
  set(TARGET_ARCH x86)
else()
  set(TARGET_ARCH)
endif()

set(AUTO_DEPENDENCIES_DEFAULT OFF)
if(TARGET_OS STREQUAL "windows")
  set(AUTO_DEPENDENCIES_DEFAULT ON)
endif()

option(PREFER_BUNDLED_LIBS "Prefer bundled libraries over system libraries" ${AUTO_DEPENDENCIES_DEFAULT})
option(DEV "Don't generate stuff necessary for packaging" OFF)

# Set the default build type to Release
if(NOT(CMAKE_BUILD_TYPE))
  if(DEV)
    set(CMAKE_BUILD_TYPE Release)
  else()
    set(CMAKE_BUILD_TYPE Debug)
  endif()
endif()

set(DBG $<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  src/game/version.h
)

set(SERVER_EXECUTABLE teeworlds_srv CACHE STRING "Name of the built server executable")

########################################################################
# Compiler flags
########################################################################

function(add_c_compiler_flag_if_supported VARIABLE FLAG)
  if(ARGC GREATER 2)
    set(CHECKED_FLAG "${ARGV2}")
  else()
    set(CHECKED_FLAG "${FLAG}")
  endif()
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" CONFIG_VARIABLE "FLAG_SUPPORTED${CHECKED_FLAG}")
  check_c_compiler_flag("${CHECKED_FLAG}" ${CONFIG_VARIABLE})
  if(${CONFIG_VARIABLE})
    if(${VARIABLE})
      set("${VARIABLE}" "${${VARIABLE}};${FLAG}" PARENT_SCOPE)
    else()
      set("${VARIABLE}" "${FLAG}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

if(NOT MSVC)
  # Protect the stack pointer.
  # -fstack-protector-all doesn't work on MinGW.
  add_c_compiler_flag_if_supported(OUR_FLAGS -fstack-protector-all)

  # Inaccurate floating point numbers cause problems on mingw-w64-gcc when
  # compiling for x86, might cause problems elsewhere. So don't store floats
  # in registers but keep them at higher accuracy.

  if(TARGET_ARCH STREQUAL "x86")
    add_c_compiler_flag_if_supported(OUR_FLAGS -ffloat-store)
  endif()

  if(TARGET_OS STREQUAL "mac")
    add_c_compiler_flag_if_supported(OUR_FLAGS -stdlib=libc++)
    add_c_compiler_flag_if_supported(OUR_FLAGS -mmacosx-version-min=10.7)
  endif()

  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wall)
  if(CMAKE_VERSION VERSION_GREATER 3.3 OR CMAKE_VERSION VERSION_EQUAL 3.3)
    add_c_compiler_flag_if_supported(OUR_FLAGS_OWN
      $<$<COMPILE_LANGUAGE:C>:-Wdeclaration-after-statement>
      -Wdeclaration-after-statement
    )
  endif()
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wextra)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-unused-parameter)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-missing-field-initializers)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wformat=2) # Warn about format strings.
  add_c_compiler_flag_if_supported(OUR_FLAGS_DEP -Wno-implicit-function-declaration)
endif()

if(NOT MSVC)
  check_c_compiler_flag("-O2;-Wp,-Werror;-D_FORTIFY_SOURCE=2" DEFINE_FORTIFY_SOURCE) # Some distributions define _FORTIFY_SOURCE by themselves.
endif()

########################################################################
# COMMON FUNCTIONS
########################################################################

function(set_glob VAR GLOBBING DIRECTORY) # ...
  file(${GLOBBING} GLOB_RESULT "${DIRECTORY}/*.c" "${DIRECTORY}/*.cpp" "${DIRECTORY}/*.h")
  list(SORT GLOB_RESULT)
  set(FILES)
  foreach(file ${ARGN})
    list(APPEND FILES "${PROJECT_SOURCE_DIR}/${DIRECTORY}/${file}")
  endforeach()

  if(NOT FILES STREQUAL GLOB_RESULT)
    message(AUTHOR_WARNING "${VAR} does not contain every file from directory ${DIRECTORY}")
    set(LIST_BUT_NOT_GLOB)
    if(POLICY CMP0057)
      cmake_policy(SET CMP0057 NEW)
      foreach(file ${FILES})
        if(NOT file IN_LIST GLOB_RESULT)
          list(APPEND LIST_BUT_NOT_GLOB ${file})
        endif()
      endforeach()
      if(LIST_BUT_NOT_GLOB)
        message(AUTHOR_WARNING "Entries only present in ${VAR}: ${LIST_BUT_NOT_GLOB}")
      endif()
      set(GLOB_BUT_NOT_LIST)
      foreach(file ${GLOB_RESULT})
        if(NOT file IN_LIST FILES)
          list(APPEND GLOB_BUT_NOT_LIST ${file})
        endif()
      endforeach()
      if(GLOB_BUT_NOT_LIST)
        message(AUTHOR_WARNING "Entries only present in ${DIRECTORY}: ${GLOB_BUT_NOT_LIST}")
      endif()
    endif()
  endif()

  set(${VAR} ${FILES} PARENT_SCOPE)
endfunction()

########################################################################
# INITIALIZE TARGET LISTS
########################################################################

set(TARGETS_OWN)
set(TARGETS_DEP)

set(TARGETS_LINK) # Targets with a linking stage.

########################################################################
# DEPENDENCIES
########################################################################

function(set_extra_dirs_lib VARIABLE NAME)
  set("PATHS_${VARIABLE}_LIBDIR" PARENT_SCOPE)
  set("HINTS_${VARIABLE}_LIBDIR" PARENT_SCOPE)
  if(PREFER_BUNDLED_LIBS)
    set(TYPE HINTS)
  else()
    set(TYPE PATHS)
  endif()
  if(TARGET_BITS AND TARGET_OS)
    set(DIR "other/${NAME}/${TARGET_OS}/lib${TARGET_BITS}")
    set("${TYPE}_${VARIABLE}_LIBDIR" "${DIR}" PARENT_SCOPE)
    set("EXTRA_${VARIABLE}_LIBDIR" "${DIR}" PARENT_SCOPE)
  endif()
endfunction()

function(set_extra_dirs_include VARIABLE NAME LIBRARY)
  set("PATHS_${VARIABLE}_INCLUDEDIR" PARENT_SCOPE)
  set("HINTS_${VARIABLE}_INCLUDEDIR" PARENT_SCOPE)
  is_bundled(IS_BUNDLED "${LIBRARY}")
  if(IS_BUNDLED)
    set("HINTS_${VARIABLE}_INCLUDEDIR" "other/${NAME}/include" "other/${NAME}/include/${TARGET_OS}" PARENT_SCOPE)
  endif()
endfunction()

if(CMAKE_CROSSCOMPILING)
  set(CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH NO_CMAKE_SYSTEM_PATH)
else()
  set(CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH)
endif()

function(is_bundled VARIABLE PATH)
  if(PATH)
    string(FIND "${PATH}" "${PROJECT_SOURCE_DIR}" LOCAL_PATH_POS)
    if(LOCAL_PATH_POS EQUAL 0 AND TARGET_BITS AND TARGET_OS)
      set("${VARIABLE}" ON PARENT_SCOPE)
    else()
      set("${VARIABLE}" OFF PARENT_SCOPE)
    endif()
  else()
    set("${VARIABLE}" OFF PARENT_SCOPE)
  endif()
endfunction()

if(NOT CMAKE_CROSSCOMPILING)
  # Check for PkgConfig once so all the other `find_package` calls can do it
  # quietly.
  find_package(PkgConfig)
endif()
find_package(ZLIB)
find_package(Git)
find_package(Python3 COMPONENTS Interpreter)
find_package(Threads)
find_package(Curl)

# Find MySQL/MariaDB C++ Connector - SINGLE LOCATION
if(PkgConfig_FOUND)
    pkg_check_modules(MYSQLCPPCONN QUIET mysqlcppconn)
endif()

if(NOT MYSQLCPPCONN_FOUND)
    find_path(MYSQLCPPCONN_INCLUDE_DIRS
        NAMES mysql_connection.h cppconn/driver.h
        PATHS 
            /usr/include/mysql-cppconn 
            /usr/local/include/mysql-cppconn
            /usr/include/mysql-cppconn-8
            /usr/local/include/mysql-cppconn-8
            /opt/homebrew/include/mysql-cppconn-8
            ${CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH}
    )
    
    find_library(MYSQLCPPCONN_LIBRARIES
        NAMES mysqlcppconn mysqlcppconn8
        PATHS 
            /usr/lib 
            /usr/local/lib
            /opt/homebrew/lib
            ${CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH}
    )
    
    if(MYSQLCPPCONN_INCLUDE_DIRS AND MYSQLCPPCONN_LIBRARIES)
        set(MYSQLCPPCONN_FOUND TRUE)
    endif()
endif()

message(STATUS "******** Teeworlds ********")
message(STATUS "Target OS: ${TARGET_OS} ${TARGET_BITS}bit")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

message(STATUS "Dependencies:")
function(show_dependency_status OUTPUT_NAME NAME)
  if(${NAME}_FOUND)
    if(${NAME}_BUNDLED)
      message(STATUS " * ${OUTPUT_NAME} not found (using bundled version)")
    else()
      message(STATUS " * ${OUTPUT_NAME} found")
    endif()
  else()
    message(STATUS " * ${OUTPUT_NAME} not found")
  endif()
endfunction()

show_dependency_status("Python" Python3_Interpreter)
show_dependency_status("Zlib" ZLIB)
show_dependency_status("Curl" CURL)
show_dependency_status("MySQL C++ Connector" MYSQLCPPCONN)

if(NOT Python3_Interpreter_FOUND)
  message(SEND_ERROR "You must install Python to compile Teeworlds")
endif()

if(NOT MYSQLCPPCONN_FOUND)
  message(WARNING "MySQL C++ Connector not found. Database features will be disabled.")
  message(STATUS "To install MySQL C++ Connector:")
  message(STATUS "  Ubuntu/Debian: sudo apt install libmysqlcppconn-dev")
  message(STATUS "  macOS: brew install mysql-connector-c++")
  message(STATUS "  Or install MariaDB: sudo apt install libmariadb-dev")
endif()

if(TARGET_OS STREQUAL "windows")
  set(PLATFORM_LIBS ws2_32) # Windows sockets
elseif(TARGET_OS STREQUAL "mac")
  find_library(CARBON Carbon)
  set(PLATFORM_LIBS ${CARBON})
else()
  if(TARGET_OS STREQUAL "linux")
    set(PLATFORM_LIBS rt) # clock_gettime for glibc < 2.17
  else()
    set(PLATFORM_LIBS)
  endif()
endif()

########################################################################
# GENERATOR SPECIFICS
########################################################################

if(CMAKE_GENERATOR STREQUAL "Ninja")
    # Force colored compiler diagnostics
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            add_compile_options(-fdiagnostics-color=always)
            add_link_options(-fdiagnostics-color=always)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            add_compile_options(-fcolor-diagnostics)
            add_link_options(-fcolor-diagnostics)
        endif()
    endif()
  
    set(ENV{CLICOLOR_FORCE} "1")
    set(ENV{FORCE_COLOR} "1")
endif()

########################################################################
# DEPENDENCY COMPILATION
########################################################################

set_glob(DEP_MD5_SRC GLOB src/engine/external/md5 md5.c md5.h)
add_library(md5 EXCLUDE_FROM_ALL OBJECT ${DEP_MD5_SRC})

list(APPEND TARGETS_DEP md5)
set(DEP_MD5 $<TARGET_OBJECTS:md5>)

set_glob(DEP_JSONPARSER_SRC GLOB src/engine/external/json json.c json.h)
add_library(json-parser EXCLUDE_FROM_ALL OBJECT ${DEP_JSONPARSER_SRC})

list(APPEND TARGETS_DEP json-parser)
set(DEP_JSONPARSER $<TARGET_OBJECTS:json-parser>)

# Set up DEPS and LIBS variables
set(DEPS ${DEP_MD5} ${ZLIB_DEP} ${DEP_JSONPARSER})
set(LIBS ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES} ${PLATFORM_LIBS})

########################################################################
# COPY DATA AND DLLS
########################################################################

file(COPY maps DESTINATION .)

########################################################################
# CODE GENERATION
########################################################################

function(chash output_file)
  add_custom_command(OUTPUT ${output_file}
    COMMAND ${Python3_EXECUTABLE} scripts/cmd5.py ${ARGN}
      > "${PROJECT_BINARY_DIR}/${output_file}"
    DEPENDS scripts/cmd5.py ${ARGN}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endfunction()

function(generate_source output_file script_parameter)
  add_custom_command(OUTPUT ${output_file}
    COMMAND ${Python3_EXECUTABLE} datasrc/compile.py ${script_parameter}
      > "${PROJECT_BINARY_DIR}/${output_file}"
    DEPENDS
      datasrc/compile.py
      datasrc/content.py
      datasrc/datatypes.py
      datasrc/network.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endfunction()

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src/game/generated/")
if(GIT_FOUND)
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --git-dir
    ERROR_QUIET
    OUTPUT_VARIABLE PROJECT_GIT_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE PROJECT_GIT_DIR_ERROR
  )
  if(NOT PROJECT_GIT_DIR_ERROR)
    set(GIT_REVISION_EXTRA_DEPS
      ${PROJECT_GIT_DIR}/index
      ${PROJECT_GIT_DIR}/logs/HEAD
    )
  endif()
endif()
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src/game/generated/git_revision.cpp
  COMMAND ${Python3_EXECUTABLE}
    scripts/git_revision.py
    > ${PROJECT_BINARY_DIR}/src/game/generated/git_revision.cpp
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  DEPENDS
    ${GIT_REVISION_EXTRA_DEPS}
    scripts/git_revision.py
)
chash("src/game/generated/nethash.cpp"
  "src/engine/shared/protocol.h"
  "${PROJECT_BINARY_DIR}/src/game/generated/protocol.h"
  "src/game/tuning.h"
  "src/game/gamecore.cpp"
)
generate_source("src/game/generated/protocol.cpp" "network_source")
generate_source("src/game/generated/protocol.h" "network_header")
generate_source("src/game/generated/server_data.cpp" "server_content_source")
generate_source("src/game/generated/server_data.h" "server_content_header")

########################################################################
# SHARED
########################################################################

# Sources
set_glob(BASE GLOB_RECURSE src/base
  detect.h
  math.h
  hash.h
  hash.cpp
  hash_bundled.cpp
  hash_ctxt.h
  hash_openssl.cpp
  hash_libtomcrypt.cpp
  vmath.h
  types.h
  color.h
  lock_scope.h
  system.cpp
  system.h
  crypt.cpp
  crypt.h
  tl/algorithm.h
  tl/allocator.h
  tl/array.h
  tl/base.h
  tl/range.h
  tl/sorted_array.h
  tl/string.h
  tl/threading.h
)
set_glob(ENGINE_INTERFACE GLOB src/engine
  config.h
  console.h
  demo.h
  engine.h
  kernel.h
  map.h
  masterserver.h
  message.h
  server.h
  storage.h
  uuid.h
)
set_glob(ENGINE_SHARED GLOB src/engine/shared
  compression.cpp
  compression.h
  config.cpp
  config.h
  config_variables.h
  console.cpp
  console.h
  datafile.cpp
  datafile.h
  demo.cpp
  demo.h
  econ.cpp
  econ.h
  engine.cpp
  filecollection.cpp
  filecollection.h
  global_uuid_manager.cpp
  http.cpp
  http.h
  huffman.cpp
  huffman.h
  jobs.cpp
  jobs.h
  json.cpp
  json.h
  jsonwriter.cpp
  jsonwriter.h
  kernel.cpp
  linereader.cpp
  linereader.h
  map.cpp
  map.h
  memheap.cpp
  memheap.h
  message.h
  netban.cpp
  netban.h
  network.cpp
  network.h
  network_conn.cpp
  network_console.cpp
  network_console_conn.cpp
  network_server.cpp
  network_server_hack.cpp
  network_stun.cpp
  packer.cpp
  packer.h
  protocol.h
  protocol_ex.cpp
  protocol_ex.h
  protocol_ex_msgs.h
  teehistorian_ex_chunks.h
  ringbuffer.cpp
  ringbuffer.h
  snapshot.cpp
  snapshot.h
  storage.cpp
  stun.cpp
  stun.h
  teehistorian_ex.cpp
  teehistorian_ex.h
  uuid_manager.cpp
  uuid_manager.h
)
set(ENGINE_GENERATED_SHARED src/game/generated/protocol.cpp src/game/generated/protocol.h)
set_glob(GAME_SHARED GLOB src/game
  collision.cpp
  collision.h
  gamecore.cpp
  gamecore.h
  layers.cpp
  layers.h
  mapitems.h
  mapitems_ex_types.h
  mapitems_ex.cpp
  mapitems_ex.h
  tuning.h
  variables.h
  version.h
  voting.h
)
set(GAME_GENERATED_SHARED
  src/game/generated/git_revision.cpp
  src/game/generated/protocol.h
)

# Targets
add_library(engine-shared EXCLUDE_FROM_ALL OBJECT ${ENGINE_INTERFACE} ${ENGINE_SHARED} ${ENGINE_GENERATED_SHARED} ${BASE})
add_library(game-shared EXCLUDE_FROM_ALL OBJECT ${GAME_SHARED} ${GAME_GENERATED_SHARED})
list(APPEND TARGETS_OWN engine-shared game-shared)

########################################################################
# SERVER
########################################################################

# Sources
set_glob(ENGINE_SERVER GLOB src/engine/server
  register.cpp
  register.h
  server.cpp
  server.h
  mastersrv.h
)
set_glob(GAME_SERVER GLOB_RECURSE src/game/server
  entities/character.cpp
  entities/character.h
  entities/flag.cpp
  entities/flag.h
  entities/laser.cpp
  entities/laser.h
  entities/pickup.cpp
  entities/pickup.h
  entities/projectile.cpp
  entities/projectile.h
  entity.cpp
  entity.h
  eventhandler.cpp
  eventhandler.h
  gamecontext.cpp
  gamecontext.h
  accounts.cpp
  accounts.h
  gamecontroller.cpp
  gamecontroller.h
  gamemodes/base_pvp.cpp
  gamemodes/base_pvp.h
  gamemodes/block.cpp
  gamemodes/block.h
  gamemodes/ctf.cpp
  gamemodes/ctf.h
  gamemodes/dm.cpp
  gamemodes/dm.h
  gamemodes/lms.cpp
  gamemodes/lms.h
  gamemodes/lts.cpp
  gamemodes/lts.h
  gamemodes/mod.cpp
  gamemodes/mod.h
  gamemodes/tdm.cpp
  gamemodes/tdm.h
  gameworld.cpp
  gameworld.h
  player.cpp
  player.h
  commands.cpp
  rollback.cpp
  rollback.h
)
set(GAME_GENERATED_SERVER
  "src/game/generated/server_data.cpp"
  "src/game/generated/server_data.h"
)
set(SERVER_SRC ${ENGINE_SERVER} ${GAME_SERVER} ${GAME_GENERATED_SERVER})
if(TARGET_OS STREQUAL "windows")
  set(SERVER_ICON "other/icons/${SERVER_EXECUTABLE}.rc")
else()
  set(SERVER_ICON)
endif()

# Libraries
set(LIBS_SERVER ${LIBS} ${CURL_LIBRARIES} ssl crypto)

# Add MySQL libraries if found
if(MYSQLCPPCONN_FOUND)
    list(APPEND LIBS_SERVER ${MYSQLCPPCONN_LIBRARIES})
endif()

# Target
set(TARGET_SERVER ${SERVER_EXECUTABLE})
add_executable(${TARGET_SERVER}
  ${DEPS}
  ${SERVER_SRC}
  ${SERVER_ICON}
  $<TARGET_OBJECTS:engine-shared>
  $<TARGET_OBJECTS:game-shared>
)
target_link_libraries(${TARGET_SERVER} ${LIBS_SERVER})
list(APPEND TARGETS_OWN ${TARGET_SERVER})
list(APPEND TARGETS_LINK ${TARGET_SERVER})

# Configure MySQL support after target creation
if(MYSQLCPPCONN_FOUND)
    target_include_directories(${TARGET_SERVER} PRIVATE ${MYSQLCPPCONN_INCLUDE_DIRS})
    target_compile_definitions(${TARGET_SERVER} PRIVATE CONF_SQL)
endif()

if(TARGET_OS AND TARGET_OS STREQUAL "mac")
  set(SERVER_LAUNCHER_SRC src/osx/server.mm)
  set(TARGET_SERVER_LAUNCHER ${TARGET_SERVER}-Launcher)
  add_executable(${TARGET_SERVER_LAUNCHER} ${SERVER_LAUNCHER_SRC})
  target_link_libraries(${TARGET_SERVER_LAUNCHER} ${COCOA})
  list(APPEND TARGETS_OWN ${TARGET_SERVER_LAUNCHER})
  list(APPEND TARGETS_LINK ${TARGET_SERVER_LAUNCHER})
endif()

########################################################################
# INSTALLATION
########################################################################

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ${PROJECT_NAME})

if(NOT DEV)
  install(DIRECTORY data DESTINATION share/${PROJECT_NAME} COMPONENT data)
  install(TARGETS ${TARGET_SERVER} DESTINATION bin COMPONENT server)
endif()

########################################################################
# COMPILER-SPECIFICS
########################################################################

# In the future (CMake 3.8.0+), use source_group(TREE ...)
macro(source_group_tree dir)
  file(GLOB ents RELATIVE ${PROJECT_SOURCE_DIR}/${dir} ${PROJECT_SOURCE_DIR}/${dir}/*)
  foreach(ent ${ents})
    if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${dir}/${ent})
      source_group_tree(${dir}/${ent})
    else()
      string(REPLACE "/" "\\" group ${dir})
      source_group(${group} FILES ${PROJECT_SOURCE_DIR}/${dir}/${ent})
    endif()
  endforeach()
endmacro()
source_group_tree(src)

set(TARGETS ${TARGETS_OWN} ${TARGETS_DEP})

foreach(target ${TARGETS})
  if(MSVC)
    target_compile_options(${target} PRIVATE $<$<NOT:${DBG}>:/MT> $<${DBG}:/MTd>) # Use static CRT
    target_compile_options(${target} PRIVATE /MP) # Use multiple cores
    target_compile_options(${target} PRIVATE /EHsc) # Only catch C++ exceptions with catch.
    target_compile_options(${target} PRIVATE /GS) # Protect the stack pointer.
    target_compile_options(${target} PRIVATE /wd4996) # Use of non-_s functions.
  endif()
  if(OUR_FLAGS)
    target_compile_options(${target} PRIVATE ${OUR_FLAGS})
  endif()
  if(DEFINE_FORTIFY_SOURCE)
    target_compile_definitions(${target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:_FORTIFY_SOURCE=2>) # Detect some buffer overflows.
  endif()
endforeach()

foreach(target ${TARGETS_LINK})
  if(MSVC)
    set_property(TARGET ${target} APPEND PROPERTY LINK_FLAGS /SAFESEH:NO) # Disable SafeSEH because the shipped libraries don't support it (would cause error LNK2026 otherwise).
  endif()
  if(TARGET_OS STREQUAL "mac")
    target_link_libraries(${target} -stdlib=libc++)
    target_link_libraries(${target} -mmacosx-version-min=10.7)
  endif()
  if((MINGW OR TARGET_OS STREQUAL "linux") AND PREFER_BUNDLED_LIBS)
    # Statically link the standard libraries with on MinGW/Linux so we don't
    # have to ship them as DLLs.
    target_link_libraries(${target} -static-libgcc)
    target_link_libraries(${target} -static-libstdc++)
    if(MINGW)
      # Link pthread library statically instead of dynamically.
      # Solution from https://stackoverflow.com/a/28001261.
      target_link_libraries(${target} -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic)
    endif()
  endif()
endforeach()

foreach(target ${TARGETS_OWN})
  if(MSVC)
    target_compile_options(${target} PRIVATE /wd4244) # Possible loss of data (float -> int, int -> float, etc.).
    target_compile_options(${target} PRIVATE /wd4267) # Possible loss of data (size_t - int on win64).
    target_compile_options(${target} PRIVATE /wd4800) # Implicit conversion of int to bool.
  endif()
  if(OUR_FLAGS_OWN)
    target_compile_options(${target} PRIVATE ${OUR_FLAGS_OWN})
  endif()
  target_include_directories(${target} PRIVATE ${PROJECT_BINARY_DIR}/src)
  target_include_directories(${target} PRIVATE src)
  target_compile_definitions(${target} PRIVATE $<$<CONFIG:Debug>:CONF_DEBUG>)
  target_include_directories(${target} PRIVATE ${CURL_INCLUDE_DIRS})
  target_include_directories(${target} PRIVATE ${ZLIB_INCLUDE_DIRS})
  
  # Add libpq includes if available
  if(MYSQLCPPCONN)
    target_include_directories(${target} PRIVATE ${MYSQLCPPCONN_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE CONF_SQL)
  endif()
  
  target_compile_features(${target} PUBLIC cxx_std_20)
endforeach()

foreach(target ${TARGETS_DEP})
  if(MSVC)
    target_compile_options(${target} PRIVATE /W0)
  endif()
  if(OUR_FLAGS_DEP)
    target_compile_options(${target} PRIVATE ${OUR_FLAGS_DEP})
  endif()
endforeach()