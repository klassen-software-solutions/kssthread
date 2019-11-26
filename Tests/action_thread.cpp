//
//  action_thread.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-01-19.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <kss/test/all.h>
#include <kss/thread/action_thread.hpp>

using namespace std;
using namespace kss::test;
using namespace kss::thread;


static TestSuite ts("action_thread", {
    make_pair("ActionThread<void>", [] {
        ActionThread<void> th;

        KSS_ASSERT(isEqualTo<int>(5, [&] {
            int counter = 0;
            for (int i = 0; i < 5; ++i) {
                auto fut = th.async([&counter] { ++counter; });
                fut.wait();
            }
            return counter;
        }));
    }),
    make_pair("ActionThread<int>", [] {
        ActionThread<int> th;

        KSS_ASSERT(isEqualTo<int>(15, [&] {
            int counter = 0;
            for (int i = 0; i < 3; ++i) {
                auto fut = th.async([] { return 5; });
                counter += fut.get();
            }
            return counter;
        }));
    })
});
