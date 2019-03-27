//
//  synchronizer.hpp
//  kssthread
//
//  Created by Steven W. Klassen on 2014-12-16.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssthread_synchronizer_hpp
#define kssthread_synchronizer_hpp

#include <condition_variable>
#include <chrono>
#include <functional>
#include <mutex>

namespace kss { namespace thread {

    /*!
     \file

     A synchronizer is a class used to synchronize the position between threads.
     This is not unlike a lock, and in fact a synchronizer requires a lock, the key
     difference being that a lock is used to protect a critical section of code from
     being executed in multiple threads at the same time while a synchronizer is used
     to get multiple threads to a known point at the same time.

     The synchonizer classes are not based on a common parent, but rather must simply
     contain the following methods:

     @code
     // Wait until the appropriate condition is true.
     void wait();

     // Wait until the appropriate condition is true or the given time has elapsed.
     // Returns true if the condition is true and false if the timeout occurred.
     // Note that Duration must be a class that follows the chrono::duration
     // semantics.
     template <class Duration>
     bool waitFor(const Duration& timeout);

     // Wait until the appropriate condition is true or the given time point has been
     // reached. Returns true if the condition is true and false if the timeout
     // occurred. Note that TimePoint must be a class that follows the
     // chrono::time_point semantics.
     template <class TimePoint>
     bool waitUntil(const TimePoint& tp);
     @endcode
     */


    /*!
     Condition based synchronization. This provides a synchronization that will wait
     until a given predicate returns true.
     */
    class Condition {
    public:
        using predicate_t = std::function<bool()>;

        explicit Condition(const predicate_t& pred) : pred(pred) {}
        explicit Condition(predicate_t&& pred) : pred(move(pred)) {}
        ~Condition() = default;

        // Moving is allowed, copying is not.
        Condition(Condition&&) = default;
        Condition& operator=(Condition&&) = default;
        Condition(const Condition&) = delete;
        Condition& operator=(const Condition&) = delete;

        /*!
         Wait for the condition to become true. Note that this only guarantees that
         the condition is true momentarily. It could become false again shortly.
         This is also a thread interruption point.
         @throws kss::thread::Interrupted if interrupted in an interruptible section
         @throws any exception that may be thrown by the underlying mutex or
            condition variable, or by the predicate.
         */
        void wait();

        /*!
         Wait up to a given duration. See wait() for more details.
         */
        template <class Duration>
        bool waitFor(const Duration& dur) {
            lock_guard_t l(lock);
            if (pred()) {
                return true;
            }
            if (cv.wait_for(l, dur) == std::cv_status::timeout) {
                return false;
            }
            return pred();
        }

        /*!
         Wait up to a given time period. See wait() for more details.
         */
        template <class TimePoint>
        bool waitUntil(const TimePoint& tp) {
            lock_guard_t l(lock);
            if (pred()) {
                return true;
            }
            if (cv.wait_until(l, tp) == std::cv_status::timeout) {
                return false;
            }
            return pred();
        }

        /*!
         Obtain the lock and perform the given method, presumably something that may change
         the status of the predicate.
         Note that Fn must return true if the condition may be true and any pending
         threads should be notified.
         */
        void process(const std::function<bool()>& fn);

    private:
        using lock_guard_t = std::unique_lock<std::mutex>;
        predicate_t             pred;
        std::mutex              lock;
        std::condition_variable cv;
    };


    /*!
     A latch is a synchronizer that waits until a process calls the release method.
     */
    class Latch : public Condition {
    public:

        Latch() : Condition([this]{ return hasBeenFreed; }) {}
        Latch(Latch&&) = default;
        Latch& operator=(Latch&&) = default;

        /*!
         * Release the latch. This will cause the waiting threads to be notified
         * and released.
         */
        void release();

        /*!
         * Reset the latch. This will cause wait calls to wait once again.
         */
        void reset();

    private:
        bool hasBeenFreed = false;
    };


    /*!
     A barrier is a synchronizer that waits until a specified number (n) of
     threads call the wait method. At that point all the waits are released.
     @throws any exceptions that a condition variable may throw.
     */
    class Barrier {
    public:
        Barrier(unsigned n) : n(n) {}
        ~Barrier() = default;

        // Move is allowed, copy is not.
        Barrier(Barrier&&) = default;
        Barrier& operator=(Barrier&&) = default;
        Barrier(const Barrier&) = delete;
        Barrier& operator=(const Barrier&) = delete;

        /*!
         Wait until n threads have called the wait() method.
         */
        void wait();

        template <class Duration>
        bool waitFor(const Duration& dur) {
            lock_guard_t l(lock);
            ++counter;
            if (counter >= n) {
                cv.notify_all();
                return true;
            }
            if (cv.wait_for(l, dur) == std::cv_status::timeout) {
                --counter;
                return false;
            }
            return (counter >= n);
        }

        /*!
         Wait up to a given time period. See wait() for more details.
         */
        template <class TimePoint>
        bool waitUntil(const TimePoint& tp) {
            lock_guard_t l(lock);
            ++counter;
            if (counter >= n) {
                cv.notify_all();
                return true;
            }
            if (cv.wait_until(l, tp) == std::cv_status::timeout) {
                --counter;
                return false;
            }
            return (counter >= n);
        }

        /**
         * Reset the barrier. This will cause wait calls to wait once again.
         */
        void reset();

    private:
        using lock_guard_t = std::unique_lock<std::mutex>;
        std::mutex              lock;
        std::condition_variable cv;
        unsigned                counter = 0;
        const unsigned          n;
    };

}}

#endif
