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
    if (!checkPredicate()) {
        unique_lock<mutex> l(lock);
        if (!pred()) {
            cv.wait(l, [this] {
                interruptionPoint();
                return pred();
            });
        }
    }
}

void Condition::process(const function<bool ()> &fn) {
    lock_guard<mutex> l(lock);
    if (fn()) {
        cv.notify_all();
    }
}

bool Condition::checkPredicate() {
    lock_guard<mutex> l(lock);
    return pred();
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
    if (!incrementCounterAndCheck()) {
        unique_lock<mutex> l(lock);
        if (counter < n) {
            cv.wait(l, [this] {
                interruptionPoint();
                return (counter >= n);
            });
        }
    }
}

void Barrier::reset() {
    lock_guard<mutex> l(lock);
    counter = 0;
}

bool Barrier::incrementCounterAndCheck() {
    lock_guard<mutex> l(lock);
    ++counter;
    if (counter >= n) {
        cv.notify_all();
        return true;
    }
    return false;
}
