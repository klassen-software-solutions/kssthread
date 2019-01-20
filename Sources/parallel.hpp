//
//  parallel.hpp
//  KSSCore
//
//  Created by Steven W. Klassen on 2003-09-22.
//  Copyright (c) Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

// This is a re-write of the earlier "Processor" class to make use of the C++11 async
// call. As part of this we have dropped the sequential portions of the class and have
// replaced the parallel portions with a single "parallel" function. This function runs
// all the actions it is given using async, then waits for all the futures to complete
// before returning.

// There is a second version of parallel() that takes a pre-defined cache of threads
// (called a ParallelThreadGroup in this API). This runs the workers in the existing
// threads rather than generating new ones each time. 

#ifndef kssthread_parallel_hpp
#define kssthread_parallel_hpp

#include <future>
#include <stdexcept>
#include <vector>

#include "action_thread.hpp"
#include "utility.hpp"

namespace kss {
    namespace thread {

        namespace _private {

            template <typename Action>
            void startActions(std::vector<std::future<void>>& futures, const Action& action) {
                futures.emplace_back(std::async(action));
            }

            template <typename Action, typename... Actions>
            void startActions(std::vector<std::future<void>>& futures,
                              const Action& action,
                              const Actions& ... actions)
            {
                startActions(futures, action);
                startActions(futures, actions...);
            }

            void waitForAll(std::vector<std::future<void>>& futures);

        }


        /*!
         This is a "helper" class used to provide the threads needed by the thread-cached
         version of the parallel() method.
         */
        class ParallelThreadGroup {
        public:
            /*!
             Create a thread group with the given number of threads.
             @throws any exception that thread creation or condition variables may throw
             */
            explicit ParallelThreadGroup(size_t numberOfThreads) : threads(numberOfThreads) {}

            /*!
             Returns the number of threads in this thread group.
             */
            inline size_t size() const noexcept { return threads.size(); }

            /*!
             Start a number of actions. It is imperative that the number of actions is no
             larger than the number of threads in the group, and that none of the threads
             are currently running. (I.e. After you call startActions() to start the
             current set of actions, you should not start the next set until waitForAll()
             has returned.) If you are calling this via the parallel() method, then that
             will all be done for you.
             */
            template <typename Action>
            void startActions(const Action& action) {
#               if !defined(NDEBUG)
                // preconditions. Note that we break from our standard here and only
                // check when in debug mode. The reason for this is that the reason
                // for creating this class is to allow a very low overhead version
                // of the parallel() method.
                if (futures.size() >= threads.size()) {
                    _KSSTHREAD_PRECONDITIONS_FAILED
                }
#               endif

                const auto nextThreadNo = futures.size();
                futures.emplace_back(threads[nextThreadNo].async(action));
            }

            template <typename Action, typename... Actions>
            inline void startActions(const Action& action, const Actions& ... actions) {
                startActions(action);
                startActions(actions...);
            }

            /*!
             Block the current thread until all the threads in this group have completed
             their current task. Note that after this you can call runOnThread() again
             with new tasks.
             @throws any exception that condition variables may throw
             */
            void waitForAll();

        private:
            std::vector<ActionThread>       threads;
            std::vector<std::future<void>>  futures;
        };


        /*!
         Run a number of actions, potentially in parallel, using the async method.
         Note that at least two actions are required. The advantage of this over the
         other parallel() method is that you don't have to create a ParallelThreadGroup
         class.

         This version of parallel() should be preferred if you are calling large actions
         (i.e. ones that run for awhile) once, or at most a few times such that the
         overhead of creating and destroying the threads is not significant.

         @throws any exceptions that the actions may throw
         @throws any exceptions that std::async may throw
         @throws any exceptions that std::future<void>::wait may throw
         */
        template <typename Action, typename... Actions>
        void parallel(const Action& action1, const Actions& ... actions) {
            std::vector<std::future<void>> futures;
            futures.reserve(sizeof...(Actions));

            _private::startActions(futures, actions...);
            action1();
            _private::waitForAll(futures);
        }

        /*!
         Run a number of actions, potentially in parallel, using a cache of threads.
         The key advantage of this over the other parallel() method, is that there is
         significantly less overhead compared to the async() method which generates
         a new thread with each call.

         This version of parallel() should be preferred if you are calling small actions
         many times so as to minimize the creation and destruction of the threads.

         Note that the ParallelThreadGroup must have at least (number of actions - 1)
         threads. (One of the actions will run in the current thread.)

         @throws std::invalid_argument if there are too many actions for the thread group
            (only checked in debug mode so be careful)
         @throws any exceptions that the actions may throw
         @throws any exceptions that std::async may throw
         @throws any exceptions that std::future<void>::wait may throw
         */
        template <typename Action, typename... Actions>
        void parallel(ParallelThreadGroup& tg,
                      const Action& action1,
                      const Actions& ... actions)
        {
#           if !defined(NDEBUG)
            // preconditions. Note that we break from our standard here and only
            // check when in debug mode. The reason for this is that the reason
            // for creating this method is to allow a very low overhead alternative
            // to the parallel() method given above.
            if (sizeof...(Actions) > tg.size()) {
                throw std::invalid_argument("The thread group must have at least "
                                            + std::to_string(sizeof...(Actions))
                                            + " threads");
            }
#           endif

            tg.startActions(actions...);
            action1();
            tg.waitForAll();
        }
    }
}

#endif
