//
//  signal.hpp
//  kssthread
//
//  Created by Steven W. Klassen on 2014-11-22.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssthread_signal_hpp
#define kssthread_signal_hpp

#include <initializer_list>
#include <thread>

namespace kss { namespace thread {

    /*!
     Threads and signals. We provide a simple mechanism to send and ignore POSIX
     signals within POSIX threads.

     Note that the ignore methods will ignore the signals for the remaining life of
     the thread. In Linux there is a sigtimedwait method that would allow me to
     ignore the signals for a block of code, however it does not exist on the mac
     and so far I have not been able to figure out how to flush a pending signal
     within a thread. The moment I remove the mask, it sends the signal.
     */
    namespace signal {

        /*!
         Send a signal to a thread.
         @throws std::system_error if the underlying C calls return an error
         */
        void send(std::thread& th, int sig);

        /*!
         Ignore a signal, or a set of signals, within the current thread. Note that
         this will remain in effect for the remaining life of the thread.
         @throws std::system_error if the underkying C calls return an error
         */
        void ignore(std::initializer_list<int> signals);
        inline void ignore(int sig) { ignore({ sig }); }

    }
}}

#endif
