//
//  interruptible.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2014-11-22.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <chrono>
#include <functional>
#include <thread>
#include <vector>

#include <pthread.h>
#include <kss/test/all.h>

#include <kss/thread/interruptible.hpp>

using namespace std;
using namespace kss::thread;
using namespace kss::test;

namespace {
    void shortPause() {
        this_thread::sleep_for(10us);
    }

    void mediumPause() {
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    void loopUntilInterrupted(bool* interrupted) {
        onInterrupted([interrupted] {
            *interrupted = true;
        });
        while (true) {
            shortPause();
        }
    }

    void* loopUntilInterrupted2(void* ptr) {
        interruptible { [&]{
            bool* interrupted = static_cast<bool*>(ptr);
            loopUntilInterrupted(interrupted);
        }};
        return nullptr;
    }

    class Finally {
    public:
        explicit Finally(function<void()>&& cleanupCode) : cleanupCode(cleanupCode) {}
        ~Finally() {
            cleanupCode();
        }
    private:
        function<void()> cleanupCode;
    };
}


static TestSuite t("interruptible", {
    make_pair("stops the correct std::thread", [] {
        bool interruptedThread1 = false;
        bool interruptedThread2 = false;
        std::thread t1 { [&]{ interruptible { loopUntilInterrupted, &interruptedThread1 }; }};
        std::thread t2 { [&]{ interruptible { loopUntilInterrupted, &interruptedThread2 }; }};
        KSS_ASSERT(t1.joinable() && t2.joinable());
        KSS_ASSERT(!interruptedThread1 && !interruptedThread2);
        interrupt(t1);
        t1.join();
        KSS_ASSERT(!t1.joinable() && t2.joinable());
        KSS_ASSERT(interruptedThread1 && !interruptedThread2);
        interrupt(t2);
        t2.join();
        KSS_ASSERT(!t1.joinable() && !t2.joinable());
        KSS_ASSERT(interruptedThread1 && interruptedThread2);
    }),
    make_pair("stops the correct pthread", [] {
        bool interruptedThread1 = false;
        bool interruptedThread2 = false;
        pthread_t t1, t2;
        pthread_create(&t1, NULL, &loopUntilInterrupted2, &interruptedThread1);
        pthread_create(&t2, NULL, &loopUntilInterrupted2, &interruptedThread2);
        KSS_ASSERT(!interruptedThread1 && !interruptedThread2);
        pthread_cancel(t1);
        pthread_join(t1, NULL);
        KSS_ASSERT(interruptedThread1 && !interruptedThread2);
        pthread_cancel(t2);
        pthread_join(t2, NULL);
        KSS_ASSERT(interruptedThread1 && interruptedThread2);
    }),
    make_pair("stops the current thread", [] {
        KSS_ASSERT(isTrue([] {
            bool interruptedThread = false;
            bool reachedWrongPoint = false;
            thread th { [&]{
                interruptible { [&]{
                    this_thread::sleep_for(10ms);
                    interrupt();
                    loopUntilInterrupted(&interruptedThread);
                    reachedWrongPoint = true;
                }};
            }};
            th.join();
            return (interruptedThread && !reachedWrongPoint);
        }));
    }),
    make_pair("interruptionPoint", [] {
        KSS_ASSERT(isTrue([] {
            bool wasInterrupted = false;
            thread th { [&]{
                interruptible { [&]{
                    onInterrupted([&]{
                        wasInterrupted = true;
                    });
                    while (true) interruptionPoint();
                }};
            }};
            mediumPause();
            interrupt(th);
            th.join();
            return wasInterrupted;
        }));
    }),
    make_pair("onInterrupted", [] {
        auto& ts = TestSuite::get();

        // Test that onInterrupted is not needed to interrupt a thread.
        KSS_ASSERT(isFalse([] {
            thread th { []{
                interruptible { []{
                    while (true) { shortPause(); }
                }};
            }};
            interrupt(th);
            th.join();
            return th.joinable();
        }));

        // Test that onInterrupted will work with multiple handlers in the correct order.
        {
            int counter = 0;
            thread th { [&counter, &ts, ctx=ts.testCaseContext()]{
                ts.setTestCaseContext(ctx);
                interruptible { [&]{
                    onInterrupted([&]{ KSS_ASSERT(++counter == 1); });
                    onInterrupted([&]{ KSS_ASSERT(++counter == 2); });
                    onInterrupted([&]{ KSS_ASSERT(++counter == 3); });
                    while (true) { shortPause(); }
                }};
            }};

            interrupt(th);
            th.join();
            KSS_ASSERT(counter == 3);
        }
    }),
    make_pair("interruptAll", [] {
        bool interruptedThread1 = false;
        bool interruptedThread2 = false;
        bool interruptedThread3 = false;
        thread t1 { [&]{ interruptible { loopUntilInterrupted, &interruptedThread1 }; }};
        thread t2 { [&]{ interruptible { loopUntilInterrupted, &interruptedThread2 }; }};
        thread t3 { [&]{ interruptible { loopUntilInterrupted, &interruptedThread3 }; }};
        vector<thread> grp1;
        grp1.push_back(move(t1));
        grp1.push_back(move(t2));
        grp1.push_back(move(t3));
        for (const auto& th : grp1) {
            KSS_ASSERT(th.joinable());
        }
        interruptAll(grp1);
        for (auto& th : grp1) th.join();
        KSS_ASSERT(interruptedThread1 && interruptedThread2 && interruptedThread3);

        interruptedThread1 = false;
        interruptedThread2 = false;
        interruptedThread3 = false;
        thread t7 { [&]{ interruptible { loopUntilInterrupted, &interruptedThread1 }; }};
        thread t8 { [&]{ interruptible { loopUntilInterrupted, &interruptedThread2 }; }};
        thread t9 { [&]{ interruptible { loopUntilInterrupted, &interruptedThread3 }; }};
        interruptAll(t7, t8, t9);
        t7.join();
        t8.join();
        t9.join();
        KSS_ASSERT(interruptedThread1 && interruptedThread2 && interruptedThread3);
    }),
    make_pair("thrown exceptions are propagated", [] {
        KSS_ASSERT(isTrue([] {
            bool caughtIt = false;
            thread th([&]{
                try {
                    interruptible { []{
                        for (;;) {
                            this_thread::sleep_for(1ms);
                            throw runtime_error("hi");
                        }
                    }};
                }
                catch (const runtime_error& e) {
                    caughtIt = true;
                }
            });
            th.join();
            return caughtIt;
        }));
    }),
    make_pair("rethrow macro", [] {
        // Test catching inside interruptible.
        KSS_ASSERT(isTrue([] {
            bool wasCleanedProperly = false;
            thread th([&]{
                Finally cleanup([&wasCleanedProperly]{
                    wasCleanedProperly = true;
                });

                interruptible { [&]{
                    try {
                        for (;;) { shortPause(); }
                    }
                    KSS_THREAD_RETHROW_CANCELLED_EX
                    catch (...) {
                    }
                }};
            });

            this_thread::yield();
            interrupt(th);
            th.join();
            return wasCleanedProperly;
        }));

        // Test catching outside interruptible.
        KSS_ASSERT(isTrue([] {
            bool wasCleanedProperly = false;
            thread th([&]{
                Finally cleanup([&wasCleanedProperly]{
                    wasCleanedProperly = true;
                });

                try {
                    interruptible { [&]{
                        for (;;) { shortPause(); }
                    }};
                }
                KSS_THREAD_RETHROW_CANCELLED_EX
                catch (...) {
                }
            });

            this_thread::yield();
            interrupt(th);
            th.join();
            return wasCleanedProperly;
        }));
    })
});
