//
//  utility.hpp
//  kssthread
//
//  Created by Steven W. Klassen on 2019-01-15.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#ifndef kssthread_utility_h
#define kssthread_utility_h

namespace kss { namespace thread {

    namespace _private {

        /*!
         A checked version of duration_cast. Note that this will be less efficient
         than std::chrono::duration_cast. In particular it is not a constexpr plus
         it needs to perform checks before the cast can be made.

         This is based on code found at
         https://stackoverflow.com/questions/44635941/how-to-check-for-overflow-in-duration-cast?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa

         "borrowed" from kssutil

         @throws std::overflow_error if the cast would not be representable in the target duration.
         */
        template <class ToDuration, class Rep, class Period>
        ToDuration checked_duration_cast(const std::chrono::duration<Rep, Period>& dtn) {
            using namespace std::chrono;
            using S = duration<long double, typename ToDuration::period>;
            constexpr S minimumAllowed = ToDuration::min();
            constexpr S maximumAllowed = ToDuration::max();
            const S s = dtn;
            if (s < minimumAllowed || s > maximumAllowed) {
                throw std::overflow_error("checked_duration_cast");
            }
            return duration_cast<ToDuration>(s);
        }


        /*!
         Returns a time_point constructed to contain the current time as defined by
         the time point's clock.
         @throws kss::util::time::checked_duration_cast if the duration value for
         TimePoint::clock::time_point::duration cannot be represented as a duration
         value for TimePoint::duration.

         "borrowed" from kssutil
         */
        template <class TimePoint>
        inline TimePoint now(const TimePoint& = TimePoint()) {
            using Clock = typename TimePoint::clock;
            using Duration = typename TimePoint::duration;
            const auto dur = Clock::now().time_since_epoch();
            return TimePoint(checked_duration_cast<Duration>(dur));
        }

    }
}}

#endif
