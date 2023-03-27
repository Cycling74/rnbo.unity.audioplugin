cmake_minimum_required(VERSION 3.18)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release.")
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "")

project(RNBO_UNITY_NATIVE_AUDIO_PLUGIN
	VERSION 0.0.1
	LANGUAGES CXX
)

#use 17 so we have shared_mutex
set (CMAKE_CXX_STANDARD 17)

set(PLUGIN_NAME "RNBOPlugin" CACHE STRING "The name of the audio plugin")
set(PLUGIN_VERSION "0.0.1" CACHE STRING "Plugin version string")
set(PLUGIN_MANUFACTURER_NAME "Cycling '74" CACHE STRING "Specify the name of the plugin's author")
set(PLUGIN_MANUFACTURER_TLD "com" CACHE STRING "the Top Level Domain for your company's website URL")

set(RNBO_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/export/rnbo/" CACHE PATH "The path to the the RNBO c++ source directory")
set(RNBO_EXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/export/" CACHE PATH "The directory that holds the generated RNBO exports")

set(RNBO_CLASS_FILE_NAME "rnbo_source.cpp" CACHE STRING "the name of your rnbo class file")
mark_as_advanced(RNBO_CLASS_FILE_NAME)

set(RNBO_UNITY_INSTANCE_ACCESS_HACK ON CACHE BOOL "Do we provide the instance index hack as a parameter?")
set(RNBO_UNITY_IS_SPATIALIZER OFF CACHE BOOL "Do we expose this plugin as a Spatializer")

set(RNBO_CLASS_FILE ${RNBO_EXPORT_DIR}/${RNBO_CLASS_FILE_NAME})
set(RNBO_DESCRIPTION_FILE ${RNBO_EXPORT_DIR}/description.json)
set(RNBO_PRESETS_FILE ${RNBO_EXPORT_DIR}/presets.json)

include(${RNBO_CPP_DIR}/cmake/CCache.cmake)
include(${RNBO_CPP_DIR}/cmake/MinGWSTDThreads.cmake)

#generate description header
include(${RNBO_CPP_DIR}/cmake/RNBODescriptionHeader.cmake)
set(DESCRIPTION_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
rnbo_write_description_header_if_exists(${RNBO_DESCRIPTION_FILE} ${DESCRIPTION_INCLUDE_DIR} ${RNBO_PRESETS_FILE})

#compute bundle id
STRING(REGEX REPLACE "[^A-Za-z0-9]" "" PLUGIN_NAME_ID ${PLUGIN_NAME})
STRING(REGEX REPLACE "[^A-Za-z0-9]" "" PLUGIN_MANUFACTURER_NAME_ID ${PLUGIN_MANUFACTURER_NAME})
STRING(TOLOWER  ${PLUGIN_NAME_ID} PLUGIN_NAME_LOWER_ID)
STRING(TOLOWER  ${PLUGIN_MANUFACTURER_NAME_ID} PLUGIN_MANUFACTURER_NAME_LOWER_ID)

set(PACKAGE_DIR ${CMAKE_BINARY_DIR}/${PLUGIN_NAME_ID})

# Test platform and 
if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL "RNBOUnityPlugin currently only supports 64-bit platforms")
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  set(PLUGIN_UNITY_PLATFORM_NAME "LinuxStandalone64")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  set(PLUGIN_UNITY_PLATFORM_NAME "WindowsStandalone64")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  set(PLUGIN_UNITY_PLATFORM_NAME "macOSStandalone")
else()
  message(FATAL "${CMAKE_SYSTEM_NAME} not yet supported for RNBOUnityPlugin")
endif()

set(PLUGIN_DIR ${PACKAGE_DIR}/Assets/Plugins/${CMAKE_SYSTEM_NAME}/)

add_library(RNBOUnityPlugin MODULE)

set_target_properties(RNBOUnityPlugin PROPERTIES
  #generic
  LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_DIR}
	OUTPUT_NAME ${PLUGIN_NAME}

  #macos
	BUNDLE TRUE
	BUNDLE_EXTENSION bundle
	XCODE_ATTRIBUTE_WRAPPER_EXTENSION bundle
	XCODE_ATTRIBUTE_LIBRARY_STYLE Bundle
	XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE YES
	XCODE_ATTRIBUTE_PRODUCT_NAME ${PLUGIN_NAME}

	#plist
	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/src/Info.plist.in
	MACOSX_BUNDLE_BUNDLE_NAME ${PLUGIN_NAME}
	MACOSX_BUNDLE_GUI_IDENTIFIER ${PLUGIN_MANUFACTURER_TLD}.${PLUGIN_MANUFACTURER_NAME_ID}.${PLUGIN_NAME_ID}
	MACOSX_BUNDLE_COPYRIGHT ${PLUGIN_MANUFACTURER_NAME}
	MACOSX_BUNDLE_LONG_VERSION_STRING ${PLUGIN_VERSION}
	MACOSX_BUNDLE_SHORT_VERSION_STRING ${PLUGIN_VERSION}
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set_target_properties(RNBOUnityPlugin PROPERTIES PREFIX "")
endif()


target_sources(RNBOUnityPlugin
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/RNBOWrapper.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/AudioPluginUtil.cpp
	${RNBO_CLASS_FILE}
	${RNBO_CPP_DIR}/RNBO.cpp
)

target_include_directories(RNBOUnityPlugin
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/
	${RNBO_CPP_DIR}/
	${RNBO_CPP_DIR}/common/
	${RNBO_CPP_DIR}/adapters/unity/native_audio_plugin
	${DESCRIPTION_INCLUDE_DIR}
	${RNBO_CPP_DIR}/src/3rdparty/
)

#configure helper
set(HELPER_OUTPUT_DIR ${PACKAGE_DIR}/Assets/Scripts/)
set(HELPER_NAME ${PLUGIN_NAME_ID}Helper.cs)
configure_file(${CMAKE_CURRENT_LIST_DIR}/src/Helper.cs.in ${HELPER_OUTPUT_DIR}/${HELPER_NAME})
configure_file(${CMAKE_CURRENT_LIST_DIR}/src/Helper.cs.asmdef.in ${HELPER_OUTPUT_DIR}/${HELPER_NAME}.asmdef)

#write package.json
configure_file(${CMAKE_CURRENT_LIST_DIR}/src/package.json.in ${PACKAGE_DIR}/package.json)

#write plugin assembly
set(PLUGIN_ASSEMBLY_NAME "${PLUGIN_MANUFACTURER_NAME_ID}.${PLUGIN_UNITY_PLATFORM_NAME}.${PLUGIN_NAME_ID}")
configure_file(${CMAKE_CURRENT_LIST_DIR}/src/Plugin.asmdef.in ${PLUGIN_DIR}/${PLUGIN_ASSEMBLY_NAME}.asmdef)

set(INSTANCE_ACCESS_HACK 0)
if (RNBO_UNITY_INSTANCE_ACCESS_HACK)
	set(INSTANCE_ACCESS_HACK 1)
endif()

set(SPATIALIZER 0)
if(RNBO_UNITY_IS_SPATIALIZER)
	set(SPATIALIZER 1)
endif()


target_compile_definitions(RNBOUnityPlugin
	PRIVATE
	PLUGIN_NAME="${PLUGIN_NAME}"
	RNBO_UNITY_INSTANCE_ACCESS_HACK=${INSTANCE_ACCESS_HACK}
	PLUGIN_IS_SPATIALIZER=${SPATIALIZER}
	RNBO_DESCRIPTION_AS_STRING=1 #we don't create a json object, we just create a const string to pass over to csharp
)

if (BUILD_SYSTEM_IS_MINGW)
	target_link_libraries(RNBOUnityPlugin
		PRIVATE
		mingw_stdthreads
		-static-libgcc
		-static-libstdc++
	)
endif()

install(DIRECTORY ${PACKAGE_DIR} DESTINATION .)

