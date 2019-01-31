//
//  semaphore.hpp
//  kssthread
//
//  Created by Steven W. Klassen on 2014-12-12.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssthread_semaphore_hpp
#define kssthread_semaphore_hpp

#include <string>
#include <semaphore.h>

namespace kss { namespace thread {

    /*!
     The Semaphore class provides an STL-style wrapper around BSd/POSIX names
     semaphores.

     We don't go into a great discussion here on just what a semaphore is, other
     than to note that value is counted down and lock and try_lock will fail
     if more than value threads/processes have obtained this lock.

     This class conforms to the lockable C++11 concept, i.e. it provides lock,
     try_lock, and unlock methods. Note that try_lock() differs from our coding
     naming standards in order to conform to the STL style API.

     @throws system_error representing any failures of the underlying UNIX errno
        codes from the sem methods.
     */
    class Semaphore {
    public:

        /**
         * Construct the semaphore with the given initial value. This does not
         * obtain a lock.
         */
        Semaphore(const std::string& name, unsigned int value);
        ~Semaphore() noexcept;

        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

        Semaphore(Semaphore&&) = default;
        Semaphore& operator=(Semaphore&&) = default;

        // The locking methods match the STL style API found in std::mutex.
        void lock();
        bool try_lock();
        void unlock();

        /*!
         Obtain the name of the semaphore.
         */
        std::string name() const noexcept {
            return _name;
        }

        /*!
         Return the underlying native handle. This should only be used with great
         care and only if you really know what you are doing.
         */
        sem_t* nativeHandle() const noexcept {
            return _handle;
        }

    private:
        sem_t*      _handle;
        std::string _name;
    };

}}

#endif
