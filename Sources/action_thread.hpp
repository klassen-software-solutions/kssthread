//
//  action_thread.hpp
//  kssthread
//
//  Created by Steven W. Klassen on 2019-01-08.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#ifndef kssthread_action_thread_hpp
#define kssthread_action_thread_hpp

#include <atomic>
#include <condition_variable>
#include <future>
#include <iostream>
#include <thread>

namespace kss { namespace thread {

    /*!
     An action thread is a thread that will wait until it is given an action to run, then
     will run it asynchronously, returning an appropriate future.
     */
    class ActionThread {
    public:
        ActionThread() = default;
        ~ActionThread() noexcept;

        ActionThread(ActionThread&&) = default;
        ActionThread& operator=(ActionThread&&) = default;

        ActionThread(const ActionThread&) = delete;
        ActionThread& operator=(const ActionThread&) = delete;

        /*!
         Wake up the thread and start an action. This is not unlike std::async, except that
         it is much more efficient since it does not create/destroy a new thread for each
         call. But it is also more limited in that it must return a future<void>. (I've
         seen a number of solutions to this, but the whole purpose of this particular
         class is that it is to be efficient with high numbers of short calls. And each
         of the solutions carries additional overhead. But I'm open to ideas.)

         Note that it is an error to call this when the ActionThread is already running an
         action. Doing so will result in terminate() being called. The only way to avoid
         this for certain is to call get() or wait() on the returned future before calling
         this again.
         */
        template <class Fn, class... Args>
        std::future<void> async(Fn&& fn, Args&&... args)
        {
            std::packaged_task<void()> pt(std::bind(fn, args...));
            auto fut = pt.get_future();
            startTask(std::move(pt));
            return fut;
        }

    private:
        void startTask(std::packaged_task<void()>&& pt);
        void workerThreadFn();

        std::thread workerThread { [this] { workerThreadFn(); }};

        // The remaining items must all be protected by the lock and condition variable.
        std::mutex                  lock;
        std::condition_variable     cv;
        bool                        stopping = false;
        std::packaged_task<void()>  worker;
    };
}}

#endif 
