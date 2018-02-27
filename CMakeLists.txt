cmake_minimum_required(VERSION 3.1)

# Load Octave and Mathematica packages from cmake/
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(Mathematica COMPONENTS MathLink)
find_package(Octave)

# Set directories
set(OMW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(OMW_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
	CACHE PATH "Path to the omw sources")

# Mathematica library
if(Mathematica_FOUND)
	add_library(omw_mathematica STATIC
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
	set(MATHEMATICA_VARIABLES Mathematica_SYSTEM_ID Mathematica_USERBASE_DIR)
	foreach(VAR_NAME ${MATHEMATICA_VARIABLES})
		set(${VAR_NAME} ${${VAR_NAME}}
			CACHE PATH "Mathematica ${VAR_NAME}" FORCE)
	endforeach()

	# Helper function to create a Mathematica target
	function(omw_add_mathematica target_name)
		cmake_parse_arguments(OMW_ADD "" "PACKAGE_NAME;INSTALL" "MPREP_SOURCES;SOURCES;LINK_LIBRARIES;COMPILE_OPTIONS" ${ARGN})

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
		set(PACKAGE_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR}/${OMW_ADD_PACKAGE_NAME})
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
			COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${target_name}>" "${PACKAGE_TARGET_DIR}/${Mathematica_SYSTEM_ID}/")

		# Make an installation target
		if(OMW_ADD_INSTALL)
			install(DIRECTORY ${PACKAGE_TARGET_DIR}
					DESTINATION ${Mathematica_USERBASE_DIR}/Applications
					COMPONENT ${OMW_ADD_INSTALL}
					USE_SOURCE_PERMISSIONS)
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
	add_library(omw_octave STATIC
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
		cmake_parse_arguments(OMW_ADD "" "INSTALL" "SOURCES;LINK_LIBRARIES;COMPILE_OPTIONS" ${ARGN})

		message(STATUS "Creating Octave target ${target_name}")

		octave_add_oct(${target_name}
			SOURCES ${OMW_ADD_SOURCES}
			LINK_LIBRARIES ${OMW_ADD_LINK_LIBRARIES} omw_octave)

		# C++ 14 required
		set_property(TARGET ${target_name} PROPERTY CXX_STANDARD 14)

		# Add compile options
		target_compile_options(${target_name} PRIVATE ${OMW_ADD_COMPILE_OPTIONS})

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
