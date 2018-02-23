cmake_minimum_required(VERSION 3.1)

# Load Octave and Mathematica packages from cmake/
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(Octave)

set(OMW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(Mathematica_FOUND)
	add_library(omw_mathematica STATIC
		${CMAKE_CURRENT_SOURCE_DIR}/src/om_mathematica.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/om_wrapper_base.cpp)

	target_compile_definitions(omw_mathematica PUBLIC
		OMW_MATHEMATICA=1 OMW_INCLUDE_MAIN=1)
	target_include_directories(omw_mathematica PUBLIC
		${OMW_INCLUDE_DIR}
		${Mathematica_MathLink_INCLUDE_DIR})
	target_link_libraries(omw_mathematica INTERFACE
		${Mathematica_MathLink_LIBRARIES})
endif()

# Octave library
if(Octave_FOUND OR OCTAVE_EXECUTABLE)
	add_library(omw_octave STATIC
		${CMAKE_CURRENT_SOURCE_DIR}/src/om_octave.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/om_wrapper_base.cpp)

	target_compile_definitions(omw_octave PUBLIC
		OMW_OCTAVE=1)
	target_include_directories(omw_octave PUBLIC
		${OMW_INCLUDE_DIR}
		${OCTAVE_INCLUDE_DIRS})

	# -fPIC for .so
	set_property(TARGET omw_octave PROPERTY POSITION_INDEPENDENT_CODE ON)

	# C++ 14 required
	set_property(TARGET omw_octave PROPERTY CXX_STANDARD 14)

	# We need to put OCTAVE_OCT_FILE_DIR in the CMakeCache because
	# omw_add_octave will be invoked from an outer scope
	set(OCTAVE_OCT_FILE_DIR ${OCTAVE_OCT_FILE_DIR}
		CACHE PATH "Octave module destination directory" FORCE)

	# Helper function to create an Octave target
	function(omw_add_octave target_name)
		cmake_parse_arguments(OMW_ADD "" "INSTALL" "SOURCES;LINK_LIBRARIES" ${ARGN})

		message(STATUS "Creating Octave target ${target_name}")

		octave_add_oct(${target_name}
			SOURCES ${OMW_ADD_SOURCES}
			LINK_LIBRARIES ${OMW_ADD_LINK_LIBRARIES} omw_octave)

		# C++ 14 required
		set_property(TARGET ${target_name} PROPERTY CXX_STANDARD 14)

		# Create install target if requested
		if(OMW_ADD_INSTALL)
			install(TARGETS ${target_name}
					DESTINATION ${OCTAVE_OCT_FILE_DIR}
					COMPONENT ${OMW_ADD_INSTALL})
		endif()
	endfunction()
else()
	# Helper function to create an Octave target, but Octave is not available
	function(omw_add_octave target_name)
		message(STATUS "Skipping Octave target ${target_name}")
	endfunction()
endif()
