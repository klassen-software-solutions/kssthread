//
//  semaphore.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2014-12-12.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <system_error>
#include <thread>

#include <kss/test/all.h>
#include <kss/thread/interruptible.hpp>
#include <kss/thread/semaphore.hpp>

using namespace std;
using namespace kss::thread;
using namespace kss::test;


static TestSuite ts("semaphore", {
make_pair("simple tests", [] {
    Semaphore s("/KssThreadTestSemaphore1", 2);
    KSS_ASSERT(s.try_lock() == true);
    KSS_ASSERT(s.try_lock() == true);
    KSS_ASSERT(s.try_lock() == false);

    s.unlock();
    KSS_ASSERT(s.try_lock() == true);
    KSS_ASSERT(s.try_lock() == false);

    s.unlock();
    s.lock();
    KSS_ASSERT(s.try_lock() == false);

    s.unlock();
    s.unlock();
}),
make_pair("will block threads", [] {
    Semaphore s("/KssThreadTestSemaphore2", 2);
    thread t1 { [&]{ s.lock(); }};
    thread t2 { [&]{ s.lock(); }};
    t1.join();
    t2.join();
    KSS_ASSERT(!t1.joinable() && !t2.joinable());
    std::thread t3 { [&]{ s.lock(); }};
    KSS_ASSERT(t3.joinable());
    s.unlock();
    t3.join();
    KSS_ASSERT(!t3.joinable());
    s.unlock();
}),
make_pair("is interruptible", [] {
    Semaphore s("/KssThreadTestSemaphore3", 1);
    // Test that semaphore::lock is thread interruptible.
    bool wasInterrupted = false;
    std::thread th { [&] {
        interruptible { [&] {
            onInterrupted([&]{ wasInterrupted = true; });
            s.lock();
        }};
    }};

    s.try_lock();
    KSS_ASSERT(th.joinable());
    interrupt(th);
    th.join();
    KSS_ASSERT(wasInterrupted);
})
});
