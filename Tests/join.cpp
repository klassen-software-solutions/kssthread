//
//  join.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2014-11-22.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <thread>
#include <vector>

#include <kss/thread/join.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::thread;
using namespace kss::test;

namespace {
    void mediumPause() {
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}


static TestSuite t("join", {
    make_pair("joinAll", [] {
        vector<std::thread> thgrp1;
        thgrp1.push_back(thread([]{ mediumPause(); }));
        thgrp1.push_back(thread([]{ mediumPause(); }));
        thgrp1.push_back(thread([]{ mediumPause(); }));
        joinAll(thgrp1);
        for (const thread& th : thgrp1) {
            KSS_ASSERT(!th.joinable());
        }

        thgrp1.clear();
        thgrp1.push_back(thread([]{ mediumPause(); }));
        thgrp1.push_back(thread([]{ mediumPause(); }));
        thgrp1.push_back(thread([]{ mediumPause(); }));
        joinAll(thgrp1.begin(), thgrp1.end());
        for (const thread& th : thgrp1) {
            KSS_ASSERT(!th.joinable());
        }

        thread t1 { []{ mediumPause(); }};
        thread t2 { []{ mediumPause(); }};
        thread t3 { []{ mediumPause(); }};
        joinAll( t1, t2, t3 );
        KSS_ASSERT(!t1.joinable() && !t2.joinable() && !t3.joinable());
    }),
    make_pair("joinAll on unjoinable", [] {
        vector<thread> v(5);
        joinAll(v);
        joinAll(v.begin(), v.end());
        joinAll(v[0], v[1], v[2], v[3], v[4]);
        KSS_ASSERT(true);   // Nothing to test except we complete without error.
    })
});
