//
//  interruptible.cpp
//  kssthread
//
//  Created by Steven W. Klassen on 2014-11-22.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <atomic>
#include <cerrno>
#include <iostream>
#include <list>
#include <system_error>
#include <thread>
#include <type_traits>

#include <pthread.h>
#include <kss/contract/all.h>

#include "interruptible.hpp"

using namespace std;
using namespace kss::thread;

// Check that we have POSIX threads.
namespace {
    static_assert(is_same<std::thread::native_handle_type, pthread_t>::value,
                  "std::thread is not a POSIX thread on this architecture");
}

// MARK: Interruptible threads

void kss::thread::interruptionPoint() {
    pthread_testcancel();
}

void kss::thread::interrupt(std::thread &th) {
    int err = pthread_cancel(th.native_handle());
    if (err != 0 && err != ESRCH) {
        throw system_error(err, system_category(), "pthread_cancel");
    }
}

void kss::thread::interrupt() {
    int err = pthread_cancel(pthread_self());
    if (err != 0 && err != ESRCH) {
        throw system_error(err, system_category(), "pthread_cancel");
    }
}

namespace {
    static thread_local list<function<void()>> onInterruptedFns;

    static void cleanup(void*) {
        for (const auto& fn : onInterruptedFns) {
            fn();
        }
#if defined(__APPLE__)
        throw _private::Interrupted();
#endif
    }

    class Guard {
    public:
        Guard() {
            onInterruptedFns.clear();
            int err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldCancelState);
            if (err != 0) {
                throw system_error(err, system_category(), "pthread_setcancelstate");
            }

            err = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
            if (err != 0) {
                throw system_error(err, system_category(), "pthread_setcanceltype");
            }
        }

        ~Guard() noexcept {
            pthread_setcancelstate(oldCancelState, NULL);
        }

        Guard(const Guard&) = delete;
        Guard(Guard&&) = delete;
        Guard& operator=(const Guard&) = delete;
        Guard& operator=(Guard&&) = delete;

    private:
        int oldCancelState = 0;
    };
}

void interruptible::doCall(const function<void()>& fn) {
    Guard guard;
    exception_ptr eptr;

    // Bad things happen (Bus error 10 on a mac) if exceptions are thrown from
    // between the pthread_cleanup_push and pthread_cleanup_pop macros. To avoid
    // this we catch any exception, save it, then rethrow it after the pop
    // macro has completed.
    pthread_cleanup_push(cleanup, nullptr);
    try {
        fn();
    }
#if defined(__linux)
    // Unfortunately Linux uses the C++ exception mechanism for handling thread cancellation.
    // Hence we must catch its specific exception and rethrow it immediatly, otherwise we
    // will get a "FATAL: exception not rethrown" error message and the process will be
    // aborted.
    catch (const abi::__forced_unwind&) {
        throw;
    }
#else
    // Otherwise we catch our interrupted exception to ensure it goes no further.
    catch (const _private::Interrupted&) {
    }
#endif
    catch (...) {
        eptr = current_exception();
    }
    pthread_cleanup_pop(0);

    if (eptr) {
        rethrow_exception(eptr);
    }
}

void kss::thread::onInterrupted(const function<void ()> &fn) {
    onInterruptedFns.push_back(fn);
}
