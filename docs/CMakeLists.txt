find_package(Doxygen)
cmake_dependent_option(OMW_BUILD_DOCS
  "Build the HTML docs" ON
  "DOXYGEN_FOUND;OMW_BUILD_DOCUMENTATION" OFF)

if(OMW_BUILD_DOCS)
  message(STATUS "Adding documentation target")

  set(doxy_main_page ${OMW_BASE_DIR}/README.md)
  set(doxyfile_in ${OMW_BASE_DIR}/docs/Doxyfile.in)
  set(doxyfile ${OMW_TARGET_DIR}/Doxyfile)

  configure_file(${doxyfile_in} ${doxyfile} @ONLY)

  add_custom_target(omw_doc ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
    WORKING_DIRECTORY ${OMW_TARGET_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)

  #install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
  #  COMPONENT omw-docs
  #  DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()
