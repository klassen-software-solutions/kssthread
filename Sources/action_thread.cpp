//
//  action_thread.cpp
//  kssthread
//
//  Created by Steven W. Klassen on 2019-01-08.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

// Note that since the whole purpose of the ActionThread is to minimize the thread
// re-use overhead, we deviate from our normal contract practice and only perform
// our condition checks whens when compiled for debugging.

#include "_contract.hpp"
#include "action_thread.hpp"

using namespace std;
using namespace kss::thread;

namespace contract = kss::thread::_private::contract;


namespace {
    inline bool isCallable(const packaged_task<void()>& fn) {
        return fn.valid();
    }
}

ActionThread::~ActionThread() noexcept {
    {
        unique_lock<mutex> l(lock);
        stopping = true;
        cv.notify_all();
    }

    if (workerThread.joinable()) {
        workerThread.join();
    }
}


void ActionThread::startTask(std::packaged_task<void ()> &&fn) {
    unique_lock<mutex> l(lock);

#if !defined(NDEBUG)
    contract::preconditions({ KSS_EXPR(!isCallable(worker)) });
#endif

    worker = move(fn);
    cv.notify_all();

#if !defined(NDEBUG)
    contract::postconditions({ KSS_EXPR(isCallable(worker)) });
#endif
}


void ActionThread::workerThreadFn() {
    while (true) {
        packaged_task<void()> localWorker;

        // Wait until a worker has been assigned.
        {
            unique_lock<mutex> l(lock);
            while (!stopping && !isCallable(worker)) { cv.wait(l); }

            if (stopping) { break; }
            localWorker.swap(worker);

#if !defined(NDEBUG)
            contract::conditions({ KSS_EXPR(!isCallable(worker)) });
#endif
        }

        // Run the worker.
#if !defined(NDEBUG)
        contract::conditions({ KSS_EXPR(isCallable(localWorker)) });
#endif
        localWorker();
    }
}
