//
//  parallel.cpp
//  KSSCore
//
//  Created by Steven W. Klassen on 2003-09-22.
//  Copyright (c) Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
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
