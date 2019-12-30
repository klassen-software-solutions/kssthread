//
//  action_queue.cpp
//  kssthread
//
//  Created by Steven W. Klassen on 2018-11-09.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <condition_variable>
#include <exception>
#include <iostream>
#include <map>
#include <mutex>
#include <system_error>
#include <thread>

#include <syslog.h>
#include <kss/contract/all.h>
#include <kss/util/all.h>

#include "action_queue.hpp"

using namespace std;
using namespace std::chrono;
using namespace kss::thread;
namespace contract = kss::contract;

using kss::util::time::now;

using time_point_t = time_point<steady_clock, milliseconds>;

namespace {
    struct ActionDetails {
        time_point_t            targetTime;
        string                  identifier;
        ActionQueue::action_t   action;
    };
}

using action_map_t = multimap<time_point_t, ActionDetails>;


// MARK: ActionQueue

struct ActionQueue::Impl {
    std::thread actionThread;
    size_t      maxPending = 0;
    bool        stopping = false;
    bool        waiting = false;
    bool        runningAction = false;

    // The following must be protected by the lock and the condition variable.
    mutex               lock;
    condition_variable  cv;
    action_map_t        pendingActions;

    inline time_point_t getNextTargetTime() noexcept {
        return (pendingActions.empty()
                ? now<time_point_t>() + 10000ms
                : pendingActions.begin()->first);
    }

    void runActionThread() {
        while (true) {
            bool haveAction = false;
            action_t currentAction;

            {
                unique_lock<mutex> l(lock);

                if (stopping) {
                    break;
                }

                const auto nextTargetTime = getNextTargetTime();
                cv.wait_until(l, nextTargetTime, [this] {
                    return stopping || !pendingActions.empty();
                });

                if (stopping) {
                    break;
                }

                if (!pendingActions.empty()) {
                    // If the next action is due to run, remove it from the queue and
                    // place it into currentAction.
                    const auto cit = pendingActions.begin();
                    const auto currentTime = now<time_point_t>();
                    if (cit->first <= currentTime) {
                        runningAction = true;
                        currentAction = move(cit->second.action);
                        pendingActions.erase(cit);
                        haveAction = true;
                    }
                }
            }

            if (haveAction) {
                runAction(currentAction);
            }
        }
    }

    void runAction(const action_t& action) {
        action();
        {
            lock_guard<mutex> l(lock);
            runningAction = false;
        }
        cv.notify_all();
    }

    void addActionDetails(ActionDetails&& details) {
        lock_guard<mutex> l(lock);
        if (!stopping) {
            if (waiting) {
                throw system_error(EAGAIN, system_category(), "addActionAfter (queue waiting)");
            }
            else {
                if (pendingActions.size() >= maxPending) {
                    throw system_error(EAGAIN, system_category(), "addActionAfter");
                }

                pendingActions.emplace(details.targetTime, move(details));
                cv.notify_all();

                contract::postconditions({
                    KSS_EXPR(!pendingActions.empty())
                });
            }
        }
    }
};


const milliseconds ActionQueue::asap { 0 };

ActionQueue::ActionQueue(ActionQueue&&) = default;
ActionQueue& ActionQueue::operator=(ActionQueue &&) noexcept = default;

ActionQueue::ActionQueue(size_t maxPending) : impl(new Impl()) {
    lock_guard<mutex> l(impl->lock);

    impl->maxPending = maxPending;
    impl->actionThread = std::thread { [this]{ impl->runActionThread(); }};

    contract::postconditions({
        KSS_EXPR(impl->maxPending == maxPending),
        KSS_EXPR(impl->stopping == false),
        KSS_EXPR(impl->waiting == false),
        KSS_EXPR(impl->runningAction == false),
        KSS_EXPR(impl->pendingActions.empty())
    });
}

ActionQueue::~ActionQueue() noexcept {
    try {
        {
            lock_guard<mutex> l(impl->lock);
            impl->stopping = true;
        }
        impl->cv.notify_all();
        cancel();
        if (impl->actionThread.joinable()) {
            impl->actionThread.join();
        }
    }
    catch (const exception& e) {
        // Best we can do is log the error and continue.
        syslog(LOG_ERR, "[%s] Exception shutting down: %s", __PRETTY_FUNCTION__, e.what());
    }
}


size_t ActionQueue::cancel(const string& identifier) {
    size_t ret = 0;
    {
        lock_guard<mutex> l(impl->lock);
        const auto sizeIn = impl->pendingActions.size();
        if (!impl->pendingActions.empty()) {
            if (identifier.empty()) {
                ret = sizeIn;
                impl->pendingActions.clear();
            }
            else {
                const auto end = impl->pendingActions.end();
                for (auto it = impl->pendingActions.begin(); it != end;) {
                    if (it->second.identifier == identifier) {
                        it = impl->pendingActions.erase(it);
                        ++ret;
                    }
                    else {
                        ++it;
                    }
                }
            }
        }

        contract::postconditions({
            KSS_EXPR(impl->pendingActions.size() == (sizeIn - ret))
        });
    }

    if (ret > 0) {
        impl->cv.notify_all();
    }
    return ret;
}


void ActionQueue::wait() {
    unique_lock<mutex> l(impl->lock);
    if (!impl->pendingActions.empty() || impl->runningAction) {
        impl->waiting = true;
        auto self = impl.get();
        impl->cv.wait(l, [self] {
            return self->stopping || (self->pendingActions.empty() && !self->runningAction);
        });
        impl->waiting = false;
    }

    contract::postconditions({
        KSS_EXPR(impl->waiting == false),
        KSS_EXPR(impl->runningAction == false),
        KSS_EXPR(impl->pendingActions.empty())
    });
}


void ActionQueue::addActionAfter(const milliseconds &delay,
                                 const string& identifier,
                                 const action_t &action)
{
    contract::parameters({
        KSS_EXPR(delay.count() >= 0)
    });

    ActionDetails details { now<time_point_t>() + delay, identifier, action };
    impl->addActionDetails(move(details));
}

void ActionQueue::addActionAfter(const milliseconds &delay,
                                 const string& identifier,
                                 action_t &&action)
{
    contract::parameters({
        KSS_EXPR(delay.count() >= 0)
    });

    ActionDetails details { now<time_point_t>() + delay, identifier, move(action) };
    impl->addActionDetails(move(details));
}


// MARK: RepeatingAction

RepeatingAction::~RepeatingAction() noexcept {
    stopping = true;
    queue.cancel(identifier);
}

void RepeatingAction::init() {
    identifier = string(kss::util::UUID::generate());
    queue.addAction(timeInterval, identifier, internalAction);

    contract::postconditions({
        KSS_EXPR(!identifier.empty()),
        KSS_EXPR(stopping == false)
    });
}

void RepeatingAction::runActionAndRequeue() {
    if (!stopping) {
        action();
        try {
            queue.addAction(timeInterval, identifier, internalAction);
        }
        catch (const std::system_error& err) {
            // If the queue is currently paused or full, we pause, then try again.
            if (err.code() == error_code(EAGAIN, system_category())) {
                this_thread::sleep_for(timeInterval);
                internalAction();
            }
            else {
                throw;
            }
        }
    }
}
