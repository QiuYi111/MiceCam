# Neural-Grid Standard Makefile
# "The only valid interface to the project"

.PHONY: init up down proto test lint verify clean help

# --- Configuration ---
PROJECT_NAME := micecam
BUILD_DIR := build
PYTHON_ENV := .venv

# --- 1. Environment & Setup ---

help: ## Show this help message
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

init: ## Initialize development environment (Tools, Hooks)
	@echo "🛠️  Initializing Development Environment..."
	@uv venv $(PYTHON_ENV)
	@uv pip install pre-commit pytest ruff mypy pybind11
	@pre-commit install
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
	@echo "🔨 Building MiceCam SDK..."
	@cmake -B $(BUILD_DIR) -S . -DCMAKE_TOOLCHAIN_FILE="$$(vcpkg get toolchain)" -DCMAKE_EXPORT_COMPILE_COMMANDS=1
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

# --- 4. The Gatekeeper ---

verify: build lint test ## Run full verification (Pre-Push Gate)
	@echo "🛡️  Full System Verification Passed."
