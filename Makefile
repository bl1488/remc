CC         = clang++
BUILD_DIR  = build
# Default -O1 -g (ASAN_UBSAN)
BUILD_TYPE ?= ASAN_UBSAN

.PHONY:       	  \
	build-server  \
	build-client  \
	run-server    \
	run-client    \
	run-test		  \
	clean

build-server:
	@cmake                             \
		-G Ninja							     \
		-DCMAKE_CXX_COMPILER=${CC}      \
		-DREMC_BUILD_TYPE=$(BUILD_TYPE) \
		-DBUILD_SERVER=ON				     \
		-B ${BUILD_DIR}
	@cmake --build ${BUILD_DIR} --target remc-server

build-client:
	@cmake                             \
		-G Ninja							     \
		-DCMAKE_CXX_COMPILER=${CC}      \
		-DREMC_BUILD_TYPE=$(BUILD_TYPE) \
		-DBUILD_CLIENT=ON				     \
		-B ${BUILD_DIR}
	@cmake --build ${BUILD_DIR} --target remc-client

build-test:
	@cmake                             \
		-G Ninja							     \
		-DCMAKE_CXX_COMPILER=${CC}      \
		-DREMC_BUILD_TYPE=$(BUILD_TYPE) \
		-DBUILD_SERVER=OFF				  \
		-DBUILD_CLIENT=OFF				  \
		-DBUILD_TEST_MODULE=ON			  \
		-B ${BUILD_DIR}
	@cmake --build ${BUILD_DIR}

run-server: build-server
	@echo "============> Exec server with: $(BUILD_TYPE)"
	@./$(BUILD_DIR)/remc-server

run-client: build-client
	@echo "============> Exec client with: $(BUILD_TYPE)"
	@./$(BUILD_DIR)/remc-client

run-test: build-test
	@./$(BUILD_DIR)/src/tests/test-crypto

clean: 
	@rm -rf $(BUILD_DIR)
