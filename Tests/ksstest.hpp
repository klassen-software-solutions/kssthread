//
//  ksstest.hpp
//  ksstest
//
//  Created by Steven W. Klassen on 2018-04-03.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef ksstest_ksstest_hpp
#define ksstest_ksstest_hpp

#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

namespace kss {
	namespace test {

		// MARK: Running

		/*!
		 Run the currently registered tests and report their results. Pass the command
		 line argument "-h" or "--help" for a description of its use.

		 @throws std::invalid_argument if any of the arguments are missing or wrong.
		 @throws std::system_error or std::runtime_exception if something goes wrong with the run itself.
		 */
		int run(const std::string& testRunName, int argc, const char* const* argv);

		/*!
		 Call this within a test if you want to skip it. Typically this would be the first
		 line of your test, but it does not have to be. The test will be aborted at the
		 point this is called, and the test will be marked as skipped regardless of whether
		 it had passed or failed up to that point.

		 If you place this in the beforeEach method, then all the test cases for that test
		 suite will be skipped. If you place it in the afterEach method, then the test cases
		 for that suite will run, but will still be marked as skipped.

		 Note that the skip is implemented by throwing an exception and must not be caught
		 or the skip will not be performed. This exception is NOT subclassed from std::exception,
		 so as long as your code does not catch (...) it will be fine.
		 */
		void skip();

        /*!
         After run() has begun this will return true if we are running in quiet mode
         and false otherwise. You can use this in your test code if you want to suppress
         output that would otherwise be shown.

         If called before run(), this will return false.
         */
        bool isQuiet() noexcept;

        /*!
         After run() has begun this will return true if we are running in verbose mode
         and false otherwise. You can use this in your test code if you want to add
         output that would otherwise not be shown.

         If called before run(), this will return false.
         */
        bool isVerbose() noexcept;
        

		// MARK: Assertions

		/*!
		 Macro to perform a single test. This works like assert but instead of halting
		 execution it updates the internal status of the test suit.
		 */
#		define KSS_ASSERT(expr) ((void) ((expr) ? kss::test::_private::_success() : kss::test::_private::_failure(#expr, __FILE__, __LINE__)))

		// The following are intended to be used inside KSS_ASSERT in order to make the
		// test intent clearer. They are especially useful if your test contains multiple
		// lines of code. For single line tests you may want to just use the assertion
		// on its own. For example,
		//
		//  KSS_ASSERT(isEqualTo(3, []{ return i; }));
		//
		// isn't really any clearer than
		//
		//  KSS_ASSERT(i == 3);
		//
		// However,
		//
		//  KSS_ASSERT(isEqualTo(3, []{
		//     auto val = obtainAValueFromSomewhere();
		//     doSomeWorkOnTheValue(&val);
		//     return val;
		//  }));
		//
		// is much clearer than trying to do it all in the KSS_ASSERT expr argument.

		/*!
		 Returns true if the lambda returns true.
		 example:
		   KSS_ASSERT(isTrue([]{ return true; }));
		 */
		inline bool isTrue(const std::function<bool()>& fn) { return fn(); }

		/*!
		 Returns true if the lambda returns false.
		 example:
		   KSS_ASSERT(isFalse([]{ return false; }));
		 */
		inline bool isFalse(const std::function<bool()>& fn) { return !fn(); }

		/*!
		 Returns true if the lambda returns a value that is equal to a.
		 example:
		   KSS_ASSERT(isEqualTo(23, []{ return 21+2; }));
		 */
		template <class T>
		bool isEqualTo(const T& a, const std::function<T()>& fn) {
			return (fn() == a);
		}

		/*!
		 Returns true if the lambda returns a value that is not equal to a.
		 example:
		   KSS_ASSERT(isNotEqualTo(23, []{ return 21-2; }));
		 */
		template <class T>
		bool isNotEqualTo(const T& a, const std::function<T()>& fn) {
			return (!(fn() == a));
		}

		/*!
		 Returns true if the lambda throws an exception of the given type. Note that if
         it throws an exception not descended from std::exception, that exception will
         be passed up the stack.
         
		 example:
		   KSS_ASSERT(throwsException<std::runtime_error>([]{
		       throw std::runtime_error("hello");
		   }));
		 */
		template <class Exception>
		bool throwsException(const std::function<void()>& fn) {
			bool caughtCorrectException = false;
			try {
				fn();
			}
			catch (const Exception&) {
				caughtCorrectException = true;
			}
			catch (const std::exception&) {
			}
			return caughtCorrectException;
		}

        /*!
         Returns true if the lambda throws a system_error with the given error category.
         Note that if it throws an exception not descended from std::exception, that
         exception will be passed up the stack.

         example:
           KSS_ASSERT(throwsSystemErrorWithCategory(std::system_category(), []{
               throw std::system_error(EAGAIN, std::system_category(), "hi");
           }));
         */
        bool throwsSystemErrorWithCategory(const std::error_category& cat,
                                           const std::function<void()>& fn);

        /*!
         Returns true if the lambda throws a system_error with the given error code. Note
         that the code includes both the integer error code plus the category.
         Note that if it throws an exception not descended from std::exception, that
         exception will be passed up the stack.

         example:
           KSS_ASSERT(throwsSystemErrorWithCode(std::error_code(EAGAIN, std::system_category()), []{
               throw std::system_error(EAGAIN, std::system_category(), "hi");
           }));
         */
        bool throwsSystemErrorWithCode(const std::error_code& code,
                                       const std::function<void()>& fn);

		/*!
		 Returns true if the lambda does not throw any exception.
         Note that if it throws an exception not descended from std::exception, that
         exception will be passed up the stack.

         example:
		   KSS_ASSERT(doesNotThrowException([]{ doSomeWork(); }));
		 */
		bool doesNotThrowException(const std::function<void()>& fn);

		/*!
		 Returns true if the lambda causes terminate to be called.
		 example:
		   inline void cannot_throw() noexcept { throw std::runtime_error("hi"); }
		   KSS_ASSERT(terminates([]{ cannot_throw() }));
		 */
		bool terminates(const std::function<void()>& fn);


		// MARK: TestSuite

		/*!
		 Use this class to define the test suites to be run. Most of the time you will
		 likely just use this class "as is" providing the necessary tests in the constructor.
		 But you can also subclass it if you need to modify its default behaviour.
		 */
		class TestSuite {
		public:
			using test_case_fn = std::function<void(TestSuite&)>;
			using test_case_list = std::initializer_list<std::pair<std::string, test_case_fn>>;

			/*!
			 Construct a test suite. All the test cases are lambdas that conform to test_case_fn.
			 They are all included in the constructor and a static version of each TestSuite
			 must be declared. (It is the static declaration that will register the test
			 suite with the internal infrastructure.)

			 Note that the lambdas are not run in the construction. They will be run at
			 the appropriate time when kss::testing::run is called.
			 */
			TestSuite(const std::string& testSuiteName, test_case_list fns);
			virtual ~TestSuite() noexcept;

			TestSuite(TestSuite&&);
			TestSuite& operator=(TestSuite&&);

			TestSuite(const TestSuite&) = delete;
			TestSuite& operator=(const TestSuite&) = delete;

			/*!
			 Accessors
			 */
			const std::string& name() const noexcept;

		private:
			friend int run(const std::string& testRunName, int argc, const char* const* argv);

			struct Impl;
			std::unique_ptr<Impl> _impl;

		public:
			// Never call this! It must be public to allow access in some of the
			// implementation internals.
			const Impl* _implementation() const noexcept { return _impl.get(); }
			Impl* _implementation() noexcept { return _impl.get(); }
		};


		// MARK: TestSuite Modifiers

		// Most of the time you will likely use TestSuite "as is" just providing the
		// tests in the constructor. However, you can modify its default behaviour by
		// creating a subclass that inherits from one or more of the following interfaces.
		// Note that after defining the subclass you still must create a static instance
		// of the class in order to register its tests to be run.

		/*!
		 Extend your TestSuite with these interfaces if you wish it to call code before
		 (or after) all the tests in your TestSuite. It is acceptable to perform tests
		 in these methods - they will be treated as a test case called "BeforeAll" and
		 "AfterAll", respectively.
		 */
		class HasBeforeAll {
		public:
			virtual void beforeAll() = 0;
		};

		class HasAfterAll {
		public:
			virtual void afterAll() = 0;
		};

		/*!
		 Extend your TestSuite with these interfaces if you wish it to call code before
		 (or after) each of the tests in your TestSuite. It is acceptable to perform
		 tests in these methods - they will be treated as part of each test case.
		 */
		class HasBeforeEach {
		public:
			virtual void beforeEach() = 0;
		};

		class HasAfterEach {
		public:
			virtual void afterEach() = 0;
		};

		/*!
		 Extend your TestSuite with this interface if you wish it to always be run on
		 the main thread. Note that it still may run in parallel with other TestSuites
		 that do not use this interface. But any that inherit from this interface will
		 not run in parallel with each other.

		 (There are no methods in this interface. Simply having your class inherit from
		 it will be enough.)
		 */
		class MustNotBeParallel {
		};

		namespace _private {
			// This namespace defines items that need to be publically available for syntactical
			// reasons but should be treated as private and never called manually.
			void _success(void) noexcept;
			void _failure(const char* expr, const char* filename, unsigned int line) noexcept;
		}
	}
}

#endif /* ksstest_hpp */
