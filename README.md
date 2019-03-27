# kssthread
C++ threading library

## What is it?

First of all, this is _not_ a replacement for std::thread. Instead, this library builds on top of the standard
threading library, adding some missing items as well as higher-level threading constructs.

Some of the features found in this library include the following:

* _ActionQueue_ allows actions (lambdas) to be queued for running on a single thread, possibly with a timer delay.
* _ActionThread_ allows a form of an `async` call, but cacheing a thread rather than starting a new one.
* _interruptible_ allows for a portion of an std::thread to be interruptible.
* _parallel_ provides a simple means of running a small number of operations in separate threads.
* There are also a few types of locks and synchronizers not currently available in the standard library.

[API Documentation](http://www.kss.cc/apis/kssthread/docs/index.html) 

## What has changed in V3?

Version 3 is a complete, non-back-compatible, rewrite of the earlier threading tools. This version drops
a number of tools that are now part of the standard library, brings the library up to C++14, and provides
consistent coding standards across the entire library. 

## Limitation on Ubuntu

While this code is regularly tested on macOS and Ubuntu, on Ubuntu it requires that you use the clang++
compiler instead of g++. The g++ compiler seg faults during the compilation. (At least version 7.3.0 does, 
newer versions may fix the problem.)

## Installing the Library

To build and install this library, run the following commands

```
./configure
make
make check
sudo make install
```

If you wish to make changes to this library that you believe will be useful to others, you can
contribute to the project. If you do, there are a number of policies you should follow:

* Check the issues to see if there are already similar bug reports or enhancements.
* Feel free to add bug reports and enhancements at any time.
* Don't work on things that are not part of an approved project.
* Don't create projects - they are only created the by owner.
* Projects are created based on conversations on the wiki.
* Feel free to initiate or join conversations on the wiki.
* Follow our [C++ Coding Standards](https://www.kss.cc/standards/c-.html).
