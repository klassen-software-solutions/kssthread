//
//  read_write_lock.cpp
//  kssthread
//
//  Created by Steven W. Klassen on 2014-12-12.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <system_error>

#include <syslog.h>

#include "_contract.hpp"
#include "read_write_lock.hpp"

using namespace std;
using namespace kss::thread;

namespace contract = kss::thread::_private::contract;


namespace {
    // Log an error using syslog. This is used to track cases where the underlying C call
    // fails but our API does not allow us to throw an exception.
    void log(const char* methodname, int err) noexcept {
        syslog(LOG_ERR,
               "[kssthread/read_write_lock.cpp] %s returned error %d (%s)",
               methodname, err, strerror(err));
    }
}


void _private::RWLockCommon::unlock() {
    contract::preconditions({
        KSS_EXPR(_handle != nullptr)
    });

    int err = pthread_rwlock_unlock(_handle);
    if (err != 0) {
        throw system_error(err, system_category(), "pthread_rwlock_unlock");
    }
}


void ReadWriteLock::ReadLock::lock() {
    contract::preconditions({
        KSS_EXPR(_handle != nullptr)
    });

    int err = pthread_rwlock_rdlock(_handle);
    if (err != 0) {
        throw system_error(err, system_category(), "pthread_rwlock_rdlock");
    }
}


bool ReadWriteLock::ReadLock::try_lock() {
    contract::preconditions({
        KSS_EXPR(_handle != nullptr)
    });

    int err = pthread_rwlock_tryrdlock(_handle);
    if (err == EBUSY) { return false; }    // not an error, just not available right now
    if (err != 0) {
        throw system_error(err, system_category(), "pthread_rwlock_tryrdlock");
    }
    return true;
}


void ReadWriteLock::WriteLock::lock() {
    contract::preconditions({
        KSS_EXPR(_handle != nullptr)
    });

    int err = pthread_rwlock_wrlock(_handle);
    if (err != 0) {
        throw system_error(err, system_category(), "pthread_rwlock_wrlock");
    }
}


bool ReadWriteLock::WriteLock::try_lock() {
    contract::preconditions({
        KSS_EXPR(_handle != nullptr)
    });

    int err = pthread_rwlock_trywrlock(_handle);
    if (err == EBUSY) { return false; }    // not an error, just not available right now
    if (err != 0) {
        throw system_error(err, system_category(), "pthread_rwlock_trywrlock");
    }
    return true;
}


ReadWriteLock::ReadWriteLock() : _readLock(&_handle), _writeLock(&_handle) {
    int err = pthread_rwlock_init(&_handle, nullptr);
    if (err != 0) {
        throw system_error(err, system_category(), "pthread_rwlock_init");
    }
}


ReadWriteLock::~ReadWriteLock() noexcept {
    int err = pthread_rwlock_destroy(&_handle);
    if (err != 0) {
        // We don't want to throw an exception, so logging an error is the best we can do.
        log("pthread_rwlock_destroy", err);
    }
}
