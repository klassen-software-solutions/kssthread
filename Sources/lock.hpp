//
//  lock.hpp
//  kssthread
//
//  Created by Steven W. Klassen on 2014-12-12.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssthread_lock_hpp
#define kssthread_lock_hpp

#include <mutex>

namespace kss { namespace thread {

    /*!
     The locked method allows for a short-hand version of a lock_guard. Specifically
     you pass in a lambda that you wish to execute while protected by the given lock
     item, and it is automatically protected by a lock_guard<Lockable>. This is useful
     if you need to lock a section of code that is only a portion of the current
     block.

     @code
     mutex m;
     locked(m, []{ ...code goes here... });
     @endcode
     */
    template <class Lockable, class Fn, class... Args>
    void locked(Lockable& l, Fn&& fn, Args&&... args) {
        std::lock_guard<Lockable> lock(l);
        fn(std::forward<Args>(args)...);
    }

    /*!
     The uniqueLocked method is similar to locked() but uses a unique_lock guard.
     */
    template <class Lockable, class Fn, class... Args>
    void uniqueLocked(Lockable& l, Fn&& fn, Args&&... args) {
        std::unique_lock<Lockable> lock(l);
        fn(std::forward<Args>(args)...);
    }


    /*!
     The TryLockGuard attempts to obtain the lock in its constructor and,
     if necessary, frees it in its destructor. Use the haveLock() method to
     determine if the lock was indeed obtained.

     This should work with any lockable type, i.e. any type that supports try_lock
     and unlock.
     */
    template <typename Lockable>
    class TryLockGuard {
    public:
        /*!
         Construct the guard and attempt the lock.
         @throws any exception the underlying locking mechanism may throw
         */
        explicit TryLockGuard(Lockable& l) : _lock(l) {
            _haveLock = _lock.try_lock();
        }
        ~TryLockGuard() {
            if (_haveLock) {
                _lock.unlock();
            }
        }

        /*!
         Returns true if the lock was obtained and false otherwise.
         */
        bool haveLock() const noexcept {
            return _haveLock;
        }

    private:
        Lockable&   _lock;
        bool        _haveLock = false;
    };

    /*!
     The ifLocked method is similar to locked() and uniqueLocked(), but uses a TryLockGuard.
     The key significance is that the lambda you provide will be run only if the lock is
     obtained.
     */
    template <class Lockable, class Fn, class... Args>
    void ifLocked(Lockable& l, Fn&& fn, Args&&... args) {
        TryLockGuard<Lockable> lock(l);
        if (lock.haveLock()) {
            fn(std::forward<Args>(args)...);
        }
    }

}}

#endif
