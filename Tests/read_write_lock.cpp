//
//  read_write_lock.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2014-12-12.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <system_error>
#include <thread>

#include <kss/thread/read_write_lock.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::thread;
using namespace kss::test;


static TestSuite ts("read_write_lock", {
    make_pair("Write lock blocks all", [] {
        ReadWriteLock l;
        bool hadLock1 = false;
        bool hadLock2 = false;
        auto& wl = l.writeLock();
        auto& rl = l.readLock();

        l.writeLock().lock();
        thread t1 { [&]{
            lock_guard<ReadWriteLock::ReadLock> lock(rl);
            hadLock1 = true;
        }};
        thread t2 { [&]{
            lock_guard<ReadWriteLock::WriteLock> lock(wl);
            hadLock2 = true;
        }};
        KSS_ASSERT(!hadLock1 && !hadLock2);

        wl.unlock();
        t1.join();
        t2.join();
        KSS_ASSERT(hadLock1 && hadLock2);
    }),
    make_pair("Read lock blocks write", [] {
        ReadWriteLock l;
        bool hadLock1 = false;
        bool hadLock2 = false;
        auto& wl = l.writeLock();
        auto& rl = l.readLock();

        rl.lock();
        thread t1 { [&]{
            lock_guard<ReadWriteLock::ReadLock> lock(rl);
            hadLock1 = true;
        }};
        thread t2 { [&]{
            while (!hadLock1) { this_thread::yield(); } // Ensure thread1 gets the lock first.
            lock_guard<ReadWriteLock::WriteLock> lock(wl);
            hadLock2 = true;
        }};

        t1.join();
        KSS_ASSERT(hadLock1 && !hadLock2);

        rl.unlock();
        t2.join();
        KSS_ASSERT(hadLock1 && hadLock2);
    }),
    make_pair("Pending write lock blocks all", [] {
        ReadWriteLock l;
        bool hadLock1 = false;
        bool hadLock2 = false;
        bool releaseT1 = false;
        auto& wl = l.writeLock();
        auto& rl = l.readLock();

        rl.lock();

        thread t1 { [&]{
            lock_guard<ReadWriteLock::WriteLock> lock(wl); // This should block until the read lock is released.
            hadLock1 = true;
            while (!releaseT1) { this_thread::yield(); }   // Don't release the lock until we are told.
        }};

        thread t2 { [&]{
            // Try to get t1 to lock first
            while (!rl.try_lock()) { this_thread::yield(); }
            rl.unlock();
            this_thread::sleep_for(10ms);

            // This should now block until t1 releases its lock.
            lock_guard<ReadWriteLock::ReadLock> lock(rl);
            hadLock2 = true;
        }};

        KSS_ASSERT(!hadLock1 && !hadLock2);     // At this point both t1 and t2 should be blocked.

        rl.unlock();
        while (!hadLock1) { this_thread::yield(); }
        KSS_ASSERT(hadLock1 && !hadLock2);      // At this point t1 should have obtained its lock but t2 should still be blocked.

        releaseT1 = true;
        t1.join();
        t2.join();
        KSS_ASSERT(hadLock1 && hadLock2);
    })
});
