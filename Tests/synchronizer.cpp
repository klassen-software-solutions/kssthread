//
//  synchronizer.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2014-12-16.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <chrono>
#include <iostream>
#include <thread>

#include <kss/thread/interruptible.hpp>
#include <kss/thread/synchronizer.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::thread;
using namespace kss::test;


static TestSuite ts("synchronizer", {
    make_pair("Condition", [] {
        {
            // Trivial condition syntax test. Shouldn't cause any waiting at all.
            Condition cond([] { return true; });

            cond.wait();
            KSS_ASSERT(cond.waitFor(100s) == true);
            KSS_ASSERT(cond.waitUntil(chrono::steady_clock::now() + 100s) == true);
        }
        {
            // Test that a couple of threads will wait for the condition to become true.
            bool passedPoint1 = false;
            bool passedPoint2 = false;
            int value = 0;
            Condition cond([&] { return (value > 100); });

            thread t1 { [&]{
                cond.wait();
                passedPoint1 = true;
            }};

            thread t2 { [&]{
                cond.wait();
                passedPoint2 = true;
            }};

            this_thread::yield();
            KSS_ASSERT(!passedPoint1 && !passedPoint2);
            KSS_ASSERT(cond.waitFor(1ns) == false);
            KSS_ASSERT(cond.waitUntil(chrono::steady_clock::now()) == false);

            cond.process([&]{
                value = 110;
                return true;
            });

            t1.join();
            t2.join();
            KSS_ASSERT(passedPoint1 && passedPoint2);
        }
#if !defined(__APPLE__)
        // Test that condition::wait is a thread interruption point.
        KSS_ASSERT(isTrue([] {
            bool wasInterrupted = false;
            int value = 0;
            Condition cond([&] { return (value > 100); });
            thread th { [&]{
                interruptible { [&]{
                    onInterrupted([&]{
                        wasInterrupted = true;
                    });
                    cond.wait();
                }};
            }};
            interrupt(th);
            th.join();
            return wasInterrupted;
        }));
#endif
    }),
    make_pair("Latch", [] {
        Latch l;
        {
            // Test that the latch will block threads.
            thread t1 { [&]{ l.wait(); }};
            thread t2 { [&]{ l.wait(); }};
            KSS_ASSERT(l.waitFor(1ns) == false);
            KSS_ASSERT(l.waitUntil(chrono::steady_clock::now()) == false);
            KSS_ASSERT(t1.joinable() && t2.joinable());

            l.release();
            t1.join();
            t2.join();
            KSS_ASSERT(!t1.joinable() && !t2.joinable());
        }

        // Test that a released latch will not block threads.
        {
            thread th { [&]{ l.wait(); }};
            th.join();
            KSS_ASSERT(!th.joinable());
            KSS_ASSERT(l.waitFor(100s) == true);
            KSS_ASSERT(l.waitUntil(chrono::steady_clock::now() + 100s) == true);
        }

        {
            // Test that a latch can be reset.
            l.reset();
            thread t1 { [&]{ l.wait(); }};
            thread t2 { [&]{ l.wait(); }};
            KSS_ASSERT(t1.joinable() && t2.joinable());

            l.release();
            t1.join();
            t2.join();
            KSS_ASSERT(!t1.joinable() && !t2.joinable());
        }
#if !defined(__APPLE__)
        // Test that latch::wait() is a thread interruption point.
        KSS_ASSERT(isTrue([&] {
            bool wasInterrupted = false;
            l.reset();
            thread th { [&]{
                interruptible { [&]{
                    onInterrupted([&]{
                        wasInterrupted = true;
                    });
                    l.wait();
                }};
            }};

            interrupt(th);
            th.join();
            return wasInterrupted;
        }));
#endif
    }),
    make_pair("Barrier", [] {
        Barrier b(3);
        {
            // Test that the barrier will block threads.
            KSS_ASSERT(b.waitFor(1ns) == false);
            KSS_ASSERT(b.waitUntil(chrono::steady_clock::now()) == false);
            thread t1 { [&]{ b.wait(); }};
            thread t2 { [&]{ b.wait(); }};
            KSS_ASSERT(t1.joinable() && t2.joinable());

            b.wait();
            KSS_ASSERT(b.waitFor(1ns) == true);
            KSS_ASSERT(b.waitUntil(chrono::steady_clock::now()) == true);
            t1.join();
            t2.join();
            KSS_ASSERT(!t1.joinable() && !t2.joinable());
        }

        // Test that a released barrier will not block threads.
        KSS_ASSERT(isFalse([&] {
            thread th { [&]{ b.wait(); }};
            th.join();
            return th.joinable();
        }));

        {
            // Test that a barrier can be reset.
            b.reset();
            thread t1 { [&]{ b.wait(); }};
            thread t2 { [&]{ b.wait(); }};
            KSS_ASSERT(t1.joinable() && t2.joinable());

            b.wait();
            t1.join();
            t2.join();
            KSS_ASSERT(!t1.joinable() && !t2.joinable());
        }
#if !defined(__APPLE__)
        // Test that barrier::wait() is a thread interruption point.
        KSS_ASSERT(isTrue([&] {
            bool wasInterrupted = false;
            b.reset();
            thread th { [&]{
                interruptible { [&]{
                    onInterrupted([&]{ wasInterrupted = true; });
                    b.wait();
                }};
            }};

            interrupt(th);
            th.join();
            return wasInterrupted;
        }));
#endif
    })
});
