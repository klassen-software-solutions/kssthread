//
//  utility.cpp
//  kssthread
//
//  Created by Steven W. Klassen on 2019-01-15.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#include "utility.hpp"

using namespace std;
using namespace kss::thread;


chrono::milliseconds kss::thread::_private::timeOfExecution(const function<void ()>& fn) {
    using namespace std::chrono;

    const auto start = high_resolution_clock::now();
    fn();
    return duration_cast<milliseconds>(high_resolution_clock::now() - start);
}
