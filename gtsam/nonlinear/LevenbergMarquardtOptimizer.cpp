/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    LevenbergMarquardtOptimizer.cpp
 * @brief   
 * @author  Richard Roberts
 * @author  Luca Carlone
 * @date  Feb 26, 2012
 */

#include <gtsam/nonlinear/LevenbergMarquardtOptimizer.h>
#include <gtsam/linear/linearExceptions.h>
#include <gtsam/linear/GaussianFactorGraph.h>
#include <gtsam/linear/VectorValues.h>
#include <gtsam/linear/Errors.h>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>
#include <string>
#include <cmath>
#include <fstream>

using namespace std;

namespace gtsam {

using boost::adaptors::map_values;

/* ************************************************************************* */
LevenbergMarquardtParams::VerbosityLM LevenbergMarquardtParams::verbosityLMTranslator(
    const std::string &src) const {
  std::string s = src;
  boost::algorithm::to_upper(s);
  if (s == "SILENT")
    return LevenbergMarquardtParams::SILENT;
  if (s == "LAMBDA")
    return LevenbergMarquardtParams::LAMBDA;
  if (s == "TRYLAMBDA")
    return LevenbergMarquardtParams::TRYLAMBDA;
  if (s == "TRYCONFIG")
    return LevenbergMarquardtParams::TRYCONFIG;
  if (s == "TRYDELTA")
    return LevenbergMarquardtParams::TRYDELTA;
  if (s == "DAMPED")
    return LevenbergMarquardtParams::DAMPED;

  /* default is silent */
  return LevenbergMarquardtParams::SILENT;
}

/* ************************************************************************* */
std::string LevenbergMarquardtParams::verbosityLMTranslator(
    VerbosityLM value) const {
  std::string s;
  switch (value) {
  case LevenbergMarquardtParams::SILENT:
    s = "SILENT";
    break;
  case LevenbergMarquardtParams::TERMINATION:
    s = "TERMINATION";
    break;
  case LevenbergMarquardtParams::LAMBDA:
    s = "LAMBDA";
    break;
  case LevenbergMarquardtParams::TRYLAMBDA:
    s = "TRYLAMBDA";
    break;
  case LevenbergMarquardtParams::TRYCONFIG:
    s = "TRYCONFIG";
    break;
  case LevenbergMarquardtParams::TRYDELTA:
    s = "TRYDELTA";
    break;
  case LevenbergMarquardtParams::DAMPED:
    s = "DAMPED";
    break;
  default:
    s = "UNDEFINED";
    break;
  }
  return s;
}

/* ************************************************************************* */
void LevenbergMarquardtParams::print(const std::string& str) const {
  NonlinearOptimizerParams::print(str);
  std::cout << "              lambdaInitial: " << lambdaInitial << "\n";
  std::cout << "               lambdaFactor: " << lambdaFactor << "\n";
  std::cout << "           lambdaUpperBound: " << lambdaUpperBound << "\n";
  std::cout << "           lambdaLowerBound: " << lambdaLowerBound << "\n";
  std::cout << "     disableInnerIterations: " << disableInnerIterations
      << "\n";
  std::cout << "           minModelFidelity: " << minModelFidelity << "\n";
  std::cout << "            diagonalDamping: " << diagonalDamping << "\n";
  std::cout << "                verbosityLM: "
      << verbosityLMTranslator(verbosityLM) << "\n";
  std::cout.flush();
}

/* ************************************************************************* */
GaussianFactorGraph::shared_ptr LevenbergMarquardtOptimizer::linearize() const {
  return graph_.linearize(state_.values);
}

/* ************************************************************************* */
void LevenbergMarquardtOptimizer::increaseLambda() {
  if (params_.useFixedLambdaFactor_) {
    state_.lambda *= params_.lambdaFactor;
  } else {
    state_.lambda *= params_.lambdaFactor;
    params_.lambdaFactor *= 2.0;
  }
  params_.reuse_diagonal_ = true;
}

/* ************************************************************************* */
void LevenbergMarquardtOptimizer::decreaseLambda(double stepQuality) {

  if (params_.useFixedLambdaFactor_) {
    state_.lambda /= params_.lambdaFactor;
  } else {
    // CHECK_GT(step_quality, 0.0);
    state_.lambda *= std::max(1.0 / 3.0, 1.0 - pow(2.0 * stepQuality - 1.0, 3));
    params_.lambdaFactor = 2.0;
  }
  state_.lambda = std::max(params_.lambdaLowerBound, state_.lambda);
  params_.reuse_diagonal_ = false;

}

/* ************************************************************************* */
GaussianFactorGraph LevenbergMarquardtOptimizer::buildDampedSystem(
    const GaussianFactorGraph& linear) {

  //Set two parameters as Ceres, will move out later
  static const double min_diagonal_ = 1e-6;
  static const double max_diagonal_ = 1e32;

  gttic(damp);
  if (params_.verbosityLM >= LevenbergMarquardtParams::DAMPED)
    cout << "building damped system with lambda " << state_.lambda << endl;

  // Only retrieve diagonal vector when reuse_diagonal = false
  if (params_.diagonalDamping && params_.reuse_diagonal_ == false) {
    state_.hessianDiagonal = linear.hessianDiagonal();
    BOOST_FOREACH(Vector& v, state_.hessianDiagonal | map_values) {
      for (int aa = 0; aa < v.size(); aa++) {
        v(aa) = std::min(std::max(v(aa), min_diagonal_), max_diagonal_);
        v(aa) = sqrt(v(aa));
      }
    }
  } // reuse diagonal

  // for each of the variables, add a prior
  double sigma = 1.0 / std::sqrt(state_.lambda);
  GaussianFactorGraph damped = linear;
  damped.reserve(damped.size() + state_.values.size());
  if (params_.diagonalDamping) {
    BOOST_FOREACH(const VectorValues::KeyValuePair& key_vector, state_.hessianDiagonal) {
      // Fill in the diagonal of A with diag(hessian)
      try {
        Matrix A = Eigen::DiagonalMatrix<double, Eigen::Dynamic>(state_.hessianDiagonal.at(key_vector.first));
        size_t dim = key_vector.second.size();
        Vector b = Vector::Zero(dim);
        SharedDiagonal model = noiseModel::Isotropic::Sigma(dim, sigma);
        damped += boost::make_shared<JacobianFactor>(key_vector.first, A, b, model);
      } catch (std::exception e) {
        // Don't attempt any damping if no key found in diagonal
        continue;
      }
    }
  } else {
    // Straightforward damping:
    BOOST_FOREACH(const Values::KeyValuePair& key_value, state_.values) {
      size_t dim = key_value.value.dim();
      Matrix A = Matrix::Identity(dim, dim);
      Vector b = Vector::Zero(dim);
      SharedDiagonal model = noiseModel::Isotropic::Sigma(dim, sigma);
      damped += boost::make_shared<JacobianFactor>(key_value.key, A, b, model);
    }
  }
  gttoc(damp);
  return damped;
}

/* ************************************************************************* */
void LevenbergMarquardtOptimizer::iterate() {

  gttic(LM_iterate);

  // Pull out parameters we'll use
  const NonlinearOptimizerParams::Verbosity nloVerbosity = params_.verbosity;
  const LevenbergMarquardtParams::VerbosityLM lmVerbosity = params_.verbosityLM;

  // Linearize graph
  if (lmVerbosity >= LevenbergMarquardtParams::DAMPED)
    cout << "linearizing = " << endl;
  GaussianFactorGraph::shared_ptr linear = linearize();

  // Keep increasing lambda until we make make progress
  while (true) {

    if (lmVerbosity >= LevenbergMarquardtParams::TRYLAMBDA)
      cout << "trying lambda = " << state_.lambda << endl;

    // Build damped system for this lambda (adds prior factors that make it like gradient descent)
    GaussianFactorGraph dampedSystem = buildDampedSystem(*linear);

    // Log current error/lambda to file
    if (!params_.logFile.empty()) {
      ofstream os(params_.logFile.c_str(), ios::app);

      boost::posix_time::ptime currentTime =
          boost::posix_time::microsec_clock::universal_time();

      os << state_.totalNumberInnerIterations << ","
          << 1e-6 * (currentTime - state_.startTime).total_microseconds() << ","
          << state_.error << "," << state_.lambda << endl;
    }

    ++state_.totalNumberInnerIterations;

    // Try solving
    double modelFidelity = 0.0;
    bool step_is_successful = false;
    bool stopSearchingLambda = false;
    double newError;
    Values newValues;
    VectorValues delta;

    bool systemSolvedSuccessfully;
    try {
      delta = solve(dampedSystem, state_.values, params_);
      systemSolvedSuccessfully = true;
    } catch (IndeterminantLinearSystemException& e) {
      systemSolvedSuccessfully = false;
    }

    if (systemSolvedSuccessfully) {
      params_.reuse_diagonal_ = true;

      if (lmVerbosity >= LevenbergMarquardtParams::TRYLAMBDA)
        cout << "linear delta norm = " << delta.norm() << endl;
      if (lmVerbosity >= LevenbergMarquardtParams::TRYDELTA)
        delta.print("delta");

      // cost change in the linearized system (old - new)
      double newlinearizedError = linear->error(delta);

      double linearizedCostChange = state_.error - newlinearizedError;

      if (linearizedCostChange >= 0) { // step is valid
        // update values
        gttic(retract);
        newValues = state_.values.retract(delta);
        gttoc(retract);

        // compute new error
        gttic(compute_error);
        if (lmVerbosity >= LevenbergMarquardtParams::TRYLAMBDA)
          cout << "calculating error" << endl;
        newError = graph_.error(newValues);
        gttoc(compute_error);

        // cost change in the original, nonlinear system (old - new)
        double costChange = state_.error - newError;

        double absolute_function_tolerance = params_.relativeErrorTol
            * state_.error;
        if (fabs(costChange) >= absolute_function_tolerance) {
          // fidelity of linearized model VS original system between
          if (linearizedCostChange > 1e-15) {
            modelFidelity = costChange / linearizedCostChange;
            // if we decrease the error in the nonlinear system and modelFidelity is above threshold
            step_is_successful = modelFidelity > params_.minModelFidelity;
          } else {
            step_is_successful = true; // linearizedCostChange close to zero
          }
        } else {
          stopSearchingLambda = true;
        }
      }
    }

    if (step_is_successful) { // we have successfully decreased the cost and we have good modelFidelity
      state_.values.swap(newValues);
      state_.error = newError;
      decreaseLambda(modelFidelity);
      break;
    } else if (!stopSearchingLambda) { // we failed to solved the system or we had no decrease in cost
      if (lmVerbosity >= LevenbergMarquardtParams::TRYLAMBDA)
        cout << "increasing lambda: old error (" << state_.error
            << ") new error (" << newError << ")" << endl;
      increaseLambda();

      // check if lambda is too big
      if (state_.lambda >= params_.lambdaUpperBound) {
        if (nloVerbosity >= NonlinearOptimizerParams::TERMINATION)
          cout
              << "Warning:  Levenberg-Marquardt giving up because cannot decrease error with maximum lambda"
              << endl;
        break;
      }
    } else { // the change in the cost is very small and it is not worth trying bigger lambdas
      break;
    }
  } // end while

  // Increment the iteration counter
  ++state_.iterations;
}

/* ************************************************************************* */
LevenbergMarquardtParams LevenbergMarquardtOptimizer::ensureHasOrdering(
    LevenbergMarquardtParams params, const NonlinearFactorGraph& graph) const {
  if (!params.ordering)
    params.ordering = Ordering::COLAMD(graph);
  return params;
}

} /* namespace gtsam */

