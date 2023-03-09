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

set(CONAN_PROFILE "default" CACHE STRING "The profile to use for building conan deps, this is useful for cross compiling")

set(RNBO_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/cpp/" CACHE FILEPATH "The path to the the RNBO c++ source directory")
set(RNBO_CLASS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../../../codegen/xam/test/test.cpp" CACHE FILEPATH "The file that holds the generated RNBO class code")
set(RNBO_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../../../codegen/xam/test/description.json" CACHE FILEPATH "path to description.json")
set(RNBO_UNITY_INSTANCE_ACCESS_HACK ON CACHE BOOL "Do we provide the instance index hack as a parameter?")
set(RNBO_UNITY_IS_SPATIALIZER OFF CACHE BOOL "Do we expose this plugin as a Spatializer")

include(${RNBO_CPP_DIR}/cmake/RNBOConan.cmake)
include(${RNBO_CPP_DIR}/cmake/CCache.cmake)
include(${RNBO_CPP_DIR}/cmake/MinGWSTDThreads.cmake)

#generate description header
include(${RNBO_CPP_DIR}/cmake/RNBODescriptionHeader.cmake)
set(DESCRIPTION_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
rnbo_write_description_header_if_exists(${RNBO_DESCRIPTION_FILE} ${DESCRIPTION_INCLUDE_DIR})

#compute bundle id
STRING(REGEX REPLACE "[^A-Za-z0-9]" "" PLUGIN_NAME_ID ${PLUGIN_NAME})
STRING(REGEX REPLACE "[^A-Za-z0-9]" "" PLUGIN_MANUFACTURER_NAME_ID ${PLUGIN_MANUFACTURER_NAME})
STRING(TOLOWER  ${PLUGIN_NAME_ID} PLUGIN_NAME_LOWER_ID)
STRING(TOLOWER  ${PLUGIN_MANUFACTURER_NAME_ID} PLUGIN_MANUFACTURER_NAME_LOWER_ID)

SET(MACOSX_BUNDLE_GUI_IDENTIFIER "${PLUGIN_MANUFACTURER_TLD}.${PLUGIN_MANUFACTURER_NAME_ID}.${PLUGIN_NAME_ID}")

set(PACKAGE_DIR ${CMAKE_BINARY_DIR}/${PLUGIN_NAME_ID})

add_library(RNBOUnityPlugin MODULE)
set_target_properties(RNBOUnityPlugin PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PACKAGE_DIR}/Assets/Plugins/
	BUNDLE_EXTENSION bundle
	XCODE_ATTRIBUTE_WRAPPER_EXTENSION bundle
	BUNDLE TRUE
	XCODE_ATTRIBUTE_LIBRARY_STYLE Bundle
	XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE YES
	#plist
	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/src/Info.plist.in
	MACOSX_BUNDLE_BUNDLE_NAME ${PLUGIN_NAME}
	MACOSX_BUNDLE_GUI_IDENTIFIER ${MACOSX_BUNDLE_GUI_IDENTIFIER}
	MACOSX_BUNDLE_COPYRIGHT ${PLUGIN_MANUFACTURER_NAME}
	MACOSX_BUNDLE_LONG_VERSION_STRING ${PLUGIN_VERSION}
	MACOSX_BUNDLE_SHORT_VERSION_STRING ${PLUGIN_VERSION}
)

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

#write package.json
configure_file(${CMAKE_CURRENT_LIST_DIR}/src/package.json.in ${PACKAGE_DIR}/package.json)

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
set_target_properties(RNBOUnityPlugin PROPERTIES
	OUTPUT_NAME ${PLUGIN_NAME}
	XCODE_ATTRIBUTE_PRODUCT_NAME ${PLUGIN_NAME}
	)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set_target_properties(RNBOUnityPlugin PROPERTIES PREFIX "")
endif()

install(DIRECTORY ${PACKAGE_DIR} DESTINATION .)

