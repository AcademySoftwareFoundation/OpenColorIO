#!/usr/bin/env bash

set -ex

PYTHON_VERSION="$1"

MACOS_MAJOR="$(sw_vers -productVersion | cut -f 1 -d .)"
MACOS_MINOR="$(sw_vers -productVersion | cut -f 2 -d .)"

# This workaround is needed for building Python on macOS >= 10.14:
#   https://developer.apple.com/documentation/xcode_release_notes/xcode_10_release_notes

if [[ "$MACOS_MAJOR" -gt 9 ] && [ "$MACOS_MINOR" -gt 13 ]]; then
    sudo installer \
        -pkg /Library/Developer/CommandLineTools/Packages/macOS_SDK_headers_for_macOS_${MACOS_MAJOR}.${MACOS_MINOR}.pkg \
        -target /
fi

brew update
brew upgrade cmake
brew install glew
brew install pyenv

echo 'eval "$(pyenv init -)"' >> .bash_profile
source .bash_profile
env PYTHON_CONFIGURE_OPTS="--enable-framework" pyenv install -v ${PYTHON_VERSION}
pyenv global ${PYTHON_VERSION}
