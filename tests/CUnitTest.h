/*
 *  CUnitTest.h
 *  CUnitTest (CUT)
 *  A simple but fairly complete unit test framework for C in a
 *  single header file. Features fixtures, test suites and integration
 *  with xcode!
 *
 * CUnitTest is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Created by Jonathon Hare on 26/11/2006.
 *  Copyright (C) 2006-2007 Jonathon Hare, University of Southampton.
 *  All rights reserved.
 *
 */

/*!
    @header CUnitTest
    @abstract   A unit test framework for C
    @discussion A simple but fairly complete unit test framework for C in a
    single header file. Features fixtures, test suites and integration with 
    xcode!
    
    To use: Write some test cases in implementation (.c) files. The test cases 
    must take no parameters and return an int. If the test passes, it should 
    return 0. Include this header file in each of the test case implementation 
    files. The test cases should use the various <code>CUT_ASSERT_*</code> 
    macros to test for failure. In order to run the tests, a test harness must 
    be created using the <code>CUT_BEGIN_TEST_HARNESS</code> and 
    <code>CUT_END_TEST_HARNESS</code> macros. These macros will generate a 
    <code>main</code> method, so should only be called once. Test cases can be 
    added to the test harness using the <code>CUT_RUN_TEST()</code> macro with 
    the name of the test case function as the argument. For example, the 
    following code creates a simple test that will always fail, together with a
    test harness to run it.
 
    <pre>
    @textblock
    #include "CUnitTest.h"
 
    int test1(void) {
        CUT_ASSERT(1==0);
        return 0;    
    }
 
    CUT_BEGIN_TEST_HARNESS
    CUT_RUN_TEST(test1);
    CUT_END_TEST_HARNESS
    @/textblock
    </pre>
    It is also possible to create suites of test cases using the 
    <code>CUT_BEGIN_SUITE</code>/<code>CUT_BEGIN_SUITE_F</code> and 
    <code>CUT_END_SUITE</code>/<code>CUT_END_SUITE_F</code> macros. The 
    <code>_F</code> versions of the macros enable fixtures to be defined which 
    will set up global variables before each test and tear the variables down 
    after each test. The fixture setup and teardown functions must have void 
    arguments and return types. Test suites must be given a name which follows 
    standard C function naming conventions (i.e. no spaces, semi-colons, etc). 
    Whole test suits can be run by adding them to the test harness with the 
    <code>CUT_RUN_SUITE_TESTS()</code> macro with the name of the suite. 
    The following example demonstrates the use of two test suites with a single
    function, one of which uses a fixture, together with a single stand-alone 
    test case.
 
    <pre>
    @textblock
    #include "CUnitTest.h"
    #include <stdlib.h>
 
    int *val = NULL;
    void setup(void) {
     	val = malloc(sizeof(int));
    	*val=255;
    }
 
    void teardown(void) {
    	free(val);
	val = NULL;
    }
 
    int test1(void) {
	CUT_ASSERT(1==1);
	return 0;    
    }
 
    int test2(void) {
	CUT_ASSERT_NULL(val);
	return 0;    
    }
 
    int test3(void) {
	CUT_ASSERT_NOT_NULL(val);
        CUT_ASSERT(*val == 255);
	return 0;
    }

    CUT_BEGIN_SUITE(suite1)
    CUT_RUN_TEST(test2);
    CUT_END_SUITE
 
    CUT_BEGIN_SUITE_F(suite2, &setup, &teardown)
    CUT_RUN_TEST(test3);
    CUT_END_SUITE_F
 
    CUT_BEGIN_TEST_HARNESS
    CUT_RUN_TEST(test1);
    CUT_RUN_SUITE_TESTS(suite1);
    CUT_RUN_SUITE_TESTS(suite2);
    CUT_END_TEST_HARNESS
    @/textblock
    </pre>
    Running the above test code will yield:
    <pre>
    @textblock
    suite1: tests-run: 1, assertions: 1, passes: 1, failures: 0
    suite2: tests-run: 1, assertions: 2, passes: 1, failures: 0
    Overall Results: tests-run: 3, assertions: 4, passes: 3, failures: 0
    @/textblock
    </pre>
    XCode integration: In order to integrate with xcode, create a "Shell Tool" 
    target and add the test files. Add a new shell script build phase to the end
    of the build phases that contains the following script (with the 
    parentheses): 
    <pre>"$BUILT_PRODUCTS_DIR/$PRODUCT_NAME"</pre>
    Building the test target will automatically run the test executable and any 
    test-case failures will be displayed in the build pane. These failures can
    be double-clicked to take you to the corresponding line of the source code.
 
    @copyright 2007 University of Southampton. All rights reserved.
    @updated 2007-03-20
*/
#ifndef __CUNITTEST_H__
#define __CUNITTEST_H__

#include <stdio.h>

extern int CUT_tests_run;
extern int CUT_tests_failed;
extern int CUT_suite_tests_run;
extern int CUT_suite_tests_failed;
extern int CUT_assertions_count;
extern const char *CUT_curr_suite;
void (*CUT_fixture_setup) (void);
void (*CUT_fixture_teardown) (void);

#if 0
#if __STDC_VERSION__ < 199901L
#if __GNUC__ >= 2
#define __func__ __FUNCTION__
#else
#define __func__ "<unknown>"
#endif
#endif
#endif

#define __func__ __FUNCTION__

/*! 
@group CUnitTest Core Test Macros
*/
#ifdef __UNHIDE_MARK_PRAGMAS__
#pragma mark -
#pragma mark CUnitTest Core Test Macros
#endif

/*!
 @define     CUT_RUN_TEST
 @abstract   Test runner.
 @param      test The name of the test function to run. The function is assumed
             to have the following signature: <code>int test_name(void)</code>.
             The function should return 0.
 @discussion Runs a single unit test using the given function and accumulates 
             statistics based on the returned value. By default, the test 
             function must return 0 if no errors occur during its execution. If
             an error does occur, the assertion macro that detects the error 
             will return a different value.
 */
#define CUT_RUN_TEST(test) \
do { \
    int f; \
    extern int test(void); \
    if (CUT_fixture_setup) CUT_fixture_setup(); \
    f = test(); \
    if (CUT_fixture_teardown) CUT_fixture_teardown(); \
    CUT_suite_tests_run++; \
    CUT_suite_tests_failed += f; \
    CUT_tests_run++; \
    CUT_tests_failed += f; \
} while (0)

/*!
 @define     CUT_RUN_SUITE_TESTS
 @abstract   Suite test runner.
 @param      name The name of the test suite to run.
 @discussion Runs the suite specified by name, accumulating statistics based 
             on the outcome of the tests in the suite.
 */
#define CUT_RUN_SUITE_TESTS(name) \
do { \
    extern void CUT_SUITE_ ## name(void); \
    CUT_SUITE_ ## name(); \
} while(0)

/*
 @parseOnly
 (Internal) The name for the default suite 
 (used if tests are run without a suite)
 */
#define CUT_NO_SUITE ""

/*
 @parseOnly
 (Internal) Sets up the test harness global vars for statistics collection
 */
#define _CUT_INIT int CUT_tests_run = 0; \
int CUT_tests_failed = 0;\
int CUT_suite_tests_run = 0; \
int CUT_suite_tests_failed = 0; \
int CUT_assertions_count = 0; \
const char * CUT_curr_suite = CUT_NO_SUITE

/*!
 @define     CUT_BEGIN_TEST_HARNESS
 @abstract   Begin test harness definition.
 @discussion Beginning of the definition of the test harness. Basically creates
             a <code>main()</code> function to run the specified tests/suite.
             Bad things will happen if this is called more than once!
 */
#define CUT_BEGIN_TEST_HARNESS _CUT_INIT; int main(void) {

/*!
 @define     CUT_END_TEST_HARNESS
 @abstract   End test harness definition.
 @discussion Ending of the definition of the test harness. Completes the 
             <code>main()</code> function created by 
             @link CUT_BEGIN_TEST_HARNESS CUT_BEGIN_TEST_HARNESS @/link and 
             prints the overall test statistics.
*/
#define CUT_END_TEST_HARNESS \
    fprintf(stdout, \
            "%s: tests-run: %d, assertions: %d, passes: %d, failures: %d\n", \
            "Overall Results", \
            CUT_tests_run, \
            CUT_assertions_count, \
            CUT_tests_run-CUT_tests_failed, \
            CUT_tests_failed); \
    return CUT_tests_failed; \
}

/*!
 @define     CUT_BEGIN_SUITE
 @abstract   Begin test suite definition.
 @param      name Name of the test suite.
 @discussion Beginning of the definition of test suite without fixtures. Creates
             a function to run the tests in the suite definition.
 */
#define CUT_BEGIN_SUITE(name) CUT_BEGIN_SUITE_F(name, NULL, NULL)

/*!
 @define     CUT_END_SUITE
 @abstract   End test suite definition.
 @discussion End of a test suite definition. Completes the function started by 
             the corresponding @link CUT_BEGIN_SUITE CUT_BEGIN_SUITE @/link
             definition and prints statistics of tests in the suite.
 */
#define CUT_END_SUITE \
    fprintf(stdout, \
            "%s: tests-run: %d, assertions: %d, passes: %d, failures: %d\n", \
            CUT_curr_suite, \
            CUT_suite_tests_run, \
            (CUT_assertions_count-CUT_assertions_count_init), \
            CUT_suite_tests_run-CUT_suite_tests_failed, \
            CUT_suite_tests_failed); \
    CUT_curr_suite=CUT_NO_SUITE; \
}

/*!
 @define     CUT_BEGIN_SUITE_F
 @abstract   Begin test suite definition with fixtures.
 @param      name Name of the test suite.
 @param      setup Name of the fixture setup function.
 @param      teardown Name of the fixture teardown function.
 @discussion Beginning of the definition of test suite with fixtures. Creates
             a function to run the tests in the suite definition. Before each 
             test, the setup function is run to initialise any global variables
             required by the test function. After the test is run, the teardown
             function is called to reset and free any global variables used by
             the test. The fixture functions both have <code>void</code> return 
             types and arguments. Either, or both the fixture function names may
             be NULL if you don't want them to run.
 */
#define CUT_BEGIN_SUITE_F(name, setup, teardown) \
void CUT_SUITE_ ## name(void) { \
    CUT_suite_tests_run=0; \
    CUT_suite_tests_failed=0; \
    CUT_curr_suite=#name; \
    CUT_fixture_setup=setup; \
    CUT_fixture_teardown=teardown; \
    int CUT_assertions_count_init = CUT_assertions_count;

/*!
 @define     CUT_END_SUITE_F
 @abstract   End test suite definition with fixtures.
 @discussion End of a test suite definition with fixtures. Completes the 
             function started by the corresponding @link CUT_BEGIN_SUITE_F 
             CUT_BEGIN_SUITE_F @/link definition, cleans up the internal fixture
             pointers and prints statistics of tests in the suite.
 */
#define CUT_END_SUITE_F \
    CUT_fixture_setup=NULL; \
    CUT_fixture_teardown=NULL; \
    CUT_END_SUITE

/*! 
  @group CUnitTest Utility Macros
*/
#ifdef __UNHIDE_MARK_PRAGMAS__
#pragma mark -
#pragma mark CUnitTest Utility Macros
#endif

#ifndef CUT_MAX
/*!
 @define     CUT_MAX
 @abstract   Maximum.
 @param      a An argument
 @param      b Another argument
 @discussion Returns the larger of the two values.
 */
#define CUT_MAX(a,b) ((a > b) ? a : b)
#endif

#ifndef CUT_MIN
/*!
 @define     CUT_MIN
 @abstract   Mimimum.
 @param      a An argument
 @param      b Another argument
 @discussion Returns the smaller of the two values.
 */
#define CUT_MIN(a,b) ((a < b) ? a : b)
#endif

/*! 
  @group CUnitTest Assertion Macros
*/
#ifdef __UNHIDE_MARK_PRAGMAS__
#pragma mark -
#pragma mark CUnitTest Assertion Macros
#endif

/*!
    @define     CUT_ASSERT
    @abstract   Basic assertion macro.
    @param      condition An expression that should evaluate to anything 
                non-false on success and false on failure.
    @discussion The basic CUnitTest assertion macro prints the file, line 
                number, and reason for failure if the passed expression is 
                false. 
*/
#define CUT_ASSERT(condition) \
do { \
    ++CUT_assertions_count; \
    if (!(condition)) { \
        fprintf(stderr, \
                "%s:%d: error: Test %s%s%s failed : %s\n", \
                __FILE__, \
                __LINE__, \
                CUT_curr_suite, \
                (CUT_curr_suite[0]=='\0' ? "":":"), \
                __func__, \
                #condition); \
        return 1; \
    } \
} while (0)

/*!
 @define     CUT_ASSERT_MESSAGE
 @abstract   Basic assertion macro with custom message.
 @param      message A string containing a custom message to print if the 
             assertion fails.
 @param      condition An expression that should evaluate to anything 
             non-false on success and false on failure.
 @discussion The assertion macro prints the file, line number, reason for 
             failure and custom message if the passed expression is false. 
 */
#define CUT_ASSERT_MESSAGE(message, condition) \
do { \
    ++CUT_assertions_count; \
    if (!(condition)) { \
        fprintf(stderr, \
                "%s:%d: error: Test %s%s%s failed : %s. %s\n", \
                __FILE__, \
                __LINE__, \
                CUT_curr_suite, \
                (CUT_curr_suite[0]=='\0' ? "":":"), \
                __func__, \
                #condition, \
                message); \
        return 1; \
    } \
} while (0)

/*!
 @define     CUT_FAIL
 @abstract   Failure macro with custom message.
 @param      message A string containing a custom message to print.
 @discussion The failure macro prints the file, line number, and custom message.
 */
#define	CUT_FAIL(message) \
do { \
    ++CUT_assertions_count; \
    fprintf(stderr, \
            "%s:%d: error: Test %s%s%s failed : %s. %s\n", \
            __FILE__, \
            __LINE__, \
            CUT_curr_suite, \
            (CUT_curr_suite[0]=='\0' ? "":":"), \
            __func__, \
            "CUT_FAIL()", \
            message); \
    return 1; \
} while (0)

/*!
 @define     CUT_ASSERT_TRUE
 @abstract   Assertion macro.
 @param      condition An expression that should evaluate to true on success and
             false on failure.
 @discussion The macro prints the file, line  number, and reason for failure if 
        the passed expression is false. 
 */
#define CUT_ASSERT_TRUE(condition) CUT_ASSERT(condition)

/*!
 @define     CUT_ASSERT_FALSE
 @abstract   Assertion macro.
 @param      condition An expression that should evaluate to false on success 
             and true on failure.
 @discussion The macro prints the file, line  number, and reason for failure if 
             the passed expression is true. 
 */
#define CUT_ASSERT_FALSE(condition) CUT_ASSERT(!condition)

/*!
 @define     CUT_ASSERT_NULL
 @abstract   Assertion macro.
 @param      val An expression that should evaluate to NULL on success 
             and anything but NULL on failure.
 @discussion The macro prints the file, line  number, and reason for failure if 
             the passed expression is something other than NULL. 
 */
#define CUT_ASSERT_NULL(val) CUT_ASSERT(val==NULL)

/*!
 @define     CUT_ASSERT_NOT_NULL
 @abstract   Assertion macro.
 @param      val An expression that should evaluate to something other than NULL
             on success and NULL on failure.
 @discussion The macro prints the file, line  number, and reason for failure if 
             the passed expression is NULL. 
 */
#define CUT_ASSERT_NOT_NULL(val) CUT_ASSERT(val!=NULL)

/*!
 @define     CUT_ASSERT_EQUAL
 @abstract   Assertion macro.
 @param      expected An expression that evaluates to the expected value.
 @param      actual An expression that evaluates to the actual value.
 @discussion The macro prints the file, line  number, and reason for failure if 
             the expected value does not equal the actual value. This should not
             normally be used for floats or doubles as two the values will only
             be considered equal if they have exactly the same binary 
             representation, which might be unlikely due to the impreciseness
             of floating-point representations. To compare floats or doubles, 
             use @link CUT_ASSERT_DOUBLES_EQUAL CUT_ASSERT_DOUBLES_EQUAL @/link
             instead.
 */
#define CUT_ASSERT_EQUAL(expected, actual) CUT_ASSERT(expected == actual)

/*!
 @define     CUT_ASSERT_NOT_EQUAL
 @abstract   Assertion macro.
 @param      expected An expression that evaluates to the expected value.
 @param      actual An expression that evaluates to the actual value.
 @discussion The macro prints the file, line  number, and reason for failure if 
             the expected value equals the actual value.
 */
#define CUT_ASSERT_NOT_EQUAL(expected, actual) CUT_ASSERT(expected != actual)

/*!
 @define     CUT_ASSERT_EQUAL_MESSAGE
 @abstract   Assertion macro with custom message.
 @param      message A string containing a custom message to print if the 
             assertion fails.
 @param      expected An expression that evaluates to the expected value.
 @param      actual An expression that evaluates to the actual value.
 @discussion The assertion macro prints the file, line number, reason for 
             failure and custom message if the actual value does not equal the
             expected value.
 */
#define CUT_ASSERT_EQUAL_MESSAGE(message, expected, actual) \
    CUT_ASSERT(message, expected == actual)

/*!
 @define     CUT_ASSERT_NOT_EQUAL_MESSAGE
 @abstract   Assertion macro with custom message.
 @param      message A string containing a custom message to print if the 
             assertion fails.
 @param      expected An expression that evaluates to the expected value.
 @param      actual An expression that evaluates to the actual value.
 @discussion The assertion macro prints the file, line number, reason for 
             failure and custom message if the actual value equals the expected
             value.
 */
#define CUT_ASSERT_NOT_EQUAL_MESSAGE(message, expected, actual) \
    CUT_ASSERT(message, expected != actual)

/*!
 @define     CUT_ASSERT_DOUBLES_EQUAL
 @abstract   Assertion macro.
 @param      expected An expression that evaluates to the expected value.
 @param      actual An expression that evaluates to the actual value.
 @param      delta The maximum allowable tolerence when deciding whether the 
             actual and expected values are equal.
 @discussion The assertion macro prints the file, line number, reason for 
             failure and custom message if the actual value does not equal the
             expected value within the given tolerence.
 */
#define CUT_ASSERT_DOUBLES_EQUAL(expected, actual, delta) \
    CUT_ASSERT((CUT_MAX(expected, actual) - CUT_MIN(expected, actual)) <= delta)

#endif
