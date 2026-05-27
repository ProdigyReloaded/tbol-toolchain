# TBOL VS Code Extension Makefile
#
# Targets:
#   all       - Build extension and language server
#   extension - Build just the VS Code extension
#   server    - Build just the language server
#   package   - Create .vsix package for distribution
#   install   - Install extension to VS Code
#   clean     - Clean build artifacts
#   dev       - Development mode: symlink server, don't package

LSP_DIR := ../lsp
EXTENSION_NAME := prodigy-tbol
VSIX_FILE := $(EXTENSION_NAME)-$(shell node -p "require('./package.json').version").vsix

.PHONY: all extension server package install clean dev check-deps

all: extension server

# Check for required tools
check-deps:
	@command -v npm >/dev/null 2>&1 || { echo "npm required but not found"; exit 1; }
	@command -v node >/dev/null 2>&1 || { echo "node required but not found"; exit 1; }

# Build the VS Code extension (TypeScript -> JavaScript)
extension: check-deps node_modules
	npm run compile

node_modules: package.json
	npm install
	touch node_modules

# Build the language server (prefer CMake build output, fall back to Makefile)
CMAKE_LSP := ../build/lsp/tbol-lsp
server:
	@if [ -x "$(CMAKE_LSP)" ]; then \
		echo "Using CMake-built tbol-lsp"; \
	else \
		$(MAKE) -C $(LSP_DIR); \
	fi

# Create distributable .vsix package
package: extension server
	@mkdir -p bin
	@if [ -x "$(CMAKE_LSP)" ]; then \
		cp $(CMAKE_LSP) bin/; \
	else \
		cp $(LSP_DIR)/tbol-lsp bin/; \
	fi
	npm run package
	@echo "Created $(VSIX_FILE)"

# Install to VS Code
install: package
	code --install-extension $(VSIX_FILE)

# Development mode - no packaging, server runs from lsp directory
dev: extension server
	@echo "Extension built. To test:"
	@echo "  1. Open VS Code in this directory"
	@echo "  2. Press F5 to launch Extension Development Host"
	@echo "  3. Open a .src file to test"

# Clean build artifacts
clean:
	rm -rf out node_modules bin *.vsix
	$(MAKE) -C $(LSP_DIR) clean

# Watch mode for development
watch: node_modules
	npm run watch
