//
//  version.hpp
//  kssthread
//
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssthread_version_hpp
#define kssthread_version_hpp

#include <string>

namespace kss { namespace thread {
    
    /*!
     Returns a string of the form x.y.z<optional tags> that describes the version
     of this library.
     */
    std::string version() noexcept;
    
    /*!
     Returns the text of the software license.
     */
    std::string license() noexcept;
    
}}

#endif
