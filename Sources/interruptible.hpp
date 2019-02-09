//
//  interruptible.hpp
//  kssthread
//
//  Created by Steven W. Klassen on 2014-11-22.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssthread_interruptible_hpp
#define kssthread_interruptible_hpp

#include <atomic>
#include <exception>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <thread>

#if defined(__linux)
#   include <cxxabi.h>
#endif


namespace kss { namespace thread {

    /*! \file interruptible.hpp

     Interruptible threads. This feature is lacking in the C++11 std::thread class,
     but since this library assumes POSIX we can also assume that the std::thread is
     in fact a POSIX thread. We make use of this in order to add an interruptible
     ability to the thread class based on the use of the pthread_cancel and associated
     methods.

     First, some terminology. The POSIX threads discuss cancellation. This simply stops
     the thread at some point. In the following discussion I use cancellation and
     interruption almost as synonyms. The difference being that "cancel" refers to the
     underlying C thread cancellation while "interruption" refers to the cancel being
     trapped and the interrupted exception being thrown.

     Next let's discuss how POSIX thread cancellation works. A thread can either be
     cancelable, in which case it will respond to the pthread_cancel command or it can
     be non-cancelable in which case the pthread_cancel command is (almost) ignored.
     (Actually the cancel request will be queued and if the thread is made cancellable
     later, it will be acted on and cancelled.)

     In addition threads can be cancelled almost immediately, or the cancel can be
     deferred to the next well-defined cancellation point. This library uses the deferred
     option as being safer than the immediate option. POSIX defines a number of methods
     as being cancellation points (see `man pthread`), essentially anything that causes
     a thread to pause (e.g. sleep, sem_wait, accept, etc.) is a cancellation point. Note
     that locks such as a mutex::lock() are not cancellation points as that could leave
     the mutex in an unknown state. You need to ensure that your locks are properly
     released to avoid deadlocked or stuck threads. If you have a long process that
     doesn't use any of the standard cancellation points, you can manually add a
     cancellation point by calling the interruptionPoint() method. (A mutex::lock()
     call qualifies as a "long" process and you should generally setup an interruption
     point after mutex::lock() returns.)

     To make all this work we define the following items:

     \class kss::thread::interruptible - a class that marks a block of code as being
        interruptible
     \fn kss::thread::interrupt(th) - will send the interruption (cancel) signal to
        the given thread
     \fn kss::thread::interruptionPoint - causes the current thread to check
        for cancellation
     \fn kss::thread::onInterrupted - a method that allows you to specify one
        or more handlers to be run when a thread is interrupted.

     Making a thread interruptible. In order to make a thread (or more accurately,
     a portion of the code within a thread) interruptible you wrap the code as a
     lambda passed to the interruptible object. This looks like the following:

     \code
     thread th { []{
        interruptible { []{
            ...lines of code...
        }};
     }};
     \endcode

     If you want to have a handler that runs when the thread is cancelled, add it
     within the interruptible section. This looks like the following:

     \code
     thread th { []{
         interruptible { []{
             onInterrupted([]{
                ... code to run when interrupted...
             });
             ...lines of code...
         }};
     }};
     \endcode

     Cancelling a thread. You can cancel any std::thread execution by calling the method
     kss::thread::interrupt(th) on that object. A word of warning: if you call interrupt
     on a thread that has not used the interruptible class, the result is undefined. What
     happens on OS/X is that the thread seems to get stopped but the interrupted exception
     is never thrown hence you don't get a nice exit point.

     Exceptions. The interruptible constructor may throw an std::system_error exception
     if the underlying pthread methods return an error code. In addition, any other
     exceptions that your code throws will also be thrown up the call chain. (Note that
     it is generally a bad idea to throw exceptions out of a thread, but that is really
     up to you. The interruptible class will simply pass on the exception.)

     Catching ... The thread interruption implementation makes use of an internal
     exception in order to properly unwrap and cleanup the stack. For Linux this is
     called "abi::__forced_unwind" and for others it is called
     "kss::thread::_private::Interrupted". If your function catches these exceptions
     (typically via `catch (...)`) the thread interruption facility will not work
     properly. For Linux users it will result in the message "FATAL: exception not
     rethrown" followed by the process aborting. For others it will likely result in
     the thread cancelling but the stack not being properly cleaned.
     This should not be a problem for most usage as "catch (...)" is generally a
     discouraged practice. Catching std::exception is fine as neither of these
     exceptions are descended from the standard one.  But if you must perform a
     "catch (...)", you will need to catch the above exceptions first and rethrow them.
     To make this easier we provide the macro KSS_THREAD_RETHROW_CANCELLED which
     expands to rethrow the exception apropriate for the current architecture. So if
     you must perform a "catch (...)" your code will need to look like the following:

     \code
     try {
        ...your application code...
     }
     KSS_THREAD_RETHROW_CANCELLED_EX
     catch (...) {
        ... normal error handling ...
     }
     \endcode
     */



    /*!
     This method will check the current thread to see if an interruption has
     been requested. If so the thread will be cancelled. Use this in long-running
     threads that do not already have a natural interruption point.

     @throws (sort of) an internal exception used to implement the thread cancellation.
        This method does not actually throw the exception itself, but will cause it
        to be thrown at this point if the thread has had a cancel request.
     */
    void interruptionPoint();

    /*!
     Interrupt the given thread. The version with no thread argument will send the
     cancel request to the current thread.
     @throws std::system_error if the pthread methods return an error code.
     */
    void interrupt(std::thread& th);
    void interrupt();


    /*!
     Class to wrap code that should be interruptible. Note that all the code will run in
     the constructor. This class will make the current thread interruptible, then will
     run the user's code, then will restore the thread's interruptible state to be its
     previous state.

     Note that we are not following our usual naming convensions (classes start with
     an uppercase letter), since we want this class to look and be used more like a
     keyword than a class. Specifically, it's intended use should look like the following:

     \code
     interruptible { [] {
        ...your code...
     }};
     \endcode

     @throws std::system_error if the pthread methods report an error code.
     */
    class interruptible {
    public:
        template <class Fn, class... Args>
        explicit interruptible(Fn&& fn, Args&&... args) {
            auto lambda = [fn = std::move(fn)](auto... params) {
                fn(params...);
            };
            doCall([&lambda, args...]{ lambda(args...); });
        }

    private:
        static void doCall(const std::function<void()>& fn);
    };

    namespace _private {
        class Interrupted {};
    }

    /*!
     In general the function you pass to interruptible should not perform a
     catch (...) as this will hinder the cancellation from working properly (see the
     description given above.) If you must do this sort of a catch, this macro will
     catch and rethrow the architecture dependant cancellation exception that is
     required.
     */
#if defined(__linux)
#   define KSS_THREAD_RETHROW_CANCELLED_EX catch (const abi::__forced_unwind&) { throw; }
#else
#   define KSS_THREAD_RETHROW_CANCELLED_EX catch (const kss::thread::_private::Interrupted&) { throw; }
#endif

    /*!
     Set a handler to be run if a thread is interrupted. Note that on some architectures
     (Mac) this is run just before the thread cleans up its local stack (i.e. before
     any destructors are run), but on others (Linux) it is run just after (i.e. after
     any destructors are run). Hence it is important that you do not reference anything
     local to the thread in this handler.

     Note that most of the time you shouldn't need this handler. You only need this
     if you need to distinguish between an interrupted and a non-interrupted exit
     of a thread.
     */
    void onInterrupted(const std::function<void()>& fn);

    
    /*!
     Call the interrupt() method on all the objects in the container. This has three
     forms:

     1. interrupt_all(container) accepts any container of interruptible objects.
     2. interrupt_all(begin, end) accepts forward iterators pointing to interruptible
        objects.
     3. interrupt_all(t1, t2, ... tn) accepts three or more references to interruptible
        objects.

     @throws any exception kss::thread::interrupt may throw.
     */
    template <typename ContainerOfInterruptibles>
    void interruptAll(ContainerOfInterruptibles& container) {
        for (auto& thrd : container) { interrupt(thrd); }
    }

    template <typename InputIteratorOfInterruptibles>
    void interruptAll(InputIteratorOfInterruptibles first, InputIteratorOfInterruptibles last) {
        for (auto it = first; it != last; ++it) { interrupt(*it); }
    }

    namespace _private {
        inline void doInterruptAll(std::thread& t1) {
            interrupt(t1);
        }
        inline void doInterruptAll(std::thread& t1, std::thread& t2) {
            interrupt(t1);
            interrupt(t2);
        }
        template <typename... Threads>
        void doInterruptAll(std::thread& t1, Threads& ... args) {
            interrupt(t1);
            doInterruptAll(args...);
        }
    }

    template <typename... Threads>
    inline void interruptAll(std::thread& t1, std::thread& t2, Threads& ... args) {
        _private::doInterruptAll(t1, t2);
        _private::doInterruptAll(args...);
    }

}}

#endif
