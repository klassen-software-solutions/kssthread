//
//  version.cpp
//  kssthread
//
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include "_license_internal.h"
#include "_version_internal.h"
#include "version.hpp"

using namespace std;

string kss::thread::version() noexcept {
    return versionText;
}

string kss::thread::license() noexcept {
    return licenseText;
}
