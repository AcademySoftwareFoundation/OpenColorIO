#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Compile ocio.kotoba with Kotoba 0.7.2 (wasm32, i64-v1) and assert
# config header fields against the vendored fixture.
# Fail closed. Do not print success unless every comparison ran.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
FIXTURE="$ROOT/fixtures/tiny.ocio"
SRC="$ROOT/ocio.kotoba"
WORKDIR="$(mktemp -d "${TMPDIR:-/tmp}/kotoba-ocio-v1.XXXXXX")"
trap 'rm -rf "$WORKDIR"' EXIT

KOTOBA_VERSION="0.7.2"
KOTOBA_TARBALL="kotoba-linux-amd64.tar.gz"
# sha256 of https://github.com/kotoba-lang/kotoba/releases/download/v0.7.2/kotoba-linux-amd64.tar.gz
KOTOBA_SHA256="95e225461e1b8a21849b251e8c8b654693d2c8a516b258532771651e978e1977"

fail() {
  printf 'kotoba/checks.sh: %s\n' "$*" >&2
  exit 1
}

command -v python3 >/dev/null 2>&1 || fail "python3 is required"
command -v node >/dev/null 2>&1 || fail "node is required to run the wasm32 module"

if [[ -n "${KOTOBA:-}" ]]; then
  KOTOBA_BIN="${KOTOBA}"
  [[ -x "${KOTOBA_BIN}" ]] || fail "KOTOBA=${KOTOBA_BIN} is not executable"
elif command -v kotoba >/dev/null 2>&1; then
  KOTOBA_BIN="$(command -v kotoba)"
else
  uname_s="$(uname -s)"
  uname_m="$(uname -m)"
  if [[ "${uname_s}" != "Linux" || "${uname_m}" != "x86_64" ]]; then
    fail "kotoba is not on PATH (need CLI 0.7.2); automatic install is linux-amd64 only (this host is ${uname_s}/${uname_m})"
  fi
  command -v curl >/dev/null 2>&1 || fail "curl is required to fetch Kotoba ${KOTOBA_VERSION}"
  cache="${ROOT}/.kotoba-cli/${KOTOBA_VERSION}"
  mkdir -p "${cache}"
  archive="${cache}/${KOTOBA_TARBALL}"
  if [[ ! -x "${cache}/kotoba" ]]; then
    url="https://github.com/kotoba-lang/kotoba/releases/download/v${KOTOBA_VERSION}/${KOTOBA_TARBALL}"
    printf 'downloading Kotoba %s from %s\n' "${KOTOBA_VERSION}" "${url}"
    curl -fsSL -o "${archive}" "${url}"
    got="$(sha256sum "${archive}" | awk '{print $1}')"
    if [[ "${got}" != "${KOTOBA_SHA256}" ]]; then
      fail "checksum mismatch for ${KOTOBA_TARBALL}: got ${got} expected ${KOTOBA_SHA256}"
    fi
    tar -xzf "${archive}" -C "${cache}" kotoba
  fi
  KOTOBA_BIN="${cache}/kotoba"
  [[ -x "${KOTOBA_BIN}" ]] || fail "extracted kotoba binary missing"
fi

printf 'kotoba binary: %s\n' "$KOTOBA_BIN"

CURRENT_LINK="${KOTOBA_HOME:-$HOME/.local/share/kotoba}/current"
if [ -L "$CURRENT_LINK" ]; then
  INSTALLED="$(readlink "$CURRENT_LINK")"
  printf 'kotoba install current: %s\n' "$INSTALLED"
  if [ "$INSTALLED" != "v0.7.2" ]; then
    fail "refusing kotoba $INSTALLED (need v0.7.2)"
  fi
fi

[ -f "$FIXTURE" ] || fail "missing fixture $FIXTURE"
[ -f "$SRC" ] || fail "missing module $SRC"

# Independent field read from the fixture. These numbers come from the
# file bytes, not from the .kotoba source. Version split matches OCIOYaml.cpp.
eval "$(python3 - "$FIXTURE" "$SRC" <<'PY'
import sys
from pathlib import Path

fixture = Path(sys.argv[1])
src = Path(sys.argv[2]).read_text()
raw = fixture.read_bytes()
key = b"ocio_profile_version:"
if not raw.startswith(key):
    sys.exit("fixture does not start with ocio_profile_version:")
if raw[21:24] != b" 2\n":
    sys.exit("fixture version token is not a single ASCII digit 2")
if b"name: raw" not in raw:
    sys.exit("fixture has no colorspace name field 'name: raw'")
if raw[67:70] != b"raw":
    sys.exit("fixture colorspace name bytes at 67:70 are not raw")
for i, b in enumerate(raw):
    needle = f"(= i {i}) {b}"
    if needle not in src:
        sys.exit(f"ocio.kotoba is missing fixture byte {i} = {b}")
# Same split as OCIOYaml.cpp load(Config): one token => minor 0.
version = raw[len(key):].split(b"\n", 1)[0].strip().decode("ascii")
parts = version.split(".")
if len(parts) == 1:
    major = int(parts[0])
    minor = 0
elif len(parts) == 2:
    major = int(parts[0])
    minor = int(parts[1])
else:
    sys.exit(f"unusable ocio_profile_version token {version!r}")
name_ok = 1
packed = 1 + 10 * major + 100 * minor + 1000 * name_ok
print(f"FIXTURE_LEN={len(raw)}")
print(f"FIELD_MAJOR={major}")
print(f"FIELD_MINOR={minor}")
print(f"FIELD_NAME_OK={name_ok}")
print(f"PACKED_EXPECT={packed}")
print(f"EMBEDDED_BYTES={len(raw)}")
PY
)"

printf 'fixture: %s (%s bytes)\n' "$FIXTURE" "$FIXTURE_LEN"
printf 'fixture fields: major=%s minor=%s name_ok=%s\n' \
  "$FIELD_MAJOR" "$FIELD_MINOR" "$FIELD_NAME_OK"
printf 'embedded fixture bytes in ocio.kotoba: %s\n' "$EMBEDDED_BYTES"
printf 'packed expect (from fixture bytes): %s\n' "$PACKED_EXPECT"

COMPILE_JSON="$WORKDIR/compile.json"
WASM="$WORKDIR/ocio.wasm"
set +e
"$KOTOBA_BIN" compile "$SRC" --target wasm -o "$WASM" --json >"$COMPILE_JSON" 2>"$WORKDIR/compile.err"
compile_rc=$?
set -e
if [ "$compile_rc" -ne 0 ]; then
  cat "$COMPILE_JSON" "$WORKDIR/compile.err" >&2 || true
  fail "kotoba compile failed (exit $compile_rc)"
fi

python3 - "$COMPILE_JSON" "$WASM" <<'PY'
import json
import sys
from pathlib import Path

report = json.loads(Path(sys.argv[1]).read_text())
wasm = Path(sys.argv[2])
if report.get("kotoba.cli/ok?") is not True:
    sys.exit(f"compile JSON ok? is {report.get('kotoba.cli/ok?')!r}")
if report.get("kotoba.cli/code") != "emitted":
    sys.exit(f"compile JSON code is {report.get('kotoba.cli/code')!r}")
data = report.get("kotoba.cli/data") or {}
profile = data.get("value-profile")
compat = data.get("compatibility") or {}
target = compat.get("target")
features = data.get("wasm-features") or []
if profile != "i64-v1":
    sys.exit(f"value-profile {profile!r} is not i64-v1")
if target != "wasm32-kotoba-v1":
    sys.exit(f"target {target!r} is not wasm32-kotoba-v1")
blocked = [f for f in features if f in ("simd", "floats", "float", "nontrapping-fptoint")]
if blocked:
    sys.exit(f"unexpected floating/SIMD wasm features: {blocked}")
if not wasm.is_file() or wasm.stat().st_size == 0:
    sys.exit("compile did not write a wasm artifact")
magic = wasm.read_bytes()[:4]
if magic != b"\x00asm":
    sys.exit(f"artifact magic {magic!r} is not wasm")
print(f"compile: value-profile={profile} target={target} wasm-features={features} bytes={wasm.stat().st_size}")
PY

GOT="$(node --input-type=module - "$WASM" <<'JS'
import fs from "node:fs";
const wasm = fs.readFileSync(process.argv[2]);
const { instance } = await WebAssembly.instantiate(wasm);
if (!instance.exports.main) {
  throw new Error("wasm module has no exported main");
}
const value = instance.exports.main();
const n = typeof value === "bigint" ? value : BigInt(value);
process.stdout.write(n.toString());
JS
)"

printf 'wasm main returned: %s\n' "$GOT"
if [ "$GOT" != "$PACKED_EXPECT" ]; then
  fail "packed result $GOT != fixture-derived $PACKED_EXPECT"
fi

printf 'kotoba/checks.sh: compile i64-v1 wasm32 and fixture fields matched\n'
