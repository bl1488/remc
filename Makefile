CC          = clang++
BUILD_DIR   = build
BUILD_TYPE ?= DEBUG

define PrintExec
	@GREEN=$$(tput setaf 2); \
	NC=$$(tput sgr0);        \
	echo "$${GREEN}============< exec $(1)::$(BUILD_TYPE) >============$${NC}"
endef

.PHONY:           \
	configure     \
    build-server  \
    build-client  \
    build-tests   \
    build-gui     \
    run-server    \
    run-client    \
    run-tests     \
    run-gui       \
    clean

configure:
	@if [ ! -d "${BUILD_DIR}" ]; then        \
		cmake                                \
		   -G Ninja                          \
		   -DCMAKE_CXX_COMPILER=${CC}        \
		   -DREMC_BUILD_TYPE=$(BUILD_TYPE)   \
				-DBUILD_CRYPTO_MODULE=ON     \
				-DBUILD_NET_MODULE=ON        \
				-DBUILD_GUI_MODULE=ON        \
				-DBUILD_TEST_MODULE=ON       \
		   -DBUILD_SERVER=ON                 \
		   -DBUILD_CLIENT=ON                 \
		   -B ${BUILD_DIR};                  \
	fi

build-server: configure
	@cmake --build ${BUILD_DIR} --target remc-server

build-client: configure
	@cmake --build ${BUILD_DIR} --target remc-client

build-tests:  configure
	@cmake --build ${BUILD_DIR} --target tests-main

build-gui:    configure
	@cmake --build ${BUILD_DIR} --target gui-main

run-server: build-server
	$(call PrintExec,SERVER)
	@./$(BUILD_DIR)/remc-server

run-client: build-client
	$(call PrintExec,CLIENT)
	@./$(BUILD_DIR)/remc-client

run-tests: build-tests
	$(call PrintExec,TESTS)
	@./$(BUILD_DIR)/src/tests/tests-main

run-gui: build-gui
	$(call PrintExec,GUI)
	@./$(BUILD_DIR)/src/tests/gui-main

clean: 
	@rm -rf $(BUILD_DIR)
