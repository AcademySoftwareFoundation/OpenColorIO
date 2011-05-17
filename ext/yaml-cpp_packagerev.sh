#!/bin/bash

SVNREV="r482"
svn export -r ${SVNREV} http://yaml-cpp.googlecode.com/svn/trunk yaml-cpp-${SVNREV}
tar -cvzf yaml-cpp-${SVNREV}.tar.gz yaml-cpp-${SVNREV}
/bin/rm -rf yaml-cpp-${SVNREV}
