cmake_minimum_required(VERSION 3.1)

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

if(Octave_FOUND OR OCTAVE_EXECUTABLE)
	add_library(omw_octave STATIC
		${CMAKE_CURRENT_SOURCE_DIR}/src/om_octave.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/om_wrapper_base.cpp)

	target_compile_definitions(omw_octave PUBLIC
		OMW_OCTAVE=1)
	target_include_directories(omw_octave PUBLIC
		${OMW_INCLUDE_DIR}
		${OCTAVE_INCLUDE_DIRS})
	set_property(TARGET omw_octave PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()
