cmake_minimum_required(VERSION 3.22)

project(
	SQGLib
	VERSION 1.0.0
	LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

macro(set_from_environment VARIABLE)
	if(NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
		set(${VARIABLE} $ENV{${VARIABLE}})
	endif()
endmacro()

set_from_environment(Skyrim64Path)
if(NOT DEFINED Skyrim64Path)
	message(FATAL_ERROR "Skyrim64Path is not set")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/includes)

set(VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/includes/Plugin.h")
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/Plugin.h.in"
	"${VERSION_HEADER}"
	@ONLY
)

set(HEADERS
	${HEADER_DIR}/PCH.h
	${HEADER_DIR}/DialogUtils.h
	${HEADER_DIR}/QuestUtils.h
	${HEADER_DIR}/PackageUtils.h
	${HEADER_DIR}/Serialization/API.h
	${HEADER_DIR}/Serialization/Model.h
	${HEADER_DIR}/Serialization/FileSystem.h
	${HEADER_DIR}/Serialization/FormCreator.h
	${HEADER_DIR}/Serialization/FormRecord.h
	${HEADER_DIR}/Serialization/FormRecordSerializer.h
	${HEADER_DIR}/Serialization/FormSerializers.h
	${HEADER_DIR}/Serialization/SKSETmp.h
)

set(HPP
	${SOURCE_DIR}/Serialization/FileSystem.hpp
	${SOURCE_DIR}/Serialization/FormRecordSerializer.hpp
	${SOURCE_DIR}/Serialization/FormSerializers.hpp
)

set(SOURCES
	${SOURCE_DIR}/main.cpp
	${SOURCE_DIR}/DialogUtils.cpp
	${SOURCE_DIR}/PackageUtils.cpp
	${SOURCE_DIR}/QuestUtils.cpp
	${SOURCE_DIR}/Serialization/API.cpp
	${SOURCE_DIR}/Serialization/FileSystem.cpp
	${SOURCE_DIR}/Serialization/FormCreator.cpp
	${SOURCE_DIR}/Serialization/FormRecord.cpp
	${SOURCE_DIR}/Serialization/Model.cpp
	${HPP}
)

set_source_files_properties(${HPP} PROPERTIES HEADER_FILE_ONLY TRUE)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/version.rc"
	@ONLY
)

file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Template.ChildProcessDbgSettings ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.ChildProcessDbgSettings)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source Files" FILES ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/includes PREFIX "Header Files" FILES ${HEADERS})

add_library(
	"${PROJECT_NAME}"
	SHARED
	${HEADERS}
	${SOURCES}
	"${VERSION_HEADER}"
	"${CMAKE_CURRENT_BINARY_DIR}/version.rc"
)

set_property(DIRECTORY
	PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}"
)

set_target_properties("${PROJECT_NAME}"
    PROPERTIES
		VS_DEBUGGER_COMMAND J:/100Install/mo2/ModOrganizer.exe
		VS_DEBUGGER_COMMAND_ARGUMENTS  "-p \"SQGLib\" \"moshortcut://SkyrimSE:Skyrim Special Edition\""
)

target_compile_features(
	"${PROJECT_NAME}"
	PRIVATE
		cxx_std_20
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	target_compile_options(
		"${PROJECT_NAME}"
		PRIVATE
			"/sdl"	# Enable Additional Security Checks
			"/utf-8"	# Set Source and Executable character sets to UTF-8
			"/Zi"	# Debug Information Format

			"/permissive-"	# Standards conformance
			"/Zc:preprocessor"	# Enable preprocessor conformance mode

			"/wd4200" # nonstandard extension used : zero-sized array in struct/union

			"$<$<CONFIG:DEBUG>:>"
			"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
	)

	target_link_options(
		"${PROJECT_NAME}"
		PRIVATE
			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)
endif()

target_include_directories(
	"${PROJECT_NAME}"
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/includes
		${SOURCE_DIR}
		${HEADER_DIR}
)	

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/CommonLibSSE" CommonLibSSE EXCLUDE_FROM_ALL)

target_link_libraries(
	"${PROJECT_NAME}"
	PRIVATE
		CommonLibSSE::CommonLibSSE
)

target_precompile_headers(
	"${PROJECT_NAME}"
	PRIVATE
		${HEADER_DIR}/PCH.h
)	

install(
	FILES
		"$<TARGET_FILE:${PROJECT_NAME}>"
	DESTINATION "SKSE/Plugins"
	COMPONENT "main"
)

install(
	FILES
		"$<TARGET_PDB_FILE:${PROJECT_NAME}>"
	DESTINATION "/"
	COMPONENT "pdbs"
)

add_custom_command(
	TARGET "${PROJECT_NAME}"
	POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_CURRENT_SOURCE_DIR}/Data/SKSE/Plugins/"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${CMAKE_CURRENT_SOURCE_DIR}/Data/SKSE/Plugins/"
	VERBATIM
)

include(cmake/packaging.cmake)
