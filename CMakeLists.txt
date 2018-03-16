cmake_minimum_required(VERSION 3.1)

include(CMakeDependentOption)
include(GNUInstallDirs)

project(omw)

# Load Octave and Mathematica packages from cmake/
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(Mathematica COMPONENTS MathLink)
find_package(Octave)

# Set directories
set(OMW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(OMW_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
  CACHE PATH "Path to the omw sources")

option(OMW_DEVELOP_BUILD "Enable building the development library" OFF)
option(OMW_BUILD_DOCS "Build the documentation of OMW using Doxygen" OFF)

# Mathematica library
if(Mathematica_FOUND)
  add_library(omw_mathematica STATIC EXCLUDE_FROM_ALL
    ${OMW_SRC_DIR}/om_mathematica.cpp
    ${OMW_SRC_DIR}/om_wrapper_base.cpp)

  target_compile_definitions(omw_mathematica PUBLIC
    OMW_MATHEMATICA=1 OMW_INCLUDE_MAIN=1)
  target_include_directories(omw_mathematica PUBLIC
    ${OMW_INCLUDE_DIR}
    ${Mathematica_MathLink_INCLUDE_DIR})
  target_link_libraries(omw_mathematica INTERFACE
    ${Mathematica_MathLink_LIBRARIES})

  # C++ 14 required
  set_property(TARGET omw_mathematica PROPERTY CXX_STANDARD 14)

  # -Wall
  target_compile_options(omw_mathematica PRIVATE "-Wall")

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
      "MPREP_SOURCES;SOURCES;LINK_LIBRARIES;COMPILE_OPTIONS" ${ARGN})

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
      Mathematica_MathLink_MPREP_TARGET(${MPREP_SOURCE} OUTPUT ${MPREP_SOURCE_TARGET})
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
    ${OMW_SRC_DIR}/om_octave.cpp
    ${OMW_SRC_DIR}/om_wrapper_base.cpp)

  target_compile_definitions(omw_octave PUBLIC
    OMW_OCTAVE=1)
  target_include_directories(omw_octave PUBLIC
    ${OMW_INCLUDE_DIR}
    ${OCTAVE_INCLUDE_DIRS})

  # -fPIC for .so
  set_property(TARGET omw_octave PROPERTY POSITION_INDEPENDENT_CODE ON)

  # C++ 14 required
  set_property(TARGET omw_octave PROPERTY CXX_STANDARD 14)

  # -Wall
  target_compile_options(omw_octave PRIVATE "-Wall")

  # We need to put OCTAVE_OCT_FILE_DIR in the CMakeCache because
  # omw_add_octave will be invoked from an outer scope
  set(OCTAVE_OCT_FILE_DIR ${OCTAVE_OCT_FILE_DIR}
    CACHE PATH "Octave module destination directory" FORCE)

  # Helper function to create an Octave target
  function(omw_add_octave target_name)
    cmake_parse_arguments(OMW_ADD "USER_INSTALL" "INSTALL;OCT_NAME" "SOURCES;LINK_LIBRARIES;COMPILE_OPTIONS" ${ARGN})

    message(STATUS "Creating Octave target ${target_name}")

    octave_add_oct(${target_name}
      SOURCES ${OMW_ADD_SOURCES}
      LINK_LIBRARIES ${OMW_ADD_LINK_LIBRARIES} omw_octave)

    # Change name of oct file
    if(OMW_ADD_OCT_NAME)
      set_property(TARGET ${target_name} PROPERTY OUTPUT_NAME ${OMW_ADD_OCT_NAME})
    endif()

    # C++ 14 required
    set_property(TARGET ${target_name} PROPERTY CXX_STANDARD 14)

    # Add compile options
    target_compile_options(${target_name} PRIVATE ${OMW_ADD_COMPILE_OPTIONS})

    # Create install target if requested
    if(OMW_ADD_INSTALL)
      if(OMW_ADD_USER_INSTALL)
        set(INSTALL_DESTINATION ${OCTAVE_OCT_FILE_DIR})
      else()
        get_filename_component(INSTALL_DESTINATION_ARCH ${OCTAVE_OCT_FILE_DIR} NAME)
        set(INSTALL_DESTINATION $ENV{HOME}/.octave/oct/${INSTALL_DESTINATION_ARCH})
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
  if(NOT Mathematica_FOUND)
    message(FATAL_ERROR "Trying to create the development build, but Wolfram Mathematica is not available")
  endif()

  if (NOT (Octave_FOUND OR OCTAVE_EXECUTABLE))
    message(FATAL_ERROR "Trying to create the development build, but Octave is not available")
  endif()

  omw_add_mathematica(omw_test_mathematica
    PACKAGE_NAME OMW
    MPREP_SOURCES ${OMW_SRC_DIR}/omw_test.tm
    SOURCES ${OMW_SRC_DIR}/omw_test.cpp)

  omw_add_octave(omw_test_octave
    OCT_NAME omw_tests
    SOURCES ${OMW_SRC_DIR}/omw_test.cpp)

  # Test procedures
  enable_testing()
  file(GLOB TEST_FILES ${CMAKE_SOURCE_DIR}/t/*.t)
  foreach(TEST_FILE ${TEST_FILES})
    get_filename_component(TEST_FILE_NAME ${TEST_FILE} NAME_WE)
    add_test(NAME ${TEST_FILE_NAME} COMMAND prove -v --color ${TEST_FILE} :: --path ${CMAKE_CURRENT_BINARY_DIR} --octave-pkg omw_tests --mathematica-pkg OMW
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  endforeach()
endif()

# Documentation
find_package(Doxygen)
cmake_dependent_option(OMW_BUILD_DOCS
  "Build the HTML docs" ON
  "DOXYGEN_FOUND;OMW_BUILD_DOCUMENTATION" OFF)

if(OMW_BUILD_DOCS)
  message(STATUS "Adding documentation target")

  set(doxy_main_page ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  configure_file(${doxyfile_in} ${doxyfile} @ONLY)

  add_custom_target(omw_doc ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)

  #install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
  #  COMPONENT omw-docs
  #  DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()

