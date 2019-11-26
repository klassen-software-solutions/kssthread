//
//  semaphore.cpp
//  kssthread
//
//  Created by Steven W. Klassen on 2014-12-12.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cerrno>
#include <cstring>
#include <system_error>

#include <fcntl.h>
#include <semaphore.h>
#include <syslog.h>
#include <sys/stat.h>
#include <kss/contract/all.h>

#include "semaphore.hpp"

using namespace std;
using namespace kss::thread;

namespace contract = kss::contract;


namespace {
    // Log an error using syslog. This is used to track cases where the underlying C call
    // fails but our API does not allow us to throw an exception.
    void log(const char* methodname, int err) noexcept {
        syslog(LOG_ERR,
               "[kssthread/semaphore.cpp] %s returned error %d (%s)",
               methodname, err, strerror(err));
    }
}


Semaphore::Semaphore(const string& name, unsigned int value) : _name(name) {
    sem_t* sem = sem_open(name.c_str(), O_CREAT, (S_IRWXU | S_IRWXG | S_IRWXO), value);
    if (sem == SEM_FAILED) {
        throw system_error(errno, system_category(), "sem_open");
    }
    _handle = sem;

    contract::postconditions({
        KSS_EXPR(_handle != nullptr && _handle != SEM_FAILED)
    });
}

Semaphore::~Semaphore() noexcept {
    if (_handle != SEM_FAILED) {
        // Don't want to throw an exception. The best we can do is log the problem.
        if (sem_close(_handle) == -1) {
            log("sem_close", errno);
        }
        if (sem_unlink(_name.c_str()) == -1) {
            log("sem_unlink", errno);
        }
    }
}

void Semaphore::lock() {
    contract::preconditions({
        KSS_EXPR(_handle != nullptr && _handle != SEM_FAILED)
    });

    if (sem_wait(_handle) == -1) {
        throw system_error(errno, system_category(), "sem_wait");
    }
}

bool Semaphore::try_lock() {
    contract::preconditions({
        KSS_EXPR(_handle != nullptr && _handle != SEM_FAILED)
    });

    if (sem_trywait(_handle) == 0) {
        return true;
    }
    else {
        if (errno == EAGAIN) {
            return false;
        }
        else {
            throw system_error(errno, system_category(), "sem_trywait");
        }
    }
}

void Semaphore::unlock() {
    contract::preconditions({
        KSS_EXPR(_handle != nullptr && _handle != SEM_FAILED)
    });

    if (sem_post(_handle) == -1) {
        throw system_error(errno, system_category(), "sem_post");
    }
}
