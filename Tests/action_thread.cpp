//
//  action_thread.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-01-19.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#include <kss/thread/action_thread.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::test;
using namespace kss::thread;


static TestSuite ts("action_thread", {
    make_pair("basic tests", [] {
        ActionThread th;

        KSS_ASSERT(isEqualTo<int>(5, [&] {
            int counter = 0;
            for (int i = 0; i < 5; ++i) {
                auto fut = th.async([&counter] { ++counter; });
                fut.wait();
            }
            return counter;
        }));
    })
});
