//
//  action_thread.hpp
//  kssthread
//
//  Created by Steven W. Klassen on 2019-01-08.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//
// Note that since the whole purpose of the ActionThread is to minimize the thread
// re-use overhead, we deviate from our normal contract practice and only perform
// our condition checks whens when compiled for debugging, via the assert macro.
//

#ifndef kssthread_action_thread_hpp
#define kssthread_action_thread_hpp

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <thread>

namespace kss { namespace thread {

    namespace _private {

        template <class T>
        inline bool isCallable(const std::packaged_task<T()>& fn) {
            return fn.valid();
        }
    }

    /*!
     An action thread is a thread that will wait until it is given an action to run, then
     will run it asynchronously, returning an appropriate future.
     */
    template <class T>
    class ActionThread {
    public:
        ActionThread() = default;

        ~ActionThread() noexcept {
            stopping = true;
            cv.notify_all();
            if (workerThread.joinable()) {
                workerThread.join();
            }
        }

        ActionThread(ActionThread&&) = default;
        ActionThread& operator=(ActionThread&&) = default;

        ActionThread(const ActionThread&) = delete;
        ActionThread& operator=(const ActionThread&) = delete;

        /*!
         Wake up the thread and start an action. This is not unlike std::async, except that
         it is much more efficient since it does not create/destroy a new thread for each
         call. But it is also more limited in that it must return a future<T>.

         Note that it is an error to call this when the ActionThread is already running an
         action. Doing so will will cause (in debug mode) an assertion to fail. Note that
         the only way to avoid this for certain is to call get() or wait() on the returned
         future before calling this method again.
         */
        template <class Fn, class... Args>
        std::future<T> async(Fn&& fn, Args&&... args) {
            std::packaged_task<T()> pt(std::bind(fn, args...));
            auto fut = pt.get_future();
            startTask(std::move(pt));
            return fut;
        }

    private:
        void startTask(std::packaged_task<T()>&& pt) {
            std::unique_lock<std::mutex> l(lock);
            assert(!_private::isCallable(worker));
            worker = move(pt);
            cv.notify_all();
            assert(_private::isCallable(worker));
        }

        std::thread workerThread { [this] {
            while (!stopping) {
                std::packaged_task<T()> localWorker;

                // Wait until a worker has been assigned.
                {
                    std::unique_lock<std::mutex> l(lock);
                    while (!stopping && !_private::isCallable(worker)) {
                        cv.wait(l);
                    }

                    if (stopping) {
                        break;
                    }
                    localWorker.swap(worker);

                    assert(!_private::isCallable(worker));
                }

                // Run the worker.
                assert(_private::isCallable(localWorker));
                if (stopping) {
                    break;
                }
                else {
                    localWorker();
                }
            }
        }};

        std::mutex              lock;
        std::condition_variable cv;
        std::atomic<bool>       stopping { false };
        std::packaged_task<T()> worker;
    };
}}

#endif 
