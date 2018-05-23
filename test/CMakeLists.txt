if(NOT Mathematica_FOUND)
  message(FATAL_ERROR "Trying to create the development build, but Wolfram Mathematica is not available")
endif()

if (NOT (Octave_FOUND OR OCTAVE_EXECUTABLE))
  message(FATAL_ERROR "Trying to create the development build, but Octave is not available")
endif()

message(STATUS "omw target directory: ${OMW_TARGET_DIR}")
message(STATUS "omw test source directory: ${OMW_TEST_SRC_DIR}")

omw_add_mathematica(omw_test_mathematica
  PACKAGE_NAME OMW
  MPREP_SOURCES ${OMW_TEST_SRC_DIR}/omw_test.tm
  SOURCES ${OMW_TEST_SRC_DIR}/omw_test.cpp
  TARGET_PACKAGE_DIR ${OMW_TARGET_DIR}
  COMPILE_OPTIONS "-Wall")

omw_add_octave(omw_test_octave
  OCT_NAME omw_tests
  SOURCES ${OMW_TEST_SRC_DIR}/omw_test.cpp
  TARGET_PACKAGE_DIR ${OMW_TARGET_DIR}
  COMPILE_OPTIONS "-Wall")

# Test procedures
file(GLOB TEST_FILES ${OMW_T_DIR}/*.t)
foreach(TEST_FILE ${TEST_FILES})
  get_filename_component(TEST_FILE_NAME ${TEST_FILE} NAME_WE)
  add_test(NAME ${TEST_FILE_NAME} COMMAND prove -v --color ${TEST_FILE} :: --path ${OMW_TARGET_DIR} --octave-pkg omw_tests --mathematica-pkg OMW
    WORKING_DIRECTORY ${OMW_TEST_SRC_DIR})
endforeach()

