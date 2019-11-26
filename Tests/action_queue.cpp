//
//  action_queue.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-12-20.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

#include <kss/test/all.h>
#include <kss/util/all.h>
#include <kss/thread/action_queue.hpp>

using namespace std;
using namespace kss::test;
using namespace kss::thread;

using kss::util::time::now;
using kss::util::time::timeOfExecution;
using time_point_t = chrono::time_point<chrono::steady_clock, chrono::milliseconds>;

namespace {
    class ActionQueueTestSuite : public TestSuite, public HasBeforeEach {
    public:
        ActionQueueTestSuite(test_case_list_t fns) : TestSuite("action_queue", fns) {}

        void beforeEach() override {
            auto& ts = TestSuite::get();
            queue.reset(new ActionQueue());
            queue->addAction([&ts, ctx=ts.testCaseContext()] {
                ts.setTestCaseContext(ctx);
            });
        }

        unique_ptr<ActionQueue> queue;
    };

    ActionQueue& getQueue() {
        auto& ts = dynamic_cast<ActionQueueTestSuite&>(TestSuite::get());
        return *ts.queue;
    }

    void resetQueue() {
        auto& ts = dynamic_cast<ActionQueueTestSuite&>(TestSuite::get());
        ts.queue.reset();
    }
}

static ActionQueueTestSuite ts({
    make_pair("ActionQueue error conditions", [] {
        resetQueue();
        ActionQueue queue1(2);

        queue1.addAction(100s, []{
            // should never run.
            KSS_ASSERT(false);
        });
        queue1.addAction(100s, []{
            // should never run.
            KSS_ASSERT(false);
        });

        KSS_ASSERT(throwsException<system_error>([&] { queue1.addAction([]{}); }));
        KSS_ASSERT(throwsException<invalid_argument>([&] { queue1.addAction(-1s, []{}); }));
    }),
    make_pair("ActionQueue non-timed", [] {
        auto& queue = getQueue();
        int counter = 0;
        for (int i = 0; i < 5; ++i) {
            queue.addAction([&]{ ++counter; });
        }
        queue.wait();
        KSS_ASSERT(counter == 5);
    }),
    make_pair("ActionQueue timed", [] {
        int counter = 0;
        const auto start = now<time_point_t>();
        auto& queue = getQueue();
        for (int i = 0; i < 5; ++i) {
            const auto pause = chrono::milliseconds(i*10);
            queue.addAction(pause, [&counter, &start, pause]{
                ++counter;
                const auto end = now<time_point_t>();
                KSS_ASSERT(end >= (start + pause));
            });
        }
        queue.wait();
        KSS_ASSERT(counter == 5);
    }),
    make_pair("ActionQueue mixed", [] {
        int immediateTick = 0;
        int slowTick = 0;
        int fastTick = 0;
        const auto start = now<time_point_t>();

        auto& queue = getQueue();

        queue.addAction([&] {
            ++immediateTick;
            KSS_ASSERT(now<time_point_t>() < (start + 10ms));
        });

        for (int i = 0; i < 3; ++i) {
            const auto pause = chrono::milliseconds(i*50);
            queue.addAction(pause, [&slowTick, start, pause]{
                ++slowTick;
                KSS_ASSERT(now<time_point_t>() >= (start + pause));
            });
        }

        queue.addAction([&] {
            ++immediateTick;
            KSS_ASSERT(now<time_point_t>() < (start + 10ms));
        });

        for (int i = 0; i < 15; ++i) {
            const auto pause = chrono::milliseconds(i*10);
            queue.addAction(pause, [&fastTick, start, pause]{
                ++fastTick;
                KSS_ASSERT(now<time_point_t>() >= (start + pause));
            });
        }

        queue.addAction([&] {
            ++immediateTick;
            KSS_ASSERT(now<time_point_t>() < (start + 15ms));
            KSS_ASSERT(slowTick <= 1);
            KSS_ASSERT(fastTick <= 1);
        });

        queue.wait();
        KSS_ASSERT(slowTick == 3);
        KSS_ASSERT(fastTick == 15);
        KSS_ASSERT(immediateTick == 3);
    }),
    make_pair("ActionQueue destructor does not wait for pending actions", [] {
        const auto t = timeOfExecution([]{
            auto& queue = getQueue();
            queue.addAction(1s, []{
                // should never run
                KSS_ASSERT(false);
            });
            resetQueue();
        });
        // Cannot use exact matches for timing results, but this should easily pass.
        KSS_ASSERT(t < 900ms);
    }),
    make_pair("RepeatingAction", [] {
        int immediateTick = 0;
        int slowTick = 0;
        int fastTick = 0;
        const auto start = now<time_point_t>();

        auto& queue = getQueue();

        {
            const auto slowPause = chrono::milliseconds(50);
            RepeatingAction slowAction(slowPause, queue, [&slowTick, start, slowPause] {
                ++slowTick;
                KSS_ASSERT(now<time_point_t>() >= (start + (slowPause * slowTick)));
            });

            const auto fastPause = chrono::milliseconds(10);
            RepeatingAction fastAction(fastPause, queue, [&fastTick, start, fastPause] {
                ++fastTick;
                KSS_ASSERT(now<time_point_t>() >= (start + (fastPause * fastTick)));
            });

            queue.addAction([&] {
                ++immediateTick;
                KSS_ASSERT(now<time_point_t>() < (start + 15ms));
                KSS_ASSERT(slowTick <= 1);
                KSS_ASSERT(fastTick <= 1);
            });

            this_thread::sleep_for(fastPause * 16);
        }

        queue.wait();
        // timings are not accurate enough for an exact value, but they should be in
        // this range.
        KSS_ASSERT(slowTick >= 1 && slowTick <= 5);
        KSS_ASSERT(fastTick >= 5 && fastTick <= 18);
        KSS_ASSERT(immediateTick == 1);
    }),
    make_pair("RepeatingAction destructor does not wait for pending actions", [] {
        const auto t = timeOfExecution([]{
            auto& queue = getQueue();
            {
                RepeatingAction ra(1s, queue, []{
                    // should never run
                    KSS_ASSERT(false);
                });
            }
            resetQueue();
        });
        // Cannot use exact matches for timing results, but this should easily pass.
        KSS_ASSERT(t < 100ms);
    })
});
