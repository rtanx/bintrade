.PHONY: help configure-debug configure-release configure-coverage build build-debug build-release test coverage coverage-report clean format lint lint-fix install

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

build-debug: configure-debug ## Build debug configuration
	cmake --build build/debug --parallel

build-release: configure-release ## Build release configuration
	cmake --build build/release --parallel

build: ## Build current configuration (BUILD_TYPE=debug)
	cmake --build $(BUILD_DIR) --parallel

test: ## Run tests
	ctest --test-dir $(BUILD_DIR) --output-on-failure

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
