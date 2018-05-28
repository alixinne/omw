cmake_minimum_required(VERSION 3.1)

project(omw)

include(CMakeDependentOption)
include(GNUInstallDirs)

find_package(Boost 1.54 REQUIRED)

# Load Octave and Mathematica packages from cmake/
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindMathematica/CMake/Mathematica)
find_package(Mathematica COMPONENTS WSTP)
find_package(Octave)

# Set directories
set(OMW_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(OMW_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(OMW_INCLUDE_DIR ${OMW_BASE_DIR}/include)
set(OMW_SRC_DIR ${OMW_BASE_DIR}/src
  CACHE PATH "Path to the omw sources")

set(OMW_SHARED_HEADERS
  ${OMW_INCLUDE_DIR}/omw.hpp
  ${OMW_INCLUDE_DIR}/omw/pre.hpp
  ${OMW_INCLUDE_DIR}/omw/array.hpp
  ${OMW_INCLUDE_DIR}/omw/matrix.hpp
  ${OMW_INCLUDE_DIR}/omw/wrapper_base.hpp
  ${OMW_INCLUDE_DIR}/omw/type_traits.hpp)

option(OMW_DEVELOP_BUILD "Enable building the development library" OFF)
option(OMW_BUILD_DOCUMENTATION "Build the documentation of OMW using Doxygen" OFF)

# Sets standard options on libomw_* targets
macro(set_shared_options target_name)
  target_include_directories(${target_name} PUBLIC ${OMW_INCLUDE_DIR})

  set_target_properties(${target_name} PROPERTIES
    POSITION_INDEPENDENT_CODE ON # -fPIC for .so
    CXX_STANDARD 14              # C++14 required
  )

  # -Wall
  target_compile_options(${target_name} PRIVATE "-Wall")
endmacro()

# Shared code
add_library(omw_base OBJECT EXCLUDE_FROM_ALL
  ${OMW_SRC_DIR}/wrapper_base.cpp)

set_shared_options(omw_base)

# Mathematica library
if(Mathematica_FOUND)
  add_library(omw_mathematica STATIC EXCLUDE_FROM_ALL
    ${OMW_SHARED_HEADERS}
    ${OMW_INCLUDE_DIR}/omw/mathematica.hpp
    ${OMW_SRC_DIR}/mathematica.cpp
    $<TARGET_OBJECTS:omw_base>)

  set_shared_options(omw_mathematica)

  target_compile_definitions(omw_mathematica PUBLIC
    OMW_MATHEMATICA=1 OMW_INCLUDE_MAIN=1)
  target_include_directories(omw_mathematica PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${Mathematica_WSTP_INCLUDE_DIR})
  target_link_libraries(omw_mathematica INTERFACE
    ${Mathematica_WSTP_LIBRARIES})

  # We need to put some variables in the CMakeCache because
  # omw_add_mathematica will be invoked from an outer scope
  set(MATHEMATICA_VARIABLES Mathematica_SYSTEM_ID Mathematica_BASE_DIR Mathematica_USERBASE_DIR)
  foreach(VAR_NAME ${MATHEMATICA_VARIABLES})
    set(${VAR_NAME} ${${VAR_NAME}}
      CACHE PATH "Mathematica ${VAR_NAME}" FORCE)
  endforeach()

  # Helper function to create a Mathematica target
  function(omw_add_mathematica target_name)
    cmake_parse_arguments(OMW_ADD "USER_INSTALL" "PACKAGE_NAME;INSTALL;TARGET_PACKAGE_DIR"
      "MPREP_SOURCES;SOURCES;LINK_LIBRARIES;COMPILE_OPTIONS;COMPILE_DEFINITIONS" ${ARGN})

    message(STATUS "Creating Mathematica target ${target_name}")

    # Set configure_file variables
    set(ML_PACKAGE_NAME ${OMW_ADD_PACKAGE_NAME})
    set(ML_OUTPUT_NAME ${target_name})

    # Process MPREP sources
    set(ML_MPREP_TARGETS "")
    foreach(MPREP_SOURCE ${OMW_ADD_MPREP_SOURCES})
      # Get source name
      get_filename_component(MPREP_SOURCE_NAME ${MPREP_SOURCE} NAME)
      # Build target path
      set(MPREP_SOURCE_TARGET "${CMAKE_CURRENT_BINARY_DIR}/${MPREP_SOURCE_NAME}.c")
      # Prepare target
      Mathematica_WSTP_WSPREP_TARGET(${MPREP_SOURCE} OUTPUT ${MPREP_SOURCE_TARGET})
      # Add to list of MPREP sources
      list(APPEND ML_MPREP_TARGETS ${MPREP_SOURCE_TARGET})
    endforeach()

    # Add the executable target
    add_executable(${target_name}
      ${OMW_ADD_SOURCES}
      ${ML_MPREP_TARGETS})

    # C++ 14 required
    set_property(TARGET ${target_name} PROPERTY CXX_STANDARD 14)

    # Add its link libraries
    target_link_libraries(${target_name} ${OMW_ADD_LINK_LIBRARIES} omw_mathematica)

    # Add compile options
    target_compile_options(${target_name} PRIVATE ${OMW_ADD_COMPILE_OPTIONS})

    # Add compile definitions
    target_compile_definitions(${target_name} PRIVATE ${OMW_ADD_COMPILE_DEFINITIONS})

    # On Windows, create a graphical executable
    if(WIN32)
      set(TARGET_NEW_LINK_FLAGS "/SUBSYSTEM:WINDOWS")
      get_target_property(TARGET_LINK_FLAGS ${target_name} LINK_FLAGS)
      if(TARGET_LINK_FLAGS)
        set(TARGET_NEW_LINK_FLAGS "${TARGET_LINK_FLAGS} ${TARGET_NEW_LINK_FLAGS}")
      endif()
      set_target_properties(${target_name} PROPERTIES LINK_FLAGS ${TARGET_NEW_LINK_FLAGS})
    endif()

    # Create the install tree in the build directory
    if(OMW_ADD_TARGET_PACKAGE_DIR)
      set(PACKAGE_TARGET_DIR ${OMW_ADD_TARGET_PACKAGE_DIR}/${OMW_ADD_PACKAGE_NAME})
    else()
      set(PACKAGE_TARGET_DIR ${CMAKE_BINARY_DIR}/${OMW_ADD_PACKAGE_NAME})
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_TARGET_DIR}")

    # Configure main package file
    configure_file(${OMW_SRC_DIR}/mathematica/Package.m
      ${PACKAGE_TARGET_DIR}/${OMW_ADD_PACKAGE_NAME}.m @ONLY)

    # Configure Kernel/init.m
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_TARGET_DIR}/Kernel")
    configure_file(${OMW_SRC_DIR}/mathematica/Kernel/init.m
      ${PACKAGE_TARGET_DIR}/Kernel/init.m @ONLY)

    # Add the commands to copy the output to the package directory
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_TARGET_DIR}/${Mathematica_SYSTEM_ID}/"
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${target_name}>"
      "${PACKAGE_TARGET_DIR}/${Mathematica_SYSTEM_ID}/")

    # Make an installation target
    if(OMW_ADD_INSTALL)
      set(TARGET_EXCLUDE $<TARGET_FILE:${target_name}>)

      if(OMW_ADD_USER_INSTALL)
        set(INSTALL_DESTINATION ${Mathematica_USERBASE_DIR})
      else()
        set(INSTALL_DESTINATION ${Mathematica_BASE_DIR})
      endif()

      install(DIRECTORY ${PACKAGE_TARGET_DIR}
        DESTINATION ${INSTALL_DESTINATION}/Applications
        COMPONENT ${OMW_ADD_INSTALL}
        USE_SOURCE_PERMISSIONS
        PATTERN ${target_name} EXCLUDE)

      install(TARGETS ${target_name}
        RUNTIME DESTINATION ${INSTALL_DESTINATION}/Applications/${OMW_ADD_PACKAGE_NAME}/${Mathematica_SYSTEM_ID}
        COMPONENT ${OMW_ADD_INSTALL})
    endif()
  endfunction()
else()
  # Helper function to create a Mathematica target, but Mathematica is not available
  function(omw_add_mathematica target_name)
    message(STATUS "Skipping Mathematica target ${target_name}")
  endfunction()
endif()

# Octave library
if(Octave_FOUND OR OCTAVE_EXECUTABLE)
  add_library(omw_octave STATIC EXCLUDE_FROM_ALL
    ${OMW_SHARED_HEADERS}
    ${OMW_INCLUDE_DIR}/omw/octave.hpp
    ${OMW_SRC_DIR}/octave.cpp
    $<TARGET_OBJECTS:omw_base>)

  target_compile_definitions(omw_octave PUBLIC
    OMW_OCTAVE=1)
  target_include_directories(omw_octave PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${OCTAVE_INCLUDE_DIRS})

  set_shared_options(omw_octave)

  # We need to put OCTAVE_OCT_FILE_DIR in the CMakeCache because
  # omw_add_octave will be invoked from an outer scope
  set(OCTAVE_OCT_FILE_DIR ${OCTAVE_OCT_FILE_DIR}
    CACHE PATH "Octave module destination directory" FORCE)

  # Helper function to create an Octave target
  function(omw_add_octave target_name)
    cmake_parse_arguments(OMW_ADD "USER_INSTALL" "INSTALL;OCT_NAME;TARGET_PACKAGE_DIR" "SOURCES;LINK_LIBRARIES;COMPILE_OPTIONS;COMPILE_DEFINITIONS" ${ARGN})

    message(STATUS "Creating Octave target ${target_name}")

    octave_add_oct(${target_name}
      SOURCES ${OMW_ADD_SOURCES}
      LINK_LIBRARIES ${OMW_ADD_LINK_LIBRARIES} omw_octave)

    # Change name of oct file
    if(OMW_ADD_OCT_NAME)
      set_property(TARGET ${target_name} PROPERTY OUTPUT_NAME ${OMW_ADD_OCT_NAME})
    endif()

    # Set location of target file
    if(OMW_ADD_TARGET_PACKAGE_DIR)
      set(PACKAGE_TARGET_DIR ${OMW_ADD_TARGET_PACKAGE_DIR})
      add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_TARGET_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${target_name}>
          ${PACKAGE_TARGET_DIR})
    endif()

    # C++ 14 required
    set_property(TARGET ${target_name} PROPERTY CXX_STANDARD 14)

    # Add compile options
    target_compile_options(${target_name} PRIVATE ${OMW_ADD_COMPILE_OPTIONS})

    # Add compile definitions
    target_compile_definitions(${target_name} PRIVATE ${OMW_ADD_COMPILE_DEFINITIONS})

    # Create install target if requested
    if(OMW_ADD_INSTALL)
      if(OMW_ADD_USER_INSTALL)
        get_filename_component(INSTALL_DESTINATION_ARCH ${OCTAVE_OCT_FILE_DIR} NAME)
        set(INSTALL_DESTINATION $ENV{HOME}/.octave/oct/${INSTALL_DESTINATION_ARCH})
      else()
        set(INSTALL_DESTINATION ${OCTAVE_OCT_FILE_DIR})
      endif()

      install(TARGETS ${target_name}
        DESTINATION ${INSTALL_DESTINATION}
        COMPONENT ${OMW_ADD_INSTALL})
    endif()
  endfunction()
else()
  # Helper function to create an Octave target, but Octave is not available
  function(omw_add_octave target_name)
    message(STATUS "Skipping Octave target ${target_name}")
  endfunction()
endif()

if(OMW_DEVELOP_BUILD)
  set(OMW_TEST_SRC_DIR ${OMW_BASE_DIR}/test)
  set(OMW_T_DIR ${OMW_BASE_DIR}/t)

  # Note: we need to enable_testing here because
  # this relies on CMAKE_CURRENT_BINARY_DIR
  enable_testing()
  add_subdirectory(${OMW_TEST_SRC_DIR})
endif()

# Documentation
if(OMW_BUILD_DOCUMENTATION)
  add_subdirectory(${OMW_BASE_DIR}/docs)
endif()
