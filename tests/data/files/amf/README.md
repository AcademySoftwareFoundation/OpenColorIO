# AMF test fixtures

Most of the `*.amf` files here (and their `.clf` / `.ccc` assets) are copied
verbatim from the canonical ACES AMF library's generated sample corpus:

    aces-amf/packages/aces-amf-lib/tests/Generated_Samples_AMF/valid_AMFs/

Using the same corpus keeps the OCIO AMF reader validated against the same
inputs as the canonical library. They were copied for now; in future they
should be pulled directly from the canonical library rather than duplicated
here.

## What the OCIO reader supports

The reader targets **AMF schema v2.0+** and **ACES 2** only (OCIO 2.6). It
resolves transform IDs against the `amf_transform_ids` interchange metadata of
the reference config (the ACES 2 studio builtin config by default). ACES 1.x
pipelines are not supported against the builtin config; a caller wanting ACES
1.x must supply their own OCIO config with the `amf_transform_ids` metadata
populated.

Consequently only the canonical samples whose transform IDs are **ACES 2.0**
(`:v2.0:`) resolve against the builtin config and are used as positive tests.
Samples with ACES 1.5 (`:v1.5:`) transform IDs are expected to fail to load and
are not copied (except one, retained as a negative test).

## Negative fixtures

- `26_aces_1_3_explicit.amf` — canonical sample using ACES 1.5 transform IDs;
  must fail to load (transforms absent from the ACES 2 builtin config).
- `negative_v1_schema.amf` — an AMF v1.0 schema document; must be rejected at
  the root element (only AMF v2.0+ is supported).

## Note on the file-based (LUT) workflow

Some samples express transforms as external `.clf`/`.ccc` files. The reader
handles these but relies on a validation workaround (`addDefaultViewTransform`
injects an unrelated `Un-tone-mapped` view transform so the generated config
passes validation, because `initAMFConfig` adds the display-referred
`CIE-XYZ-D65` interchange space). This file-based ODT workflow needs further
validation and should be revisited.
