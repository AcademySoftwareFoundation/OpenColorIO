#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

PYTHON_VERSION="$1"

brew install pyenv openssl readline

# Workaround Python build errors:
#   https://github.com/pyenv/pyenv/wiki/common-build-problems
export CFLAGS="$CLFAGS -I$(brew --prefix openssl)/include -I$(brew --prefix readline)/include"
export LDFLAGS="-L$(brew --prefix openssl)/lib -L$(brew --prefix readline)/lib $LDFLAGS"

# Include macOS SDK headers, required for macOS >= 10.14:
#   https://apple.stackexchange.com/questions/337940/why-is-usr-include-missing-i-have-xcode-and-command-line-tools-installed-moja
export CFLAGS="$CFLAGS -I$(xcrun --show-sdk-path)/usr/include"

# Silence Python build warnings
export CFLAGS="$CLFAGS -Wno-nullability-completeness"

echo 'eval "$(pyenv init -)"' >> .bash_profile
source .bash_profile
env PYTHON_CONFIGURE_OPTS="--enable-framework" pyenv install -v ${PYTHON_VERSION}
pyenv global ${PYTHON_VERSION}
