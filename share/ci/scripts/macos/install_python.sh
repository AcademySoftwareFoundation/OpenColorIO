#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

PYTHON_VERSION="$1"

brew install pyenv openssl

# Include openssl, disable nullability warning, to greatly reduce log noise during Python install
# Include macOS SDK headers, required for macOS >= 10.14:
#   https://apple.stackexchange.com/questions/337940/why-is-usr-include-missing-i-have-xcode-and-command-line-tools-installed-moja
export CFLAGS="-I$(brew --prefix openssl)/include -Wno-nullability-completeness -isysroot $(xcrun --show-sdk-path)"
export LDFLAGS="-L$(brew --prefix openssl)/lib $LDFLAGS"

echo 'eval "$(pyenv init -)"' >> .bash_profile
source .bash_profile
env PYTHON_CONFIGURE_OPTS="--enable-framework" pyenv install -v ${PYTHON_VERSION}
pyenv global ${PYTHON_VERSION}
