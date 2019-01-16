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
#include <stdexcept>
#include <thread>

#include <kss/thread/action_queue.hpp>
#include <kss/thread/utility.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::test;
using namespace kss::thread;

using kss::thread::_private::now;
using kss::thread::_private::timeOfExecution;
using time_point_t = chrono::time_point<chrono::steady_clock, chrono::milliseconds>;


static TestSuite ts("action_queue", {
    make_pair("ActionQueue error conditions", [](TestSuite&) {
        ActionQueue queue(2);

        queue.addAction(100s, []{
            // should never run.
            KSS_ASSERT(false);
        });
        queue.addAction(100s, []{
            // should never run.
            KSS_ASSERT(false);
        });

        KSS_ASSERT(throwsException<system_error>([&] {
            queue.addAction([]{});
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            queue.addAction(-1s, []{});
        }));
    }),
    make_pair("ActionQueue non-timed", [](TestSuite&) {
        int counter = 0;
        ActionQueue queue;

        for (int i = 0; i < 5; ++i) {
            queue.addAction([&]{ ++counter; });
        }
        queue.wait();
        KSS_ASSERT(counter == 5);
    }),
    make_pair("ActionQueue timed", [](TestSuite&) {
        int counter = 0;
        const auto start = now<time_point_t>();
        array<time_point_t, 5> finishTimes;
        ActionQueue queue;
        for (int i = 0; i < finishTimes.size(); ++i) {
            const auto pause = chrono::milliseconds(i*10);
            queue.addAction(pause, [&counter, &finishTimes, i]{
                ++counter;
                finishTimes[i] = now<time_point_t>();
            });
        }

        queue.wait();
        for (int i = 0; i < finishTimes.size(); ++i) {
            const auto pause = chrono::milliseconds(i*10);
            KSS_ASSERT(finishTimes[i] >= (start + pause));
        }
        KSS_ASSERT(counter == 5);
    }),
#if 0
    make_pair("ActionQueue mixed", [](TestSuite&) {
        int immediateTick = 0;
        int slowTick = 0;
        int fastTick = 0;
        const auto start = now<time_point_t>();

        ActionQueue queue;

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
    make_pair("ActionQueue destructor does not wait for pending actions", [](TestSuite&) {
        const auto t = timeOfExecution([]{
            ActionQueue queue;
            queue.addAction(1s, []{
                // should never run
                KSS_ASSERT(false);
            });
        });
        // Cannot use exact matches for timing results, but this should easily pass.
        KSS_ASSERT(t < 100ms);
    }),
    make_pair("RepeatingAction", [](TestSuite&) {
        int immediateTick = 0;
        int slowTick = 0;
        int fastTick = 0;
        const auto start = now<time_point_t>();

        ActionQueue queue;

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
    make_pair("RepeatingAction destructor does not wait for pending actions", [](TestSuite&) {
        const auto t = timeOfExecution([]{
            ActionQueue queue;
            RepeatingAction ra(1s, queue, []{
                // should never run
                KSS_ASSERT(false);
            });
        });
        // Cannot use exact matches for timing results, but this should easily pass.
        KSS_ASSERT(t < 100ms);
    })
#endif
});
