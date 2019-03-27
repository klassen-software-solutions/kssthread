//
//  join.hpp
//  kssthread
//
//  Created by Steven W. Klassen on 2014-11-22.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssthread_join_hpp
#define kssthread_join_hpp

#include <thread>

namespace kss { namespace thread {

    /*!
     Call the join() method on all the objects. This has three forms:
     1. joinAll(container) accepts any container of joinable objects.
     2. joinAll(begin, end) accepts forward iterators pointing to joinable objects.
     3. joinAll(t1, t2, ... tn) accepts three or more references to joinable objects.
     @throws any exception the objects' join method may throw
     */
    template <typename ContainerOfThreads>
    void joinAll(ContainerOfThreads& container) {
        for (auto& thrd : container) {
            if (thrd.joinable()) {
                thrd.join();
            }
        }
    }

    template <typename InputIteratorOfThread>
    void joinAll(InputIteratorOfThread first, InputIteratorOfThread last) {
        for (auto it = first; it != last; ++it) {
            if (it->joinable()) {
                it->join();
            }
        }
    }

    namespace _private {
        inline void doJoinAll(std::thread& t1) {
            if (t1.joinable()) {
                t1.join();
            }
        }
        inline void doJoinAll(std::thread& t1, std::thread& t2) {
            doJoinAll(t1);
            doJoinAll(t2);
        }

        template <typename... Threads>
        void doJoinAll(std::thread& t1, Threads& ... args) {
            if (t1.joinable()) {
                t1.join();
            }
            doJoinAll(args...);
        }
    }

    template <typename... Threads>
    inline void joinAll(std::thread& t1, std::thread& t2, Threads& ... args) {
        _private::doJoinAll(t1, t2);
        _private::doJoinAll(args...);
    }

}}

#endif
