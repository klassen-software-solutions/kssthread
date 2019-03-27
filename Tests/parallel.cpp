//
//  parallel.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2003-09-22.
//  Copyright (c) Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include <kss/thread/parallel.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace std::chrono;
using namespace kss::thread;
using namespace kss::test;

namespace {
    void run(atomic<int>& numRan) {
        this_thread::sleep_for(10ms);
        ++numRan;
    }

    // "borrowed" from kssutil
    milliseconds timeOfExecution(const function<void ()>& fn) {
        const auto start = high_resolution_clock::now();
        fn();
        return duration_cast<milliseconds>(high_resolution_clock::now() - start);
    }
}


static TestSuite ts("parallel", {
    make_pair("non-cached version", [] {
        KSS_ASSERT(isEqualTo<int>(2, [] {
            atomic<int> numRan { 0 };
            parallel([&numRan]{ run(numRan); },
                     [&numRan]{ run(numRan); });
            return numRan.load();
        }));

        KSS_ASSERT(isEqualTo<int>(3, [] {
            atomic<int> numRan { 0 };
            parallel([&numRan]{ run(numRan); },
                     [&numRan]{ run(numRan); },
                     [&numRan]{ run(numRan); });
            return numRan.load();
        }));

        KSS_ASSERT(isEqualTo<int>( 4, [] {
            atomic<int> numRan { 0 };
            parallel([&numRan]{ run(numRan); },
                     [&numRan]{ run(numRan); },
                     [&numRan]{ run(numRan); },
                     [&numRan]{ run(numRan); });
            return numRan.load();
        }));
    }),
    make_pair("cached version", [] {
        constexpr size_t numIterations = 1000;
        constexpr auto smallDelay = 90us;
        atomic<int> numRan { 0 };

        const auto tSerial = duration_cast<milliseconds>(smallDelay * (numIterations*3));

        const auto t = timeOfExecution([&] {
            for (size_t i = 0; i < numIterations; ++i) {
                parallel([&]{ this_thread::sleep_for(smallDelay); ++numRan; },
                         [&]{ this_thread::sleep_for(smallDelay); ++numRan; },
                         [&]{ this_thread::sleep_for(smallDelay); ++numRan; });
            }
        });
        KSS_ASSERT(numRan == numIterations*3);
        KSS_ASSERT(t < tSerial);

        numRan = 0;
        const auto tpg = timeOfExecution([&] {
            ParallelThreadGroup tg(3);
            for (size_t i = 0; i < numIterations; ++i) {
                parallel(tg,
                         [&]{ this_thread::sleep_for(smallDelay); ++numRan; },
                         [&]{ this_thread::sleep_for(smallDelay); ++numRan; },
                         [&]{ this_thread::sleep_for(smallDelay); ++numRan; });
            }
        });
        KSS_ASSERT(numRan == numIterations*3);
        KSS_ASSERT((tpg < t) && (tpg < tSerial));
    })
});
