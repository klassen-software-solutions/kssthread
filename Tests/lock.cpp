//
//  lock.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2014-12-12.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <future>
#include <mutex>
#include <vector>

#include <kss/test/all.h>
#include <kss/thread/lock.hpp>

using namespace std;
using namespace kss::thread;
using namespace kss::test;


static TestSuite ts("lock", {
    make_pair("locked", [] {
        KSS_ASSERT(isEqualTo<int>(10, [] {
            int counter = 0;
            mutex m;
            vector<future<void>> futures;
            for (int i = 0; i < 10; ++i) {
                futures.push_back(async([&] {
                    locked(m, [&] { ++counter; });
                }));
            }

            for (auto& fut : futures) {
                fut.wait();
            }

            return counter;
        }));
    }),
    make_pair("uniqueLocked", [] {
        KSS_ASSERT(isEqualTo<int>(10, [] {
            int counter = 0;
            mutex m;
            vector<future<void>> futures;
            for (int i = 0; i < 10; ++i) {
                futures.push_back(async([&] {
                    uniqueLocked(m, [&] { ++counter; });
                }));
            }

            for (auto& fut : futures) {
                fut.wait();
            }

            return counter;
        }));
    }),
    make_pair("ifLocked (& TryLockGuard)", [] {
        KSS_ASSERT(isEqualTo<int>(10, [] {
            int counter = 0;
            mutex m;

            // The lock will be obtained.
            for (int i = 0; i < 10; ++i) {
                ifLocked(m, [&] {
                    ++counter;
                });
            }

            locked(m, [&] {
                // The lock will not be obtained.
                vector<future<void>> futures;
                for (int i = 0; i < 5; ++i) {
                    futures.push_back(async([&] {
                        ifLocked(m, [&] { ++counter; });
                    }));
                }

                for (auto& fut : futures) {
                    fut.wait();
                }
            });

            return counter;
        }));
    })
});
