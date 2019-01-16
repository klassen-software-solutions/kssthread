//
//  action_queue.hpp
//  kssthread
//
//  Created by Steven W. Klassen on 2018-11-09.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//
// Although this version is created more recently (2018) it is largely based on earlier
// legacy versions of kssutil and kssthread. Note that it is intended to replace the
// somewhat broken dispatch class, which is now marked as deprecated.
//
//    kssutil thread.h (2002)
//    kssutil timer.h (2003)
//    kssthread dispatch.h (2014)
//

#ifndef kssthread_action_queue_hpp
#define kssthread_action_queue_hpp

#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include "utility.hpp"

namespace kss { namespace thread {

    /*!
     An action queue is used to allow a variety of actions to be queued on a single thread.
     Each action, added by one of the addAction() methods, will be run as soon as possible
     after its target delay. If the delay is the value asap (or any other duration that maps
     to zero) the action will be run as soon as possible.

     The key advantage of this over, for example, a thread looping with a wait_for() call,
     is that multiple timed things can be added, all shared with a single thread. In
     particular, if you have two or three threads that each loop and call wait_for(), you
     can replace them with a single instance of this class (and its single thread) and
     its helpers. 

     Another use of an action queue is to serialize accesses to some resource. That is,
     instead of accessing your resource directly, add actions that access it to the
     action queue. Since the action queue operates in a single thread, this ensures that
     the access will always be serialized without the need to managing locks. (Or more
     accurately, by relying on the ActionQueue internal locks.)
     */
    class ActionQueue {
    public:
        using action_t = std::function<void()>;

        /*!
         Use this in the constructor to specify that you do not want to enforce a limit
         on the number of items in the queue.
         */
        static constexpr size_t noLimit = std::numeric_limits<size_t>::max();

        /*!
         Use this in addAction() to specify that you want the action to be performed as
         soon as possible.
         */
        static const std::chrono::milliseconds asap;

        /*!
         Use this in cancel() to cancel all pending actions.
         */
        static constexpr const char* all = "";


        /*!
         Construct the queue.
         @param maxPending puts a maximum limit on the number of pending actions.
         */
        explicit ActionQueue(size_t maxPending = noLimit);

        ActionQueue(ActionQueue&&);
        ActionQueue& operator=(ActionQueue&&) noexcept;
        ~ActionQueue() noexcept;

        ActionQueue(const ActionQueue&) = delete;
        ActionQueue& operator=(const ActionQueue&) = delete;

        /*!
         Add an action to the queue. The action will be performed as soon as possible after
         the given delay.
         @param delay The amount of time before the action will be performed. Note that
            Duration must conform to the std::chrono::duration API.
         @param identifier If not empty this identifier can be used to cancel the action
            before it is performed. Note that it need not be unique. For example, you
            can give a set of actions the same identifier and then cancel them as a block.
         @param action The action to be performed.
         @throws std::invalid_argument if the delay is a negative value
         @throws std::system_error with a value of EAGAIN if the action queue
            already has maxPending items, or if we are currently waiting for
            pending actions to complete. In either case it is recommended that
            the caller wait a short time, then try again.
         @throws any exceptions that a condition_variable, a map, or a
            checked_duration_cast may throw.
         */
        template <class Duration>
        inline void addAction(const Duration& delay,
                              const std::string& identifier,
                              const action_t& action)
        {
            using kss::thread::_private::checked_duration_cast;
            addActionAfter(checked_duration_cast<std::chrono::milliseconds>(delay),
                           identifier, action);
        }

        template <class Duration>
        inline void addAction(const Duration& delay,
                              const std::string& identifier,
                              action_t&& action)
        {
            using kss::thread::_private::checked_duration_cast;
            addActionAfter(checked_duration_cast<std::chrono::milliseconds>(delay),
                           identifier, move(action));
        }

        template <class Duration>
        inline void addAction(const Duration& delay, const action_t& action) {
            using kss::thread::_private::checked_duration_cast;
            addActionAfter(checked_duration_cast<std::chrono::milliseconds>(delay),
                           "", action);
        }

        template <class Duration>
        inline void addAction(const Duration& delay, action_t&& action) {
            using kss::thread::_private::checked_duration_cast;
            addActionAfter(checked_duration_cast<std::chrono::milliseconds>(delay),
                           "", move(action));
        }

        /*!
         Add an action to the queue. The action will be performed as soon as possible.
         (I.e. It will be given a delay of 0.)
         @throws std::invalid_argument if the delay is a negative value
         @throws std::system_error with a value of EAGAIN if the action queue
            already has maxPending items, or if we are currently waiting for
            pending actions to complete. In either case it is recommended that
            the caller wait a short time, then try again.
         @throws any exceptions that a condition_variable or a map may throw.
         */
        inline void addAction(const action_t& action) {
            addActionAfter(asap, "", action);
        }

        inline void addAction(action_t&& action) {
            addActionAfter(asap, "", move(action));
        }

        /*!
         Cancel any actions that match the given identifier. If the identifier is all
         or anything else that is an empty string, then all pending actions, even those
         with identifiers, are cancelled.

         Note that actions that are in progress cannot be cancelled, only ones that are
         pending.

         @param identifier The id of the actions to be cancelled.
         @return the number of actions that were actually cancelled.
         */
        size_t cancel(const std::string& identifier = all);

        /*!
         Wait until all pending actions have completed. Note that no further actions
         may be added while waiting. But they may be added as soon as wait()
         has returned.
         @throws any exceptions that a condition_variable or a map may throw
         */
        void wait();

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;

        void addActionAfter(const std::chrono::milliseconds& delay,
                            const std::string& identifier,
                            const action_t& action);
        void addActionAfter(const std::chrono::milliseconds& delay,
                            const std::string& identifier,
                            action_t&& action);
    };


    /*!
     A repeating action is a helper class useful when you want to repeat the same action
     at regular intervals. You give it the desired interval, an ActionQueue, and the action,
     and it will repeatedly add the action with the given interval to the queue. It will
     continue to do this until it goes out of scope.

     Note that it is important that the ActionQueue remain in scope for at least as long
     as the RepeatingAction is in scope.
     */
    class RepeatingAction {
    public:
        template <class Duration>
        RepeatingAction(const Duration& interval,
                        ActionQueue& q,
                        const ActionQueue::action_t& act)
        : timeInterval(kss::thread::_private::checked_duration_cast<std::chrono::milliseconds>(interval)),
        queue(q), action(act)
        {
            init();
        }

        template <class Duration>
        RepeatingAction(const Duration& interval,
                        ActionQueue& q,
                        ActionQueue::action_t&& act)
        : timeInterval(kss::thread::_private::checked_duration_cast<std::chrono::milliseconds>(interval)),
        queue(q), action(move(act))
        {
            init();
        }

        ~RepeatingAction() noexcept;

    private:
        std::chrono::milliseconds   timeInterval;
        ActionQueue&                queue;
        std::string                 identifier;
        std::atomic<bool>           stopping { false };
        ActionQueue::action_t       action;
        ActionQueue::action_t       internalAction = [this] { runActionAndRequeue(); };

        void init();
        void runActionAndRequeue();
    };
}}

#endif
