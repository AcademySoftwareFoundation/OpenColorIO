#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

# Homebrew does not publish versioned openimageio@X formulae (unlike e.g. python@X), so a
# specific version cannot be pinned here without installing from an old homebrew-core formula
# commit, which risks building OIIO and its whole dependency tree from source if no bottle
# exists for that commit on the runner's current macOS/Xcode. Always install whatever the
# current bottle is instead.
brew install openimageio
