#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

DOXYGEN_LOCATION="$1"

# Utility to parse JSON object.
choco install jq

# Get the URL of the latest zip package for Doxygen.
url=$(curl -s 'https://api.github.com/repos/doxygen/doxygen/releases/latest' | jq -r '.assets[] | select(.name | test("doxygen-.*windows.x64.bin.zip")) | .browser_download_url')

# Download the zip.
mkdir $DOXYGEN_LOCATION
cd $DOXYGEN_LOCATION
powershell 'iwr -URI '$url' -OutFile doxygen.zip'

# Unzip the file into $DOXYGEN_LOCATION.
unzip -o doxygen.zip