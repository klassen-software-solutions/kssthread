//
//  semaphore.cpp
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

#include <kss/thread/semaphore.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::thread;
using namespace kss::test;

namespace {

    class MyTestSuite : public TestSuite, public HasBeforeAll, public HasAfterAll {
    public:
        MyTestSuite(const string& name, test_case_list_t fns) : TestSuite(name, fns) {}

        void beforeAll() override {
            _semaphore.reset(new Semaphore("/KssThreadSemaphoreTest", 2));
        }

        void afterAll() override {
            // Check that the semaphore is still valid.
            KSS_ASSERT((bool)_semaphore == true);
            if ((bool)_semaphore) {
                _semaphore.reset();
            }
        }

        static Semaphore& semaphore() noexcept {
            return *dynamic_cast<MyTestSuite&>(TestSuite::get())._semaphore;
        }

    private:
        unique_ptr<Semaphore> _semaphore;
    };
}


static MyTestSuite ts("semaphore", {
    make_pair("Simple Tests", [] {
        auto& s = MyTestSuite::semaphore();
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
    make_pair("Will Block Threads", [] {
        auto& s = MyTestSuite::semaphore();
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
    })
});

#if 0
// TODO: put this test back in once interruptible has been moved
static TestSet ts("lock", {
    []{
        KSS_TEST_GROUP("semaphore");
        const auto name = kss::util::file::temporary_filename("kssthreadsem");
        {
            semaphore s(name, 2);
            {
                // Test that semaphore::lock is thread interruptible.
                bool wasInterrupted = false;
                std::thread th { [&] {
                    interruptible { [&] {
                        on_interrupted([&]{ wasInterrupted = true; });
                        s.lock();
                    }};
                }};

                std::this_thread::yield();
                KSS_ASSERT(th.joinable());
                interrupt(th);
                th.join();
                KSS_ASSERT(wasInterrupted);
            }
        }
    },
});
#endif
