/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    testClusterTree.cpp
 * @brief   Unit tests for Bayes Tree
 * @author  Kai Ni
 * @author  Frank Dellaert
 */

#include <boost/assign/std/list.hpp> // for operator +=
using namespace boost::assign;

#include <gtsam/CppUnitLite/TestHarness.h>

#include <gtsam/inference/SymbolicFactorGraph.h>
#include <gtsam/inference/ClusterTree-inl.h>

using namespace gtsam;

// explicit instantiation and typedef
template class ClusterTree<SymbolicFactorGraph>;
typedef ClusterTree<SymbolicFactorGraph> SymbolicClusterTree;

/* ************************************************************************* */
int main() {
	TestResult tr;
	return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
