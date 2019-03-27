//
//  interruptible.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2014-11-22.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <csignal>
#include <thread>
#include <kss/thread/signal.hpp>
#include "ksstest.hpp"

using namespace std;
using namespace kss::thread;
using namespace kss::test;


static TestSuite t("signal", {
    make_pair("signals in threads", [] {
        bool stopIt = false;
        thread th1([&stopIt]{
            signal::ignore(SIGUSR1);
            while (!stopIt) {
                this_thread::sleep_for(1ms);
            }
        });

        thread th2([&stopIt]{
            signal::ignore({ SIGUSR1, SIGUSR2 });
            while (!stopIt) {
                this_thread::sleep_for(1ms);
            }
        });

        this_thread::sleep_for(1ms);
        signal::send(th1, SIGUSR1);
        signal::send(th2, SIGUSR2);
        stopIt = true;
        th1.join();
        th2.join();
        KSS_ASSERT(true);   // Nothing to test except that we complete without error.
    }),
});
