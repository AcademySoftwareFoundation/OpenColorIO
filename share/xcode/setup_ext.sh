#!/bin/sh

# Run this before attempting to build the xcode project

dir=`dirname $0`
echo Expanding external dependencies: $dir/ext
mkdir -p $dir/ext
cd $dir/ext
echo Installing lcms2-2.1.tar.gz
tar xzvf ../../../ext/lcms2-2.1.tar.gz
echo Installing tinyxml_2_6_1.tar.gz
tar xzvf ../../../ext/tinyxml_2_6_1.tar.gz
echo Installing yaml-cpp-0.3.0.tar.gz 
tar xzvf ../../../ext/yaml-cpp-0.3.0.tar.gz 

