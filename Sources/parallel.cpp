//
//  parallel.cpp
//  KSSCore
//
//  Created by Steven W. Klassen on 2003-09-22.
//  Copyright (c) Klassen Software Solutions. All rights reserved.
//
//  Permission is hereby granted to use/modify/publish this code without restriction or
//  requirement other than you are not allowed to hinder others from doing the same.
//  No warranty or other guarantee is given regarding the use of this code.
//

#include "parallel.hpp"

using namespace std;
using namespace kss::thread;


void kss::thread::_private::waitForAll(vector<future<void>> &futures) {
    for (auto& fut : futures) {
        fut.wait();
    }
}


// MARK: ParallelThreadGroup

void ParallelThreadGroup::waitForAll() {
    if (!futures.empty()) {
        _private::waitForAll(futures);
        futures.clear();
    }
}
