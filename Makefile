CC          = clang++
BUILD_DIR   = build
BUILD_TYPE ?= DEBUG

define PrintExec
	@GREEN=$$(tput setaf 2); \
	NC=$$(tput sgr0);        \
	echo "$${GREEN}============< exec ${1}::${BUILD_TYPE} >============$${NC}"
endef

# build bin-patcher
# execute bin-patcher to patch remc-client or main-test
define InitBinPatcher
	@cmake --build ${BUILD_DIR} --target bin-patcher
	@./${BUILD_DIR}/src/crypto/bin-patcher --create
	@./${BUILD_DIR}/src/crypto/bin-patcher ${1} ${2}
endef

.PHONY:           \
	configure     \
	build-server  \
	build-client  \
	build-tests   \
	build-gui     \
	run-server    \
	run-client    \
	run-test      \
	run-gui       \
	clean

configure:
	@if [ ! -d "${BUILD_DIR}" ]; then   \
		cmake                           \
		-G Ninja                        \
		-DCMAKE_CXX_COMPILER=${CC}      \
		-DREMC_BUILD_TYPE=${BUILD_TYPE} \
			-DBUILD_CRYPTO_MODULE=ON    \
			-DBUILD_NET_MODULE=ON       \
			-DBUILD_GUI_MODULE=ON       \
			-DBUILD_TEST_MODULE=ON      \
		-DBUILD_SERVER=ON               \
		-DBUILD_CLIENT=ON               \
		-B ${BUILD_DIR};                \
fi

#
# build
#
build-server: configure
	@cmake --build ${BUILD_DIR} --target remc-server

build-client: configure
	@cmake --build ${BUILD_DIR} --target remc-client

build-tests:  configure
	@cmake --build ${BUILD_DIR} --target main-test

build-gui:    configure
	@cmake --build ${BUILD_DIR} --target gui-main

#
# run
#
run-server: build-server
	$(call PrintExec,SERVER)
	@./$(BUILD_DIR)/remc-server

run-client: build-client
	$(call InitBinPatcher, ${BUILD_DIR}/remc-client, public-keys.json)
	$(call PrintExec,CLIENT)
	@./${BUILD_DIR}/remc-client

run-test: build-tests
	$(call InitBinPatcher, ${BUILD_DIR}/src/tests/main-test, public-keys.json)
	$(call PrintExec,TESTS)
	@./${BUILD_DIR}/src/tests/main-test

run-gui: build-gui
	$(call PrintExec,GUI)
	@./${BUILD_DIR}/src/tests/gui-main

#
# clean
#
clean: 
	@rm -rf ${BUILD_DIR}
