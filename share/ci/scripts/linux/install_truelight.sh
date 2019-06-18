#!/usr/bin/env bash

set -ex

TRUELIGHT_VERSION="$1"
TRUELIGHT_MAJOR_MINOR=$(echo "${TRUELIGHT_VERSION}" | cut -d. -f-2)
TRUELIGHT_MAJOR=$(echo "${TRUELIGHT_VERSION}" | cut -d. -f-1)
TRUELIGHT_MINOR=$(echo "${TRUELIGHT_MAJOR_MINOR}" | cut -d. -f2-)

mkdir _truelight
cd _truelight

wget -q -O truelight-${TRUELIGHT_VERSION}_64.run \
     --post-data "access=public&download=truelight/${TRUELIGHT_MAJOR}_${TRUELIGHT_MINOR}/truelight-${TRUELIGHT_VERSION}_64.run&last_page=/support/customer-login/truelight_sp/truelight_${TRUELIGHT_MAJOR}${TRUELIGHT_MINOR}.php" \
     https://www.filmlight.ltd.uk/resources/download.php

sh truelight-${TRUELIGHT_VERSION}_64.run

cd ..
rm -rf _truelight
