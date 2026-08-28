# Kotoba v1 — OCIO config header

This directory is a first-class language tree on the `kotoba-lang/OpenColorIO`
fork, next to `src/OpenColorIO` and `src/bindings/{java,python}`. It is **not**
part of AcademySoftwareFoundation/OpenColorIO upstream.

## Honest scope

Kotoba binding **v1** parses only:

- the config header key `ocio_profile_version:` (the field `OCIOYaml.cpp`
  requires before it will load a config)
- the **major** profile version as an ASCII digit, and **minor** `0` when
  the token has no `.` (same split as `OCIOYaml.cpp`)
- the ColorSpace **name** `raw` on the vendored fixture

from `fixtures/tiny.ocio` (71 bytes). That file is the version line plus
the `colorspaces` / `!<ColorSpace>` / `name: raw` lines from the built-in
raw profile in `src/OpenColorIO/Config.cpp` (`INTERNAL_RAW_PROFILE`),
with the float-bearing middle (roles, displays, `bitdepth: 32f`) dropped.
It is enough to identify those fields and is **not** a complete OCIO
config.

This is **not** a color engine and **not** a replacement for
libOpenColorIO. It does not implement processors, GPU/CPU paths, looks,
viewing rules, config merge, or a LUT/transform pipeline. It is **not
robotics-ready**.

The interesting OCIO surface (matrices, LUT samples, luma coefficients,
transfer functions, `bitdepth: 32f`) is IEEE-float. Kotoba 0.7.2 wasm32 is
**i64-v1** — no IEEE floats — so this tree stays on integer/byte header
fields and says so.

## Language constraints

- Kotoba CLI **0.7.2**
- `kotoba compile --target wasm` → `wasm32-kotoba-v1`
- value profile **i64-v1** (no IEEE floats)
- no FFI / no host imports

`ocio.kotoba` embeds the fixture as integer bytes and uses only `+`, `*`,
`if`, `=`, `<`, and `and`. `main` returns a packed i64 of the parsed fields.

## Checks

`checks.sh` compiles with Kotoba 0.7.2, runs the wasm32 module, and
compares the packed result to fields read from the fixture bytes. It does
not invent pass/fail.

```sh
# requires kotoba 0.7.2 on PATH, or downloads the linux-amd64 0.7.2 CLI
./checks.sh
```

## Operator

awai.network / Ryo Awai
