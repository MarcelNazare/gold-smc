#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

if [[ -f .env ]]; then
    set -a; source .env; set +a
fi

if [[ -z "${TWELVEDATA_API_KEY:-}" ]]; then
    echo "ERROR: TWELVEDATA_API_KEY not set (put it in .env)" >&2
    exit 1
fi

./build/gold_smc "${1:-config/config.json}"