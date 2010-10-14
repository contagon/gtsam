/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    VectorMap.cpp
 * @brief   Factor Graph Values
 * @brief   VectorMap
 * @author  Carlos Nieto
 * @author  Christian Potthast
 */

#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <gtsam/linear/VectorMap.h>

// trick from some reading group
#define FOREACH_PAIR( KEY, VAL, COL) BOOST_FOREACH (boost::tie(KEY,VAL),COL) 

using namespace std;

namespace gtsam {

/* ************************************************************************* */
void check_size(Index key, const Vector & vj, const Vector & dj) {
  if (dj.size()!=vj.size()) {
    cout << "For key \"" << key << "\"" << endl;
    cout << "vj.size = " << vj.size() << endl;
    cout << "dj.size = " << dj.size() << endl;
    throw(std::invalid_argument("VectorMap::+ mismatched dimensions"));
  }
}

/* ************************************************************************* */
std::vector<Index> VectorMap::get_names() const {
  std::vector<Index> names;
  for(const_iterator it=values.begin(); it!=values.end(); it++)
    names.push_back(it->first);
  return names;
}

/* ************************************************************************* */
VectorMap& VectorMap::insert(Index name, const Vector& v) {
  values.insert(std::make_pair(name,v));
  return *this;
}

/* ************************************************************************* */
VectorMap& VectorMap::insertAdd(Index j, const Vector& a) {
	Vector& vj = values[j];
	if (vj.size()==0) vj = a; else vj += a;
	return *this;
}

/* ************************************************************************* */
void VectorMap::insert(const VectorMap& config) {
	for (const_iterator it = config.begin(); it!=config.end(); it++)
		insert(it->first, it->second);
}

/* ************************************************************************* */
void VectorMap::insertAdd(const VectorMap& config) {
	for (const_iterator it = config.begin(); it!=config.end(); it++)
    insertAdd(it->first,it->second);
}

/* ************************************************************************* */
size_t VectorMap::dim() const {
	size_t result=0;
	for (const_iterator it = begin(); it != end(); it++)
		result += it->second.size();
	return result;
}

/* ************************************************************************* */
const Vector& VectorMap::operator[](Index name) const {
  return values.at(name);
}

/* ************************************************************************* */
Vector& VectorMap::operator[](Index name) {
  return values.at(name);
}

/* ************************************************************************* */
VectorMap VectorMap::scale(double s) const {
	VectorMap scaled;
	for (const_iterator it = begin(); it != end(); it++)
		scaled.insert(it->first, s*it->second);
	return scaled;
}

/* ************************************************************************* */
VectorMap VectorMap::operator*(double s) const {
	return scale(s);
}

/* ************************************************************************* */
VectorMap VectorMap::operator-() const {
	VectorMap result;
	for (const_iterator it = begin(); it != end(); it++)
		result.insert(it->first, - it->second);
	return result;
}

/* ************************************************************************* */
void VectorMap::operator+=(const VectorMap& b) {
	insertAdd(b);
}

/* ************************************************************************* */
VectorMap VectorMap::operator+(const VectorMap& b) const {
	VectorMap result = *this;
	result += b;
	return result;
}

/* ************************************************************************* */
VectorMap VectorMap::operator-(const VectorMap& b) const {
	VectorMap result;
	for (const_iterator it = begin(); it != end(); it++)
		result.insert(it->first, it->second - b[it->first]);
	return result;
}

/* ************************************************************************* */
VectorMap& VectorMap::zero() {
	for (iterator it = begin(); it != end(); it++)
		std::fill(it->second.begin(), it->second.end(), 0.0);
	return *this;
}

/* ************************************************************************* */
VectorMap VectorMap::zero(const VectorMap& x) {
	VectorMap cloned(x);
	return cloned.zero();
}

/* ************************************************************************* */
Vector VectorMap::vector() const {
	Vector result(dim());

	size_t cur_dim = 0;
	Index j; Vector vj;
	FOREACH_PAIR(j, vj, values) {
		subInsert(result, vj, cur_dim);
		cur_dim += vj.size();
	}
	return result;
}

/* ************************************************************************* */
VectorMap expmap(const VectorMap& original, const VectorMap& delta)
{
	VectorMap newValues;
	Index j; Vector vj; // rtodo: copying vector?
	FOREACH_PAIR(j, vj, original.values) {
		if (delta.contains(j)) {
			const Vector& dj = delta[j];
			check_size(j,vj,dj);
			newValues.insert(j, vj + dj);
		} else {
			newValues.insert(j, vj);
		}
	}
	return newValues;
}

/* ************************************************************************* */
VectorMap expmap(const VectorMap& original, const Vector& delta)
{
	VectorMap newValues;
	size_t i = 0;
	Index j; Vector vj; // rtodo: copying vector?
	FOREACH_PAIR(j, vj, original.values) {
		size_t mj = vj.size();
		Vector dj = sub(delta, i, i+mj);
		newValues.insert(j, vj + dj);
		i += mj;
	}
	return newValues;
}

/* ************************************************************************* */
void VectorMap::print(const string& name) const {
  odprintf("VectorMap %s\n", name.c_str());
  odprintf("size: %d\n", values.size());
  for (const_iterator it = begin(); it != end(); it++) {
    odprintf("%d:", it->first);
    gtsam::print(it->second);
  }
}

/* ************************************************************************* */
bool VectorMap::equals(const VectorMap& expected, double tol) const {
  if( values.size() != expected.size() ) return false;

  // iterate over all nodes
  for (const_iterator it = begin(); it != end(); it++) {
    Vector vExpected = expected[it->first];
    if(!equal_with_abs_tol(vExpected,it->second,tol))
    	return false;
  }
  return true;
}

/* ************************************************************************* */
double VectorMap::dot(const VectorMap& b) const {
  double result = 0.0; // rtodo: copying vector
  for (const_iterator it = begin(); it != end(); it++)
  	result += gtsam::dot(it->second,b[it->first]);
	return result;
}

/* ************************************************************************* */
double dot(const VectorMap& a, const VectorMap& b) {
	return a.dot(b);
}

/* ************************************************************************* */
void scal(double alpha, VectorMap& x) {
	for (VectorMap::iterator xj = x.begin(); xj != x.end(); xj++)
		scal(alpha, xj->second);
}

/* ************************************************************************* */
void axpy(double alpha, const VectorMap& x, VectorMap& y) {
	VectorMap::const_iterator xj = x.begin();
	for (VectorMap::iterator yj = y.begin(); yj != y.end(); yj++, xj++)
		axpy(alpha, xj->second, yj->second);
}

/* ************************************************************************* */
void print(const VectorMap& v, const std::string& s){
	v.print(s);
}

/* ************************************************************************* */

} // gtsam
