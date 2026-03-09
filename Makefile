# Neural-Grid Standard Makefile
# "The only valid interface to the project"

.PHONY: init up down proto test lint verify clean help build

# --- Configuration ---
PROJECT_NAME := micecam
BUILD_DIR := build
PYTHON_ENV := .venv
VCPKG_ROOT := $(CURDIR)/vcpkg
VCPKG_TOOLCHAIN := $(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
CMAKE_MAKE_PROGRAM := $(shell which make)

# --- 1. Environment & Setup ---

define check_macos_deps
	@if [ "$$(uname)" = "Darwin" ]; then \
		echo "🍎 Checking macOS dependencies..."; \
		for dep in autoconf automake pkg-config nasm; do \
			if ! command -v $$dep >/dev/null 2>&1; then \
				echo "📦 Missing $$dep. Attempting to install via Homebrew..."; \
				brew install $$dep || { echo "❌ Failed to install $$dep."; exit 1; }; \
			fi; \
		done; \
		if ! command -v glibtoolize >/dev/null 2>&1; then \
			echo "📦 Missing libtool. Attempting to install via Homebrew..."; \
			brew install libtool || { echo "❌ Failed to install libtool."; exit 1; }; \
		fi; \
		if ! brew list autoconf-archive >/dev/null 2>&1; then \
			echo "📦 Missing autoconf-archive. Attempting to install via Homebrew..."; \
			brew install autoconf-archive || { echo "❌ Failed to install autoconf-archive."; exit 1; }; \
		fi; \
	fi
endef

help: ## Show this help message
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

init: ## Initialize development environment (Tools, Hooks)
	$(check_macos_deps)
	@echo "🛠️  Initializing Development Environment..."
	@git submodule update --init --recursive
	@if [ ! -d "$(VCPKG_ROOT)" ]; then \
		echo "📦 vcpkg not found. Cloning vcpkg..."; \
		git clone https://github.com/microsoft/vcpkg.git $(VCPKG_ROOT); \
		$(VCPKG_ROOT)/bootstrap-vcpkg.sh; \
	fi
	@uv venv $(PYTHON_ENV)
	@uv sync --all-extras
	@uv run pre-commit install
	@echo "✅ Done! Environment is ready. Use 'source $(PYTHON_ENV)/bin/activate' to enter."

up: ## Start infrastructure (Docker)
	@echo "🚀 Starting Infrastructure..."
	@docker-compose up -d --wait
	@echo "✅ Infrastructure is UP."

down: ## Stop infrastructure
	@echo "🛑 Stopping Infrastructure..."
	@docker-compose down -v
	@echo "✅ Cleaned up."

# --- 2. Build ---

build: ## Build CMake project
	$(check_macos_deps)
	@echo "🔨 Building MiceCam SDK..."
	@if [ ! -f "$(VCPKG_TOOLCHAIN)" ]; then \
		echo "📦 vcpkg toolchain not found. Initializing vcpkg..."; \
		if [ ! -d "$(VCPKG_ROOT)" ]; then git clone https://github.com/microsoft/vcpkg.git $(VCPKG_ROOT); fi; \
		$(VCPKG_ROOT)/bootstrap-vcpkg.sh; \
	fi
	@cmake -B $(BUILD_DIR) -S . \
		-DCMAKE_TOOLCHAIN_FILE="$(VCPKG_TOOLCHAIN)" \
		-DCMAKE_MAKE_PROGRAM="$(CMAKE_MAKE_PROGRAM)" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=1
	@cmake --build $(BUILD_DIR) -j
	@echo "✅ Build complete."

# --- 3. Quality Assurance ---

lint: ## Run Code Linters
	@echo "🔍 Running Linters..."
	@pre-commit run --all-files
	@echo "✅ Lint check passed."

test: ## Run Unit & Integration Tests
	@echo "🧪 Running Tests..."
	@cd $(BUILD_DIR) && ctest --output-on-failure
	@echo "✅ Tests passed."

# --- 4. Distribution ---
package: ## Build Python Package (Wheel/Sdist)
	@echo "📦 Building Python Package..."
	@uv build
	@echo "✅ Package built in dist/."

# --- 5. The Gatekeeper ---

verify: build lint test ## Run full verification (Pre-Push Gate)
	@echo "🛡️  Full System Verification Passed."
