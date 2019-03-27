//
//  signal.cpp
//  kssthread
//
//  Created by Steven W. Klassen on 2014-11-22.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <csignal>
#include <system_error>
#include <pthread.h>
#include "signal.hpp"

using namespace std;
using namespace kss::thread;


void kss::thread::signal::send(std::thread& th, int sig) {
    const auto err = pthread_kill(th.native_handle(), sig);
    if (err != 0) {
        throw system_error(err, system_category(), "pthread_kill");
    }
}

void kss::thread::signal::ignore(initializer_list<int> signals) {
    // Setup the signal mask.
    sigset_t mask;
    if (sigemptyset(&mask) == -1) {
        throw system_error(errno, system_category(), "sigemptyset");
    }
    for (int sig : signals) {
        if (sigaddset(&mask, sig) == -1) {
            throw system_error(errno, system_category(), "sigaddset");
        }
    }

    // Apply the mask.
    const auto err = pthread_sigmask(SIG_BLOCK, &mask, nullptr);
    if (err != 0) {
        throw system_error(err, system_category(), "pthread_sigmask");
    }
}
