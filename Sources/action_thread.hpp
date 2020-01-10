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
// our condition checks whens when compiled for debugging.
//

#ifndef kssthread_action_thread_hpp
#define kssthread_action_thread_hpp

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <thread>

#include <kss/contract/all.h>

#include "lock.hpp"

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
            locked(lock, [this] { stopping = true; });
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
         action. Doing so will will cause (in debug mode) an condition to fail. Note that
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
            std::lock_guard<std::mutex> l(lock);

#           if !defined(NDEBUG)
            kss::contract::preconditions({ KSS_EXPR(!_private::isCallable(worker)) });
#           endif

            worker = move(pt);
            cv.notify_all();

#           if !defined(NDEBUG)
            kss::contract::postconditions({ KSS_EXPR(_private::isCallable(worker)) });
#           endif
        }

        std::thread workerThread { [this] {
            while (true) {
                std::packaged_task<T()> localWorker;

                // Wait until a worker has been assigned.
                {
                    std::unique_lock<std::mutex> l(lock);

                    if (stopping) { break; }
                    cv.wait(l, [this] { return stopping || _private::isCallable(worker); });

                    if (stopping) { break; }
                    localWorker.swap(worker);

#                   if !defined(NDEBUG)
                    kss::contract::conditions({ KSS_EXPR(!_private::isCallable(worker)) });
#                   endif
                }

                // Run the worker.
#               if !defined(NDEBUG)
                kss::contract::conditions({ KSS_EXPR(_private::isCallable(localWorker)) });
#               endif
                localWorker();
            }
        }};

        std::mutex              lock;
        std::condition_variable cv;
        bool                    stopping = false;
        std::packaged_task<T()> worker;
    };
}}

#endif 
