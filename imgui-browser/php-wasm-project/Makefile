# Makefile – Convenience targets for the PHP WASM project.
# ─────────────────────────────────────────────────────────────────────────────
# Targets:
#   make build          Full build (requires Emscripten on PATH)
#   make docker-build   Full build inside the official Emscripten Docker image
#   make serve          Start the Node.js dev server (sets COOP/COEP + WASM headers)
#   make clean          Remove generated files
#   make help           Print this help
# ─────────────────────────────────────────────────────────────────────────────

SHELL := /bin/bash
.DEFAULT_GOAL := help

PHP_VERSION   ?= 8.3.20
EMSDK_VERSION ?= 5.0.2
EMSDK_ENV     ?= /opt/emsdk/emsdk_env.sh
PORT          ?= 8080

DOCKER_IMAGE  := emscripten/emsdk:$(EMSDK_VERSION)
DOCKER_RUN    := docker run --rm -it \
                     -v "$(CURDIR):/src" \
                     -w /src \
                     $(DOCKER_IMAGE)

# ── Primary targets ────────────────────────────────────────────────────────

.PHONY: build
build: ## Full WASM build using local EMSDK (emcc must be installed at /opt/emsdk or set EMSDK_ENV)
	@EMSDK_ENV=$(EMSDK_ENV) bash build.sh --php-version $(PHP_VERSION)

.PHONY: docker-build
docker-build: ## Full WASM build inside Emscripten Docker container (alternative to local build)
	$(DOCKER_RUN) bash build.sh --php-version $(PHP_VERSION)

.PHONY: serve
serve: ## Start the Node dev server on localhost:$(PORT) -- sets COOP/COEP + application/wasm headers
	@node server.js $(PORT)

.PHONY: test
test: ## Run headless Node.js integration tests against wasm/php.js (no host PHP needed)
	@node test/run-tests.js

.PHONY: clean
clean: ## Remove generated build artefacts
	rm -rf php-src wasm/php.js wasm/php.wasm

.PHONY: clean-all
clean-all: clean ## Remove generated files AND the downloaded PHP sources
	rm -rf php-src

# ── Utility targets ────────────────────────────────────────────────────────

.PHONY: fetch
fetch: ## Download PHP $(PHP_VERSION) sources only
	@bash scripts/fetch-php.sh $(PHP_VERSION)

.PHONY: configure
configure: ## Run emconfigure ./configure inside php-src/
	@bash scripts/configure-php.sh

.PHONY: link
link: ## Re-run the emcc link step only (skips PHP compile)
	@bash scripts/link-wasm.sh

# ── Help ──────────────────────────────────────────────────────────────────

.PHONY: help
help: ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*##' $(MAKEFILE_LIST) \
		| awk 'BEGIN {FS = ":.*##"}; {printf "  \033[36m%-16s\033[0m %s\n", $$1, $$2}'
