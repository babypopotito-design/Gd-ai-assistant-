#!/usr/bin/env bash
set -euo pipefail

if ! command -v geode >/dev/null 2>&1; then
  echo "Geode CLI was not found. Install it from https://docs.geode-sdk.org/getting-started/geode-cli/" >&2
  exit 1
fi

if [[ -z "${GEODE_SDK:-}" ]]; then
  echo "GEODE_SDK is not set. Run 'geode config setup' or export GEODE_SDK to your SDK directory." >&2
  exit 1
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"
geode build "$@"

echo
find "$SCRIPT_DIR" -type f -name '*.geode' -print
