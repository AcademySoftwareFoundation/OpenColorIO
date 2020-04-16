// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <iostream>
#include "DDImage/ddImageVersionNumbers.h"
int main(int, char*[])
{
    // Print a Nuke API identifier number, used to make the
    // lib/nuke${API_NUMBER} directory

    // Nuke aims to maintain API compatibility between "v" releases, so
    // compiling for 6.1v1 will work with 6.1v2 etc (but not
    // 6.2v1). Only exception has been 5.1v5 and 5.1v6 (because it was
    // supposed to be 5.2v1)
    std::cout << kDDImageVersionMajorNum << "." << kDDImageVersionMinorNum;
    return 0;
}
