/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    Matrix-inl.h
 * @brief   
 * @author  Richard Roberts
 * @created Sep 26, 2010
 */

#pragma once

#include <gtsam/base/Matrix.h>

#include <iostream>

namespace gtsam {

using namespace std;

/* ************************************************************************* */
/**
 * backSubstitute U*x=b
 * @param U an upper triangular matrix
 * @param b an RHS vector
 * @return the solution x of U*x=b
 */
template<class MatrixAE, class VectorAE>
Vector backSubstituteUpper(const boost::numeric::ublas::matrix_expression<MatrixAE>& _U,
    const boost::numeric::ublas::vector_expression<VectorAE>& _b, bool unit=false) {
  const MatrixAE& U((const MatrixAE&)_U);
  const VectorAE& b((const VectorAE&)_b);
  size_t n = U.size2();
#ifndef NDEBUG
  size_t m = U.size1();
  if (m!=n)
    throw invalid_argument("backSubstituteUpper: U must be square");
#endif

  Vector result(n);
  for (size_t i = n; i > 0; i--) {
    double zi = b(i-1);
    for (size_t j = i+1; j <= n; j++)
      zi -= U(i-1,j-1) * result(j-1);
#ifndef NDEBUG
    if(!unit && fabs(U(i-1,i-1)) <= numeric_limits<double>::epsilon()) {
      stringstream ss;
      ss << "backSubstituteUpper: U is singular,\n";
      print(U, "U: ", ss);
      throw invalid_argument(ss.str());
    }
#endif
    if (!unit) zi /= U(i-1,i-1);
    result(i-1) = zi;
  }

  return result;
}

/* ************************************************************************* */
/**
 * backSubstitute x'*U=b'
 * @param U an upper triangular matrix
 * @param b an RHS vector
 * @param unit, set true if unit triangular
 * @return the solution x of x'*U=b'
 * TODO: use boost
 */
template<class VectorAE, class MatrixAE>
Vector backSubstituteUpper(const boost::numeric::ublas::vector_expression<VectorAE>& _b,
    const boost::numeric::ublas::matrix_expression<MatrixAE>& _U, bool unit=false) {
  const VectorAE& b((const VectorAE&)_b);
  const MatrixAE& U((const MatrixAE&)_U);
  size_t n = U.size2();
#ifndef NDEBUG
  size_t m = U.size1();
  if (m!=n)
    throw invalid_argument("backSubstituteUpper: U must be square");
#endif

  Vector result(n);
  for (size_t i = 1; i <= n; i++) {
    double zi = b(i-1);
    for (size_t j = 1; j < i; j++)
      zi -= U(j-1,i-1) * result(j-1);
#ifndef NDEBUG
    if(!unit && fabs(U(i-1,i-1)) <= numeric_limits<double>::epsilon()) {
      stringstream ss;
      ss << "backSubstituteUpper: U is singular,\n";
      print(U, "U: ", ss);
      throw invalid_argument(ss.str());
    }
#endif
    if (!unit) zi /= U(i-1,i-1);
    result(i-1) = zi;
  }

  return result;
}

}
