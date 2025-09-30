"""
Script to generate pyi stubs by patching and calling mypy's stubgen tool.

There are two entry-points which are designed to call this script:
 - `cmake --build . --target pystubs` is called during local development to generate the
   stubs and copy them into the git repo to be committed and reviewed.
 - in CI, the cibuildwheel action is used to validate that the stubs match what
   has been committed to the repo.

The depdendencies for the script are defined in pyproject.toml.
"""

from __future__ import absolute_import, annotations, division, print_function

import re

from typing import Any

import mypy.stubgen
import mypy.stubgenc
from mypy.stubgenc import DocstringSignatureGenerator, SignatureGenerator

from stubgenlib.siggen import (
    AdvancedSignatureGenerator,
    AdvancedSigMatcher,
)
from stubgenlib.utils import add_positional_only_args


class OCIOSignatureGenerator(AdvancedSignatureGenerator):
    sig_matcher = AdvancedSigMatcher(
        # Override entire function signature:
        signature_overrides={
            # signatures for these special methods include many inaccurate overloads
            "*.__ne__": "(self, other: object) -> bool",
            "*.__eq__": "(self, other: object) -> bool",
        },
        # Override argument types
        #   dict of (name_pattern, arg, type) to arg_type
        #   type can be str | re.Pattern
        arg_type_overrides={
            ("*", "*", re.compile(r"list\[(.*)]")): r"Iterable[\1]",
        },
        # Override result types
        #   dict of (name_pattern, type) to result_type
        #   e.g. ("*", "Buffer"): "numpy.ndarray"
        result_type_overrides={},
        # Override property types
        #   dict of (name_pattern, type) to result_type
        #   e.g. ("*", "Buffer"): "numpy.ndarray"
        property_type_overrides={},
        # Types that have implicit alternatives.
        #   dict of type_str to list of types that can be used instead
        #   e.g. "PySide2.QtGui.QKeySequence": ["str"],
        # converts any matching argument to a union of the supported types
        implicit_arg_types={},
    )

    def process_sig(
        self, ctx: mypy.stubgen.FunctionContext, sig: mypy.stubgen.FunctionSig
    ) -> mypy.stubgen.FunctionSig:
        # Analyze the signature and add a '/' argument if necessary to mark
        # arguments which cannot be access by name.
        return add_positional_only_args(ctx, super().process_sig(ctx, sig))


class InspectionStubGenerator(mypy.stubgenc.InspectionStubGenerator):
    def get_sig_generators(self) -> list[SignatureGenerator]:
        return [
            OCIOSignatureGenerator(
                fallback_sig_gen=DocstringSignatureGenerator(),
            )
        ]

    def set_defined_names(self, defined_names: set[str]) -> None:
        super().set_defined_names(defined_names)
        for typ in ["Buffer"]:
            self.add_name(f"typing_extensions.{typ}", require=False)

    def get_base_types(self, obj: type) -> list[str]:
        bases = super().get_base_types(obj)
        if obj.__name__ == "Exception":
            return ["__builtins__.Exception"]
        else:
            return bases


mypy.stubgen.InspectionStubGenerator = InspectionStubGenerator  # type: ignore[attr-defined,misc]
mypy.stubgenc.InspectionStubGenerator = InspectionStubGenerator  # type: ignore[misc]


def get_colored_diff(old_text: str, new_text: str):
    """
    Generates a colored diff between two strings.

    Returns:
        A string containing the colored diff output.
    """
    import difflib

    red = '\033[31m'
    green = '\033[32m'
    reset = '\033[0m'

    diff = difflib.unified_diff(
        old_text.splitlines(keepends=True),
        new_text.splitlines(keepends=True),
        lineterm="",
    )
    lines = []
    for line in diff:
        if line.startswith('-'):
            lines.append(f"{red}{line}{reset}")
        elif line.startswith('+'):
            lines.append(f"{green}{line}{reset}")
        else:
            lines.append(line)
    return "".join(lines)


def main() -> None:
    import argparse
    import pathlib
    import os
    import sys

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--out-path",
        help="Directory to write the stubs."
    )
    parser.add_argument(
        "--validate-path",
        default=None,
        help="If provided, compare the generated stub to this file. Exits with code 2 if the "
             "contents differ."
    )
    args = parser.parse_args()
    if not args.out_path:
        out_path = pathlib.Path(sys.modules[__name__].__file__).parent
    else:
        out_path = pathlib.Path(args.out_path)
    print(f"Stub output directory: {out_path}")

    # perform import so we can see the traceback if it fails.
    import PyOpenColorIO

    sys.argv[1:] = ["-p", "PyOpenColorIO", "-o", str(out_path)]
    mypy.stubgen.main()
    source_path = out_path.joinpath("PyOpenColorIO", "PyOpenColorIO.pyi")
    if not source_path.exists():
        print("Stub generation failed")
        sys.exit(1)

    dest_path = out_path.joinpath("PyOpenColorIO", "__init__.pyi")
    print(f"Renaming to {dest_path}")
    os.rename(source_path, dest_path)

    new_text = dest_path.read_text()
    new_text = (
        "#\n# This file is auto-generated. DO NOT MODIFY!\n"
        "# See docs/quick_start/installation.rst for more info\n#\n\n"
    ) + new_text
    dest_path.write_text(new_text)

    if args.validate_path and os.environ.get("GITHUB_ACTIONS", "false").lower() == "true":
        # in CI, validate that what has been committed to the repo is what we expect.
        validate_path = pathlib.Path(args.validate_path)

        print("Validating stubs against repository")
        print(f"Comparing {dest_path} to {validate_path}")

        old_text = validate_path.read_text()

        if old_text != new_text:
            print("Stub verification failed!")
            print("Changes to the source code have resulted in a change to the stubs.")
            print(get_colored_diff(old_text, new_text))
            print("Run `cmake /path/to/source; cmake --build . --target pystubs` locally and "
                  "commit the results for review.")
            sys.exit(2)


if __name__ == "__main__":
    main()
