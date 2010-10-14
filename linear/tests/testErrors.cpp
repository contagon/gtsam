/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/*
 * testErrors.cpp
 *
 *  Created on: Feb 20, 2010
 *  @Author: Frank Dellaert
 */

#include <boost/assign/std/list.hpp> // for +=
using namespace boost::assign;

#include <gtsam/CppUnitLite/TestHarness.h>
#include <gtsam/linear/Errors.h>

using namespace std;
using namespace gtsam;

/* ************************************************************************* */
TEST( Errors, arithmetic )
{
	Errors e;
	e += Vector_(2,1.0,2.0), Vector_(3,3.0,4.0,5.0);
	DOUBLES_EQUAL(1+4+9+16+25,dot(e,e),1e-9);

	axpy(2.0,e,e);
	Errors expected;
	expected += Vector_(2,3.0,6.0), Vector_(3,9.0,12.0,15.0);
	CHECK(assert_equal(expected,e));
}

/* ************************************************************************* */
int main() {
	TestResult tr;
	return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
