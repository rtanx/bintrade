.PHONY: help configure-debug configure-release configure-coverage configure-bench build build-debug build-release test bench coverage coverage-report clean format lint lint-fix install docs integration-test packaging-test

BUILD_TYPE ?= debug
BUILD_DIR  := build/$(BUILD_TYPE)

help: ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-20s\033[0m %s\n", $$1, $$2}'

configure-debug: ## Configure debug build
	cmake --preset debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

configure-release: ## Configure release build
	cmake --preset release

configure-coverage: ## Configure coverage build (requires gcovr: pip install gcovr)
	cmake --preset coverage

configure-bench: ## Configure benchmark build (Release)
	cmake --preset bench

build-debug: configure-debug ## Build debug configuration
	cmake --build build/debug --parallel

build-release: configure-release ## Build release configuration
	cmake --build build/release --parallel

build: ## Build current configuration (BUILD_TYPE=debug)
	cmake --build $(BUILD_DIR) --parallel

test: ## Run tests
	ctest --test-dir $(BUILD_DIR) --output-on-failure

bench: configure-bench ## Build and run benchmarks (Release)
	cmake --build build/bench --parallel
	./build/bench/benchmarks/bintrade_benchmarks --benchmark_format=console --benchmark_counters_tabular=true

coverage: configure-coverage ## Build, run tests, and generate HTML coverage report (build/coverage/report/index.html)
	cmake --build build/coverage --parallel
	ctest --test-dir build/coverage --output-on-failure
	mkdir -p build/coverage/report
	gcovr \
		--root $(CURDIR) \
		--object-directory build/coverage \
		--exclude '$(CURDIR)/build/.*' \
		--exclude '$(CURDIR)/tests/.*' \
		--exclude '$(CURDIR)/examples/.*' \
		--html-details build/coverage/report/index.html \
		--print-summary

coverage-report: ## Open the HTML coverage report in the browser
	open build/coverage/report/index.html

clean: ## Remove all build artifacts
	rm -rf build

format: ## Format source code with clang-format
	find include src tests examples \( -name '*.hpp' -o -name '*.cpp' \) | xargs clang-format -i

lint: ## Run clang-tidy static analysis (parallel)
	run-clang-tidy -p $(BUILD_DIR) -quiet 'include/.*\.(hpp|cpp)$$' 'src/.*\.(hpp|cpp)$$'

lint-fix: ## Run clang-tidy with auto-fix (parallel)
	run-clang-tidy -p $(BUILD_DIR) -quiet -fix 'include/.*\.(hpp|cpp)$$' 'src/.*\.(hpp|cpp)$$'

install: ## Install library
	cmake --install $(BUILD_DIR)

docs: ## Generate Doxygen HTML reference into build/docs/html/
	@command -v doxygen >/dev/null 2>&1 || { \
		echo "doxygen not found on PATH. Install it:"; \
		echo "  macOS  : brew install doxygen"; \
		echo "  Ubuntu : sudo apt-get install -y doxygen"; \
		echo "  Windows: choco install doxygen.install"; \
		exit 1; \
	}
	@mkdir -p build/docs
	doxygen Doxyfile
	@echo ""
	@echo "Docs written to: build/docs/html/index.html"

integration-test: ## Configure + run live integration tests against Binance testnet (network required)
	cmake --preset debug -DBINTRADE_ENABLE_INTEGRATION_TESTS=ON
	cmake --build build/debug --target bintrade_integration_tests --parallel
	./build/debug/tests/bintrade_integration_tests

packaging-test: ## Install to a tmp prefix and build the downstream consumer smoke test
	@echo "==> Installing bintrade to a temp prefix"
	cmake --build $(BUILD_DIR) --parallel
	cmake --install $(BUILD_DIR) --prefix build/packaging-test/prefix
	@echo "==> Configuring downstream consumer against installed package"
	cmake \
		-S tests/packaging \
		-B build/packaging-test/consumer \
		-DCMAKE_TOOLCHAIN_FILE=$$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
		-Dbintrade_DIR=$(CURDIR)/build/packaging-test/prefix/lib/cmake/bintrade
	cmake --build build/packaging-test/consumer --parallel
	@echo "==> Running consumer smoke binary"
	./build/packaging-test/consumer/consumer
