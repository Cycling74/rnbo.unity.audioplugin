cmake_minimum_required(VERSION 3.18)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release.")
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "")

project(RNBO_UNITY_NATIVE_AUDIO_PLUGIN
	VERSION 0.0.2
	LANGUAGES CXX
)

#use 17 so we have shared_mutex
set (CMAKE_CXX_STANDARD 17)
#didn't become available until the 10.12 osx sdk
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)

#support default in either standalone or rnbo adapters/
set(DEFAULT_CPP_DIR "${CMAKE_CURRENT_LIST_DIR}/export/rnbo/")
set(PKG_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(IS_ADAPTER_BUILD Off)
if (EXISTS "${PKG_CPP_DIR}/RNBO.h")
	set(DEFAULT_CPP_DIR "${PKG_CPP_DIR}/")
	set(IS_ADAPTER_BUILD On)
endif()

set(PLUGIN_NAME "RNBOPlugin" CACHE STRING "The name of the audio plugin")
set(PLUGIN_VERSION "0.0.1" CACHE STRING "Plugin version string")
set(PLUGIN_MANUFACTURER_NAME "Cycling '74" CACHE STRING "Specify the name of the plugin's author")
set(PLUGIN_MANUFACTURER_TLD "com" CACHE STRING "the Top Level Domain for your company's website URL")

set(RNBO_CPP_DIR ${DEFAULT_CPP_DIR} CACHE PATH "The path to the the RNBO c++ source directory")
set(RNBO_EXPORT_DIR "${CMAKE_CURRENT_LIST_DIR}/export/" CACHE PATH "The directory that holds the generated RNBO exports")

set(RNBO_CLASS_FILE_NAME "rnbo_source.cpp" CACHE STRING "the name of your rnbo class file")
mark_as_advanced(RNBO_CLASS_FILE_NAME)

set(RNBO_UNITY_INSTANCE_ACCESS_HACK ON CACHE BOOL "Do we provide the instance index hack as a parameter?")
set(RNBO_UNITY_IS_SPATIALIZER OFF CACHE BOOL "Do we expose this plugin as a Spatializer")

set(RNBO_CLASS_FILE ${RNBO_EXPORT_DIR}/${RNBO_CLASS_FILE_NAME})
set(RNBO_DESCRIPTION_FILE ${RNBO_EXPORT_DIR}/description.json)
set(RNBO_PRESETS_FILE ${RNBO_EXPORT_DIR}/presets.json)

#generate documentation without the need to actually have rnbo source etc
set(DOCUMENTATION_ONLY Off CACHE BOOL "Shall we only generate documentation?")
mark_as_advanced(DOCUMENTATION_ONLY)

#compute bundle id
STRING(REGEX REPLACE "[^A-Za-z0-9]" "" PLUGIN_NAME_ID ${PLUGIN_NAME})
STRING(REGEX REPLACE "[^A-Za-z0-9]" "" PLUGIN_MANUFACTURER_NAME_ID ${PLUGIN_MANUFACTURER_NAME})
STRING(TOLOWER ${PLUGIN_NAME_ID} PLUGIN_NAME_LOWER_ID)
STRING(TOLOWER ${PLUGIN_MANUFACTURER_NAME_ID} PLUGIN_MANUFACTURER_NAME_LOWER_ID)

set(PACKAGE_DIR ${CMAKE_BINARY_DIR}/${PLUGIN_NAME_ID})
set(DOXYGEN_OUTPUT_DIRECTORY ${PACKAGE_DIR}/Documentation/)

# Test platform and bit depth
set(BIT_DEPTH 64)
if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(BIT_DEPTH 32)
endif()

#configure helper
set(HELPER_OUTPUT_DIR ${PACKAGE_DIR}/Assets/Scripts/)
set(HELPER_NAME ${PLUGIN_NAME_ID}Helper.cs)
configure_file(${CMAKE_CURRENT_LIST_DIR}/src/Helper.cs.in ${HELPER_OUTPUT_DIR}/${HELPER_NAME})
configure_file(${CMAKE_CURRENT_LIST_DIR}/src/Helper.cs.asmdef.in ${HELPER_OUTPUT_DIR}/${HELPER_NAME}.asmdef)

if (NOT DOCUMENTATION_ONLY)
	include(${RNBO_CPP_DIR}/cmake/CCache.cmake)

	if (IS_ADAPTER_BUILD)
		include(${RNBO_CPP_DIR}/cmake/MinGWSTDThreads.cmake)
	endif()

	#generate description header
	include(${RNBO_CPP_DIR}/cmake/RNBODescriptionHeader.cmake)
	set(DESCRIPTION_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
	rnbo_write_description_header_if_exists(${RNBO_DESCRIPTION_FILE} ${DESCRIPTION_INCLUDE_DIR} ${RNBO_PRESETS_FILE})

	#platforms:
	#macOSStandalone
	#WindowsStandalone32
	#WindowsStandalone64
	#LinuxStandalone32
	#LinuxStandalone64
	#LinuxStandaloneUniversal
	#iOS
	#Android
	#WebGL
	#WSA
	#Tizen
	#PSVita
	#PS4
	#PSMobile
	#XboxOne
	#Nintendo3DS
	#WiiU
	#tvOS
	#Switch

	set(PLUGIN_DIR ${PACKAGE_DIR}/Assets/Plugins/${CMAKE_SYSTEM_NAME}/)

	#settings for plugin .meta file
	set(PLUGIN_UNITY_CPU ${CMAKE_SYSTEM_PROCESSOR})
	if (${PLUGIN_UNITY_CPU} STREQUAL aarch64)
		set(PLUGIN_UNITY_CPU "ARM64")
	elseif (${PLUGIN_UNITY_CPU} STREQUAL armv7-a)
		set(PLUGIN_UNITY_CPU "ARMv7")
	endif()

	set(UNITY_EXCLUDE_ANDROID 1)
	set(UNITY_EXCLUDE_EDITOR 0)
	set(UNITY_EXCLUDE_LINUX64 1)
	set(UNITY_EXCLUDE_OSX 1)
	set(UNITY_EXCLUDE_WIN32 1)
	set(UNITY_EXCLUDE_WIN64 1)

	set(UNITY_ENABLE_ANDROID 0)
	set(UNITY_ENABLE_LINUX64 0)
	set(UNITY_ENABLE_OSX 0)
	set(UNITY_ENABLE_WIN32 0)
	set(UNITY_ENABLE_WIN64 0)

	set(UNITY_ARM_CPU ARMv7)
	set(UNITY_EDITOR_OS AnyOS)

	set(GENERATE_META On)

	if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
		if (${BIT_DEPTH} EQUAL 32)
			set(GENERATE_META Off)
		endif()
		set(PLUGIN_UNITY_PLATFORM_NAME "LinuxStandalone${BIT_DEPTH}")
		set(PLUGIN_META_PATH ${PLUGIN_DIR}/lib${PLUGIN_NAME_ID}.so.meta)
		set(UNITY_EXCLUDE_LINUX64 0)
		set(UNITY_ENABLE_LINUX64 1)
		set(UNITY_EDITOR_OS Linux)
	elseif (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
		set(PLUGIN_UNITY_PLATFORM_NAME "WindowsStandalone${BIT_DEPTH}")
		set(PLUGIN_META_PATH ${PLUGIN_DIR}/${PLUGIN_NAME_ID}.dll.meta)
		if (${BIT_DEPTH} STREQUAL 32)
			set(UNITY_EXCLUDE_WIN32 0)
			set(UNITY_ENABLE_WIN32 1)
		else ()
			set(UNITY_EXCLUDE_WIN64 0)
			set(UNITY_ENABLE_WIN64 1)
		endif()
		set(UNITY_EDITOR_OS Windows)
	elseif (${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
		set(PLUGIN_UNITY_PLATFORM_NAME "macOSStandalone")
		set(PLUGIN_META_PATH ${PLUGIN_DIR}/${PLUGIN_NAME_ID}.bundle.meta)
		set(PLUGIN_UNITY_CPU "AnyCPU")
		set(UNITY_EXCLUDE_OSX 0)
		set(UNITY_ENABLE_OSX 1)
		set(UNITY_EDITOR_OS OSX)
	elseif (${CMAKE_SYSTEM_NAME} STREQUAL Android)
		set(PLUGIN_UNITY_PLATFORM_NAME "Android")
		set(PLUGIN_DIR ${PLUGIN_DIR}/${ANDROID_ABI}/)
		set(PLUGIN_META_PATH ${PLUGIN_DIR}/lib${PLUGIN_NAME_ID}.so.meta)
		set(UNITY_EXCLUDE_ANDROID 0)
		set(UNITY_ENABLE_ANDROID 1)
		set(UNITY_EXCLUDE_EDITOR 1)
		set(UNITY_ARM_CPU ${PLUGIN_UNITY_CPU})
	else()
		set(GENERATE_META Off)
		message(FATAL "${CMAKE_SYSTEM_NAME} not yet supported for RNBOUnityPlugin")
	endif()

	if (${UNITY_EXCLUDE_EDITOR} EQUAL 0)
		set(UNITY_ENABLE_EDITOR 1)
	else()
		set(UNITY_ENABLE_EDITOR 0)
	endif()

	message(STATUS "building for ${PLUGIN_UNITY_PLATFORM_NAME} with bit depth ${BIT_DEPTH} for CPU ${PLUGIN_UNITY_CPU}")

	add_library(RNBOUnityPlugin MODULE)

	set_target_properties(RNBOUnityPlugin PROPERTIES
		#generic
		LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_DIR}
		LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PLUGIN_DIR}
		LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PLUGIN_DIR}
		OUTPUT_NAME ${PLUGIN_NAME_ID}

		#macos
		BUNDLE TRUE
		BUNDLE_EXTENSION bundle
		XCODE_ATTRIBUTE_WRAPPER_EXTENSION bundle
		XCODE_ATTRIBUTE_LIBRARY_STYLE Bundle
		XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE YES
		XCODE_ATTRIBUTE_PRODUCT_NAME ${PLUGIN_NAME}

		#plist
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/src/Info.plist.in
		MACOSX_BUNDLE_BUNDLE_NAME ${PLUGIN_NAME_ID}
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

	#write package.json
	configure_file(${CMAKE_CURRENT_LIST_DIR}/src/package.json.in ${PACKAGE_DIR}/package.json)

	if (GENERATE_META)
		#write plugin meta, namespace is just a guid alex produced online
		string(UUID PLUGIN_GUID NAMESPACE afee6755-c498-42a2-b023-6188a76c942c NAME ${PLUGIN_NAME_ID} TYPE SHA1)
		STRING(REPLACE "-" "" PLUGIN_GUID ${PLUGIN_GUID})
		configure_file(${CMAKE_CURRENT_LIST_DIR}/src/plugin.meta.in ${PLUGIN_META_PATH})
	endif()

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
		#mingw_stdthreads doesn't have shared_lock
		target_compile_definitions(RNBOUnityPlugin
			PRIVATE
			NO_SHARED_LOCK
		)
		target_link_libraries(RNBOUnityPlugin
			PRIVATE
			mingw_stdthreads
			-static-libgcc
			-static-libstdc++
		)
	endif()

else()
	set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Documentation/)
endif()

find_package(Doxygen)
if (DOXYGEN_FOUND)
	set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
	set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
	configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT})

	add_custom_target(RNBOUnityPluginDocs ALL
		COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen"
		VERBATIM)
endif()

install(DIRECTORY ${PACKAGE_DIR} DESTINATION .)

