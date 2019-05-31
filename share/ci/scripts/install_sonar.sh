#!/usr/bin/env bash

set -ex

SONAR_VERSION="$1"

mkdir _sonar
cd _sonar

wget https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip
unzip build-wrapper-linux-x86.zip
mv build-wrapper-linux-x86 /var/opt/.

export PATH="/var/opt/build-wrapper-linux-x86:${PATH}"

wget https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-${SONAR_VERSION}-linux.zip
unzip sonar-scanner-cli-${SONAR_VERSION}-linux.zip
mv sonar-scanner-${SONAR_VERSION}-linux /var/opt/.

export PATH="/var/opt/sonar-scanner-${SONAR_VERSION}-linux/bin:${PATH}"
export SONAR_RUNNER_HOME="/var/opt/sonar-scanner-${SONAR_VERSION}-linux"

cd ..
rm -rf _sonar
