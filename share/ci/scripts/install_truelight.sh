#!/usr/bin/env bash

set -ex

TRUELIGHT_MAJOR="$1"
TRUELIGHT_MINOR="$2"
TRUELIGHT_PATCH="$3"
TRUELIGHT_VERSION="${TRUELIGHT_MAJOR}.${TRUELIGHT_MINOR}.${TRUELIGHT_PATCH}"

mkdir _truelight
cd _truelight

wget -q -O truelight-${TRUELIGHT_VERSION}_64.run \
     --post-data "access=public&download=truelight/${TRUELIGHT_MAJOR}_${TRUELIGHT_MINOR}/truelight-${TRUELIGHT_VERSION}_64.run&last_page=/support/customer-login/truelight_sp/truelight_${TRUELIGHT_MAJOR}${TRUELIGHT_MINOR}.php" \
     https://www.filmlight.ltd.uk/resources/download.php

sh truelight-${TRUELIGHT_VERSION}_64.run

cd ..
rm -rf _truelight
