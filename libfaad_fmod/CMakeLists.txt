cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)


project(libfaad_fmod
    VERSION 0.9.0
    LANGUAGES C CXX
)

set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE})

if(DEFINED CUSTOM_DEFINES)
    set(CUSTOM_DEFINES ${CUSTOM_DEFINES})
    foreach(definition ${CUSTOM_DEFINES})
        add_definitions(-D${definition})
    endforeach()
endif()


if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR "this only builds on macOS")
endif()

include_directories(src/includes)


add_library(faad_fmod
    src/main.cpp
)

target_link_libraries(faad_fmod 
    faad_fixed
)

# target_link_libraries(${LIBRARY_OUT} "${FAAD_PATH}/libfaad_drm_2.11.1.dylib")

# if(ANDROID)
#     #add_library(${LIBRARY_OUT} SHARED "${SOURCE_FILES}")
#     install(TARGETS ${LIBRARY_OUT} DESTINATION ${OUTPUT_DIR}/Android/${ANDROID_ABI})
# elseif(IOS)
#     # add_definitions(-D_TARGET_IOS_=1)
#     # set(SDK_NAME iphoneos)    
#     # set(CMAKE_IOS_INSTALL_UNIVERSAL_LIBS ON)
#     # set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH OFF)
#     # set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos")
#     # set(CMAKE_OSX_ARCHITECTURES "arm64") # strangely, this is the way to do arm64
#     # set(IOS_ARCH                "arm64") # needed?

#     # # Setup iOS developer location unless specified manually with CMAKE_IOS_DEVELOPER_ROOT
#     # set(CMAKE_IOS_SDK_ROOT "${APPLE_PLATFORMS_PATH}/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk")
#     # if (NOT DEFINED CMAKE_IOS_SDK_ROOT)
#     #     set(CMAKE_IOS_SDK_ROOT "")
#     # endif()
    
#     # if (CMAKE_IOS_SDK_ROOT STREQUAL "")
#     #     exec_program(/usr/bin/xcode-select ARGS -print-path OUTPUT_VARIABLE CMAKE_XCODE_DEVELOPER_DIR)
#     #     set (IOS_SDK_ROOT "${CMAKE_XCODE_DEVELOPER_DIR}/Platforms/iPhoneOS.platform/Developer")
#     #     message(STATUS "IOS_SDK_ROOT:${IOS_SDK_ROOT}")
          
#     #     # Scan the SDK dir and choose the latest SDK
#     #     file(GLOB _CMAKE_IOS_SDKS "${IOS_SDK_ROOT}/SDKs/*")
#     #     if(_CMAKE_IOS_SDKS)
#     #         list(SORT    _CMAKE_IOS_SDKS)
#     #         list(REVERSE _CMAKE_IOS_SDKS)
#     #         list(GET     _CMAKE_IOS_SDKS 0 CMAKE_IOS_SDK_ROOT)
#     #     else(_CMAKE_IOS_SDKS)
#     #         message(FATAL_ERROR "No iOS SDK's found in default search path ${CMAKE_IOS_DEVELOPER_ROOT}. Manually set CMAKE_IOS_SDK_ROOT or install the iOS SDK.")
#     #     endif(_CMAKE_IOS_SDKS)
#     #     message(STATUS "Toolchain using default iOS SDK: ${CMAKE_IOS_SDK_ROOT}")
#     # endif()
    
#     # set(CMAKE_OSX_SYSROOT "${CMAKE_IOS_SDK_ROOT}")
#     # set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")    
    
#     # add_library(${LIBRARY_OUT} STATIC ${SOURCE_FILES})
#     # install(TARGETS ${LIBRARY_OUT} DESTINATION ${OUTPUT_DIR}/iOS)
    
# elseif(WEBGL)
#     # set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   --em-config ${EM_CONFIG}")
#     # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --em-config ${EM_CONFIG}")
#     # add_library(${LIBRARY_OUT} SHARED "${SOURCE_FILES}")
#     # set_target_properties(${LIBRARY_OUT} PROPERTIES PREFIX "" SUFFIX ".bc")
#     # install(TARGETS ${LIBRARY_OUT} DESTINATION ${OUTPUT_DIR}/WebGL)
# elseif(OSX)

#     # In the OSX12 SDK, old school libs are no longer available, so we use the 11.3 for now:
#     #    https://discourse.cmake.org/t/cmake-not-working-after-update-do-big-sur/2546/14
#     # To reset cli tools:
#     #    sudo rm -rf /Library/Developer/CommandLineTools
#     #    xcode-select --install
#     #set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14") #"11.3")
#     #set(CMAKE_OSX_SYSROOT "/${APPLE_PLATFORMS_PATH}/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
#     #set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
    
#     set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
  
#     #add_library(${LIBRARY_OUT} MODULE "${SOURCE_FILES}")
#     set_target_properties(${LIBRARY_OUT} PROPERTIES BUNDLE TRUE)
#     install(TARGETS ${LIBRARY_OUT} DESTINATION "${OUTPUT_DIR}/OSX")
# elseif(WINDOWS)
#     add_definitions(-D_TARGET_WIN_=1)
#     add_library(${LIBRARY_OUT} MODULE "${SOURCE_FILES}")
#     install(TARGETS ${LIBRARY_OUT} DESTINATION "${OUTPUT_DIR}/Windows/${ARCH}")
# elseif(UWP)
#     add_library(${LIBRARY_OUT} MODULE "${SOURCE_FILES}")
#     install(TARGETS ${LIBRARY_OUT} DESTINATION "${OUTPUT_DIR}/WSA/${ARCH}")
# elseif(LINUX)
#     add_library(${LIBRARY_OUT} MODULE "${SOURCE_FILES}")
#         if(ARCH STREQUAL "x86")
#            set_target_properties(${LIBRARY_OUT} PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
#         elseif(ARCH STREQUAL "x86_64")
#            set_target_properties(${LIBRARY_OUT} PROPERTIES COMPILE_FLAGS "-m64" LINK_FLAGS "-m64")
#         endif()
#     install(TARGETS ${LIBRARY_OUT} DESTINATION "${OUTPUT_DIR}/Linux/${ARCH}")
# else()
#     message(FATAL_ERROR "Unsupported platform")
# endif()

# #target_compile_options(${LIBRARY_OUT} PUBLIC "-Wno-switch;-Wno-multichar;-Wno-null-conversion;-Wno-deprecated-declarations;-Wno-macro-redefined")
# target_compile_options(${LIBRARY_OUT} PUBLIC "-fdeclspec")


# install(TARGETS ${LIBRARY_OUT} DESTINATION "${OUTPUT_DIR}/Linux/${ARCH}")
