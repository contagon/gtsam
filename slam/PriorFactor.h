/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 *  @file  PriorFactor.h
 *  @authors Frank Dellaert
 **/
#pragma once

#include <ostream>
#include <gtsam/linear/NoiseModel.h>
#include <gtsam/nonlinear/NonlinearFactor.h>
#include <gtsam/geometry/Pose2.h>

namespace gtsam {

	/**
	 * A class for a soft prior on any Lie type
	 * It takes three template parameters:
	 *   T is the Lie group type for which the prior is define
	 *   Key (typically TypedSymbol) is used to look up T's in a Values
	 *   Values where the T's are stored, typically LieValues<Key,T> or a TupleValues<...>
	 * The Key type is not arbitrary: we need to cast to a Symbol at linearize, so
	 * a simple type like int will not work
	 */
	template<class Values, class Key>
	class PriorFactor: public NonlinearFactor1<Values, Key> {

	public:
		typedef typename Key::Value T;

	private:

		typedef NonlinearFactor1<Values, Key> Base;

		T prior_; /** The measurement */

	public:

		// shorthand for a smart pointer to a factor
		typedef typename boost::shared_ptr<PriorFactor> shared_ptr;

		/** Constructor */
		PriorFactor(const Key& key, const T& prior,
				const SharedGaussian& model) :
			Base(model, key), prior_(prior) {
		}

		/** implement functions needed for Testable */

		/** print */
		void print(const std::string& s) const {
			Base::print(s);
			prior_.print("prior");
		}

		/** equals */
		bool equals(const NonlinearFactor<Values>& expected, double tol) const {
			const PriorFactor<Values, Key> *e = dynamic_cast<const PriorFactor<
					Values, Key>*> (&expected);
			return e != NULL && Base::equals(expected, tol) && this->prior_.equals(
					e->prior_, tol);
		}

		/** implement functions needed to derive from Factor */

		/** vector of errors */
		Vector evaluateError(const T& p, boost::optional<Matrix&> H = boost::none) const {
			if (H) (*H) = eye(p.dim());
			// manifold equivalent of h(x)-z -> log(z,h(x))
			return prior_.logmap(p);
		}
	};

} /// namespace gtsam
