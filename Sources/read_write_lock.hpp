//
//  read_write_lock.hpp
//  kssthread
//
//  Created by Steven W. Klassen on 2014-12-12.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssthread_read_write_lock_hpp
#define kssthread_read_write_lock_hpp

#include <memory>
#include <pthread.h>

namespace kss { namespace thread {

    namespace _private {

        class RWLockCommon  {
        public:
            explicit RWLockCommon(pthread_rwlock_t* handle) : _handle(handle) {}
            void unlock();
        protected:
            pthread_rwlock_t* _handle;
        };

    }

    /*!
     A read/write lock allows any number of threads to obtain the read lock at a time
     but only one to obtain the write lock. In addition the write lock is favoured
     in that if there are any threads waiting for the write lock, requests for read
     locks will be blocked.

     Note that this lock itself does not constitute a locking interface. Instead it is
     just a container allowing you to obtain the read lock or the write lock and make
     lock requests on them.

     @throws std::system_error if the underlying pthread methods return an error code
     */
    class ReadWriteLock {
    public:
        class ReadLock : public _private::RWLockCommon {
        public:
            explicit ReadLock(pthread_rwlock_t* handle) : _private::RWLockCommon(handle) {}
            void lock();
            bool try_lock();
        };

        class WriteLock : public _private::RWLockCommon {
        public:
            explicit WriteLock(pthread_rwlock_t* handle) : _private::RWLockCommon(handle) {}
            void lock();
            bool try_lock();
        };


        /*!
         Construct a read/write lock.
         */
        ReadWriteLock();
        ~ReadWriteLock() noexcept;

        ReadWriteLock(const ReadWriteLock&) = delete;
        ReadWriteLock& operator=(const ReadWriteLock&) = delete;

        ReadWriteLock(ReadWriteLock&&) = delete;
        ReadWriteLock& operator=(ReadWriteLock&&) = delete;

        /*!
         Returns a reference to the read lock.
         */
        ReadLock& readLock() noexcept {
            return _readLock;
        }

        /*!
         Returns a reference to the write lock.
         */
        WriteLock& writeLock() noexcept {
            return _writeLock;
        }

        /*!
         Return the underlying native handle. This should only be used with great
         care and only if you really know what you are doing.
         */
        pthread_rwlock_t nativeHandle() const noexcept {
            return _handle;
        }

    private:
        pthread_rwlock_t    _handle;
        ReadLock            _readLock;
        WriteLock           _writeLock;
    };

}}

#endif
