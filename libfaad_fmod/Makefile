MAKEFLAGS += --warn-undefined-variables
SHELL = /bin/bash -o nounset -o errexit -o pipefail
.DEFAULT_GOAL = build
CMAKE = "/Applications/CMake.app/Contents/bin/cmake"
MAKE_PATH := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

UNITY_PROJ := "$(dir ${MAKE_PATH})amp.client.unity"
UNITY_PATH := $(shell python3 ${UNITY_PROJ}/amp-utils.py --get-engine-path --base-path "${UNITY_PROJ}")

#ANDROID_TARGET_ABI = armeabi-v7a
ANDROID_TARGET_ABI = arm64-v8a
ANDROID_NDK = "${UNITY_PATH}/PlaybackEngines/AndroidPlayer/NDK"

BUILD_MODE=Release
#BUILD_MODE=Debug

export COLOR
export MAKESILENT
export VERBOSE
#$(VERBOSE).SILENT:


CMAKE_SETUP = cd "${MAKE_PATH}/build" && \
	${CMAKE} .. \
	-DLIBRARY_OUT:STRING=fmod_faac \
	-DPLUGIN_VERSION:STRING=1.0.0.0 \
	-DPLUGIN_BUILD_NUMBER:STRING=1 \
	-DSOURCE_FOLDER:PATH="${MAKE_PATH}/src" \
	-DOUTPUT_DIR:PATH="${UNITY_PROJ}/Assets/Plugins/Amp.Unity/Plugins" \
	-DCMAKE_BUILD_TYPE=${BUILD_MODE}
	
CMAKE_RELEASE = ${CMAKE} --build . --config ${BUILD_MODE} --clean-first


	
## display this help message
help:
	@echo "    MAKE_PATH:      ${MAKE_PATH}"
	@echo "    UNITY_PATH:     ${UNITY_PATH}"
	@echo "    UNITY_PROJ:     ${UNITY_PROJ}"
	@echo "    ANDROID_NDK:    ${ANDROID_NDK}"

	@echo -e "\033[32m"
	@echo
	@awk '/^##.*$$/,/[a-zA-Z_-]+:/' $(MAKEFILE_LIST) | awk '!(NR%2){print $$0p}{p=$$0}' | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-16s\033[0m %s\n", $$1, $$2}' | sort


.PHONY: build

clean:
	rm    -rf debug
	rm    -rf build
	mkdir -p  build
	
## build for all platforms
build: clean osx ios android

osx:	
	${CMAKE_SETUP} \
	-DOSX:BOOL=ON -BOSX
	
	cd "build/OSX" && \
		${CMAKE_RELEASE}


ios:
	${CMAKE_SETUP} \
	-DIOS:BOOL=ON -BiOS 

	cd "build/iOS" && \
        IPHONEOS_DEPLOYMENT_TARGET=12.4 ${CMAKE_RELEASE}


android:
	${CMAKE_SETUP}                                                                          \
	-G "Unix Makefiles"                                                                     \
	-DANDROID:BOOL=ON -BAndroid/${ANDROID_TARGET_ABI}                                       \
	-DANDROID_NDK:PATH=${ANDROID_NDK}                                                       \
	-DCMAKE_TOOLCHAIN_FILE:FILEPATH="${ANDROID_NDK}/build/cmake/android.toolchain.cmake"    \
	-DANDROID_ABI=${ANDROID_TARGET_ABI}                                                     \
	-DANDROID_PLATFORM=android-27
	
	cd "build/Android/${ANDROID_TARGET_ABI}" && \
		${CMAKE_RELEASE}

linux:
	${CMAKE_SETUP} \
	-G "Unix Makefiles" \
	-DLINUX:BOOL=ON -BLinux/x86_64 
	-DARCH:STRING=x86_64 

	cd "build/Linux/x86_64" && \
		${CMAKE_RELEASE}

