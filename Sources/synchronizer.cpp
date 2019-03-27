//
//  synchronizer.cpp
//  KSSCore
//
//  Created by Steven W. Klassen on 2014-12-16.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include "interruptible.hpp"
#include "synchronizer.hpp"

using namespace std;
using namespace kss::thread;


// MARK: Condition

void Condition::wait() {
    lock_guard_t l(lock);
    while (!pred()) {
        interruptionPoint();
        cv.wait(l);
    }
}

void Condition::process(const function<bool ()> &fn) {
    lock_guard_t l(lock);
    if (fn()) {
        cv.notify_all();
    }
}


// MARK: latch

void Latch::release() {
    process([this] {
        if (!hasBeenFreed) {
            hasBeenFreed = true;
            return true;
        }
        return false;
    });
}

void Latch::reset() {
    process([this] {
        hasBeenFreed = false;
        return false;
    });
}


// MARK: barrier

void Barrier::wait() {
    lock_guard_t l(lock);
    ++counter;
    if (counter >= n) {
        cv.notify_all();
        return;
    }
    while (counter < n) {
        interruptionPoint();
        cv.wait(l);
    }
}

void Barrier::reset() {
    lock_guard_t l(lock);
    counter = 0;
}
