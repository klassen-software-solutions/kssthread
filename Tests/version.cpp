//
//  version.cpp
//  unittest
//
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <kss/thread/version.hpp>
#include "ksstest.hpp"

using namespace std;
using namespace kss::test;


static TestSuite ts("version", {
    make_pair("version", [](TestSuite&) {
        KSS_ASSERT(!kss::thread::version().empty());
        KSS_ASSERT(!kss::thread::license().empty());
    })
});
