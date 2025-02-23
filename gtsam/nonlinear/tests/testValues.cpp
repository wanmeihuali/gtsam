/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file testValues.cpp
 * @author Richard Roberts
 * @author Frank Dellaert
 * @author Mike Bosse
 */

#include <gtsam/nonlinear/Values.h>
#include <gtsam/inference/Symbol.h>
#include <gtsam/linear/VectorValues.h>
#include <gtsam/geometry/Pose2.h>
#include <gtsam/geometry/Pose3.h>
#include <gtsam/base/Testable.h>
#include <gtsam/base/TestableAssertions.h>

#include <CppUnitLite/TestHarness.h>
#include <boost/bind/bind.hpp>
#include <stdexcept>
#include <limits>
#include <type_traits>

using namespace std::placeholders;
using namespace gtsam;
using namespace std;
static double inf = std::numeric_limits<double>::infinity();

// Convenience for named keys
using symbol_shorthand::X;
using symbol_shorthand::L;

const Symbol key1('v', 1), key2('v', 2), key3('v', 3), key4('v', 4);


class TestValueData {
public:
  static int ConstructorCount;
  static int DestructorCount;
  TestValueData(const TestValueData& other) { ++ ConstructorCount; }
  TestValueData() { ++ ConstructorCount; }
  ~TestValueData() { ++ DestructorCount; }
};
int TestValueData::ConstructorCount = 0;
int TestValueData::DestructorCount = 0;
class TestValue {
  TestValueData data_;
public:
  enum {dimension = 0};
  void print(const std::string& str = "") const {}
  bool equals(const TestValue& other, double tol = 1e-9) const { return true; }
  size_t dim() const { return 0; }
  TestValue retract(const Vector&,
                    OptionalJacobian<dimension,dimension> H1={},
                    OptionalJacobian<dimension,dimension> H2={}) const {
    return TestValue();
  }
  Vector localCoordinates(const TestValue&,
                          OptionalJacobian<dimension,dimension> H1={},
                          OptionalJacobian<dimension,dimension> H2={}) const {
    return Vector();
  }
};

namespace gtsam {
template <> struct traits<TestValue> : public internal::Manifold<TestValue> {};
}

/* ************************************************************************* */
TEST( Values, equals1 )
{
  Values expected;
  Vector3 v(5.0, 6.0, 7.0);

  expected.insert(key1, v);
  Values actual;
  actual.insert(key1, v);
  CHECK(assert_equal(expected, actual));
}

/* ************************************************************************* */
TEST( Values, equals2 )
{
  Values cfg1, cfg2;
  Vector3 v1(5.0, 6.0, 7.0);
  Vector3 v2(5.0, 6.0, 8.0);

  cfg1.insert(key1, v1);
  cfg2.insert(key1, v2);
  CHECK(!cfg1.equals(cfg2));
  CHECK(!cfg2.equals(cfg1));
}

/* ************************************************************************* */
TEST( Values, equals_nan )
{
  Values cfg1, cfg2;
  Vector3 v1(5.0, 6.0, 7.0);
  Vector3 v2(inf, inf, inf);

  cfg1.insert(key1, v1);
  cfg2.insert(key1, v2);
  CHECK(!cfg1.equals(cfg2));
  CHECK(!cfg2.equals(cfg1));
}

/* ************************************************************************* */
TEST( Values, insert_good )
{
  Values cfg1, cfg2, expected;
  Vector3 v1(5.0, 6.0, 7.0);
  Vector3 v2(8.0, 9.0, 1.0);
  Vector3 v3(2.0, 4.0, 3.0);
  Vector3 v4(8.0, 3.0, 7.0);

  cfg1.insert(key1, v1);
  cfg1.insert(key2, v2);
  cfg2.insert(key3, v4);

  cfg1.insert(cfg2);

  expected.insert(key1, v1);
  expected.insert(key2, v2);
  expected.insert(key3, v4);

  CHECK(assert_equal(expected, cfg1));
}

/* ************************************************************************* */
TEST( Values, insert_bad )
{
  Values cfg1, cfg2;
  Vector3 v1(5.0, 6.0, 7.0);
  Vector3 v2(8.0, 9.0, 1.0);
  Vector3 v3(2.0, 4.0, 3.0);
  Vector3 v4(8.0, 3.0, 7.0);

  cfg1.insert(key1, v1);
  cfg1.insert(key2, v2);
  cfg2.insert(key2, v3);
  cfg2.insert(key3, v4);

  CHECK_EXCEPTION(cfg1.insert(cfg2), ValuesKeyAlreadyExists);
}

/* ************************************************************************* */
TEST( Values, update_element )
{
  Values cfg;
  Vector3 v1(5.0, 6.0, 7.0);
  Vector3 v2(8.0, 9.0, 1.0);

  cfg.insert(key1, v1);
  CHECK(cfg.size() == 1);
  CHECK(assert_equal((Vector)v1, cfg.at<Vector3>(key1)));

  cfg.update(key1, v2);
  CHECK(cfg.size() == 1);
  CHECK(assert_equal((Vector)v2, cfg.at<Vector3>(key1)));
}

TEST(Values, InsertOrAssign) {
  Values values;
  Key X(0);
  double x = 1;
  
  CHECK(values.size() == 0);
  // This should perform an insert.
  values.insert_or_assign(X, x);
  EXPECT(assert_equal(values.at<double>(X), x));

  // This should perform an update.
  double y = 2;
  values.insert_or_assign(X, y);
  EXPECT(assert_equal(values.at<double>(X), y));
}

/* ************************************************************************* */
TEST(Values, basic_functions)
{
  Values values;
  Matrix23 M1 = Matrix23::Zero(), M2 = Matrix23::Zero();
  values.insert(2, Vector3(0, 0, 0));
  values.insert(4, Vector3(0, 0, 0));
  values.insert(6, M1);
  values.insert(8, M2);

  EXPECT(!values.exists(1));
  EXPECT(values.exists(2));
  EXPECT(values.exists(4));
  EXPECT(values.exists(6));
  EXPECT(values.exists(8));
}

/* ************************************************************************* */
TEST(Values, retract_full)
{
  Values config0;
  config0.insert(key1, Vector3(1.0, 2.0, 3.0));
  config0.insert(key2, Vector3(5.0, 6.0, 7.0));

  const VectorValues delta{{key1, Vector3(1.0, 1.1, 1.2)},
                           {key2, Vector3(1.3, 1.4, 1.5)}};

  Values expected;
  expected.insert(key1, Vector3(2.0, 3.1, 4.2));
  expected.insert(key2, Vector3(6.3, 7.4, 8.5));

  CHECK(assert_equal(expected, config0.retract(delta)));
  CHECK(assert_equal(expected, Values(config0, delta)));
}

/* ************************************************************************* */
TEST(Values, retract_partial)
{
  Values config0;
  config0.insert(key1, Vector3(1.0, 2.0, 3.0));
  config0.insert(key2, Vector3(5.0, 6.0, 7.0));

  const VectorValues delta{{key2, Vector3(1.3, 1.4, 1.5)}};

  Values expected;
  expected.insert(key1, Vector3(1.0, 2.0, 3.0));
  expected.insert(key2, Vector3(6.3, 7.4, 8.5));

  CHECK(assert_equal(expected, config0.retract(delta)));
  CHECK(assert_equal(expected, Values(config0, delta)));
}

/* ************************************************************************* */
TEST(Values, retract_masked)
{
  Values config0;
  config0.insert(key1, Vector3(1.0, 2.0, 3.0));
  config0.insert(key2, Vector3(5.0, 6.0, 7.0));

  const VectorValues delta{{key1, Vector3(1.0, 1.1, 1.2)},
                           {key2, Vector3(1.3, 1.4, 1.5)}};

  Values expected;
  expected.insert(key1, Vector3(1.0, 2.0, 3.0));
  expected.insert(key2, Vector3(6.3, 7.4, 8.5));

  config0.retractMasked(delta, {key2});
  CHECK(assert_equal(expected, config0));
}

/* ************************************************************************* */
TEST(Values, equals)
{
  Values config0;
  config0.insert(key1, Vector3(1.0, 2.0, 3.0));
  config0.insert(key2, Vector3(5.0, 6.0, 7.0));

  CHECK(equal(config0, config0));
  CHECK(config0.equals(config0));

  Values poseconfig;
  poseconfig.insert(key1, Pose2(1, 2, 3));
  poseconfig.insert(key2, Pose2(0.3, 0.4, 0.5));

  CHECK(equal(poseconfig, poseconfig));
  CHECK(poseconfig.equals(poseconfig));
}

/* ************************************************************************* */
TEST(Values, localCoordinates)
{
  Values valuesA;
  valuesA.insert(key1, Vector3(1.0, 2.0, 3.0));
  valuesA.insert(key2, Vector3(5.0, 6.0, 7.0));

  VectorValues expDelta{{key1, Vector3(0.1, 0.2, 0.3)},
                        {key2, Vector3(0.4, 0.5, 0.6)}};

  Values valuesB = valuesA.retract(expDelta);

  EXPECT(assert_equal(expDelta, valuesA.localCoordinates(valuesB)));
}

/* ************************************************************************* */
TEST(Values, extract_keys)
{
  Values config;

  config.insert(key1, Pose2());
  config.insert(key2, Pose2());
  config.insert(key3, Pose2());
  config.insert(key4, Pose2());

  KeyVector expected {key1, key2, key3, key4};
  KeyVector actual = config.keys();

  CHECK(actual.size() == expected.size());
  KeyVector::const_iterator itAct = actual.begin(), itExp = expected.begin();
  for (; itAct != actual.end() && itExp != expected.end(); ++itAct, ++itExp) {
    EXPECT(*itExp == *itAct);
  }
}

/* ************************************************************************* */
TEST(Values, exists_)
{
  Values config0;
  config0.insert(key1, 1.0);
  config0.insert(key2, 2.0);

  const double* v = config0.exists<double>(key1);
  DOUBLES_EQUAL(1.0,*v,1e-9);
}

/* ************************************************************************* */
TEST(Values, update)
{
  Values config0;
  config0.insert(key1, 1.0);
  config0.insert(key2, 2.0);

  Values superset;
  superset.insert(key1, -1.0);
  superset.insert(key2, -2.0);
  config0.update(superset);

  Values expected;
  expected.insert(key1, -1.0);
  expected.insert(key2, -2.0);
  CHECK(assert_equal(expected, config0));
}

/* ************************************************************************* */
TEST(Values, filter) {
  Pose2 pose0(1.0, 2.0, 0.3);
  Pose3 pose1(Pose2(0.1, 0.2, 0.3));
  Pose2 pose2(4.0, 5.0, 0.6);
  Pose3 pose3(Pose2(0.3, 0.7, 0.9));

  Values values;
  values.insert(0, pose0);
  values.insert(1, pose1);
  values.insert(2, pose2);
  values.insert(3, pose3);

  // Test counting by type.
  EXPECT_LONGS_EQUAL(2, (long)values.count<Pose3>());
  EXPECT_LONGS_EQUAL(2, (long)values.count<Pose2>());

  // Filter by type using extract.
  auto extracted_pose3s = values.extract<Pose3>();
  EXPECT_LONGS_EQUAL(2, (long)extracted_pose3s.size());
}

/* ************************************************************************* */
TEST(Values, Symbol_filter) {
  Pose2 pose0(1.0, 2.0, 0.3);
  Pose3 pose1(Pose2(0.1, 0.2, 0.3));
  Pose2 pose2(4.0, 5.0, 0.6);
  Pose3 pose3(Pose2(0.3, 0.7, 0.9));

  Values values;
  values.insert(X(0), pose0);
  values.insert(Symbol('y', 1), pose1);
  values.insert(X(2), pose2);
  values.insert(Symbol('y', 3), pose3);

  // Test extract with filter on symbol:
  auto extracted_pose3s = values.extract<Pose3>(Symbol::ChrTest('y'));
  EXPECT_LONGS_EQUAL(2, (long)extracted_pose3s.size());
}

/* ************************************************************************* */
// Check that Value destructors are called when Values container is deleted
TEST(Values, Destructors) {
  {
    Values values;
    {
      TestValue value1;
      TestValue value2;
      LONGS_EQUAL(2, (long)TestValueData::ConstructorCount);
      LONGS_EQUAL(0, (long)TestValueData::DestructorCount);
      values.insert(0, value1);
      values.insert(1, value2);
    }
    // additional 2 con/destructor counts for the temporary
    // GenericValue<TestValue> in insert()
    // but I'm sure some advanced programmer can figure out
    // a way to avoid the temporary, or optimize it out
    LONGS_EQUAL(4 + 2, (long)TestValueData::ConstructorCount);
    LONGS_EQUAL(2 + 2, (long)TestValueData::DestructorCount);
  }
  LONGS_EQUAL(4 + 2, (long)TestValueData::ConstructorCount);
  LONGS_EQUAL(4 + 2, (long)TestValueData::DestructorCount);
}

/* ************************************************************************* */
TEST(Values, copy_constructor) {
  {
    Values values;
    TestValueData::ConstructorCount = 0;
    TestValueData::DestructorCount = 0;
    {
      TestValue value1;
      TestValue value2;
      EXPECT_LONGS_EQUAL(2, (long)TestValueData::ConstructorCount);
      EXPECT_LONGS_EQUAL(0, (long)TestValueData::DestructorCount);
      values.insert(0, value1);
      values.insert(1, value2);
    }
    EXPECT_LONGS_EQUAL(6, (long)TestValueData::ConstructorCount);
    EXPECT_LONGS_EQUAL(4, (long)TestValueData::DestructorCount);

    // Copy constructor
    {
      Values copied(values); // makes 2 extra copies
      EXPECT_LONGS_EQUAL(8, (long)TestValueData::ConstructorCount);
      EXPECT_LONGS_EQUAL(4, (long)TestValueData::DestructorCount);
    }
    EXPECT_LONGS_EQUAL(8, (long)TestValueData::ConstructorCount);
    EXPECT_LONGS_EQUAL(6, (long)TestValueData::DestructorCount); // copied destructed !
  }
  EXPECT_LONGS_EQUAL(8, (long)TestValueData::ConstructorCount);
  EXPECT_LONGS_EQUAL(8, (long)TestValueData::DestructorCount); // values destructed !
}

/* ************************************************************************* */
// small class with a constructor to create an rvalue
struct TestValues : Values {
  using Values::Values; // inherits move constructor

  TestValues(const TestValue& value1, const TestValue& value2) {
    insert(0, value1);
    insert(1, value2);
  }
};
static_assert(std::is_move_constructible<Values>::value, "");
static_assert(std::is_move_constructible<TestValues>::value, "");

// test move semantics
TEST(Values, move_constructor) {
  {
    TestValueData::ConstructorCount = 0;
    TestValueData::DestructorCount = 0;
    TestValue value1;
    TestValue value2;
    EXPECT_LONGS_EQUAL(2, (long)TestValueData::ConstructorCount);
    EXPECT_LONGS_EQUAL(0, (long)TestValueData::DestructorCount);
    TestValues values(TestValues(value1, value2)); // Move happens here ! (could be optimization?)
    EXPECT_LONGS_EQUAL(2, values.size());
    EXPECT_LONGS_EQUAL(6, (long)TestValueData::ConstructorCount); // yay ! We don't copy
    EXPECT_LONGS_EQUAL(2, (long)TestValueData::DestructorCount); // extra insert copies
  }
  EXPECT_LONGS_EQUAL(6, (long)TestValueData::ConstructorCount);
  EXPECT_LONGS_EQUAL(6, (long)TestValueData::DestructorCount);
}

// test use of std::move
TEST(Values, std_move) {
  {
    TestValueData::ConstructorCount = 0;
    TestValueData::DestructorCount = 0;
    {
      TestValue value1;
      TestValue value2;
      TestValues values(value1, value2);
      EXPECT_LONGS_EQUAL(6, (long)TestValueData::ConstructorCount);
      EXPECT_LONGS_EQUAL(2, (long)TestValueData::DestructorCount);
      EXPECT_LONGS_EQUAL(2, values.size());
      TestValues moved(std::move(values));   // Move happens here !
      EXPECT_LONGS_EQUAL(0, values.size());  // Should be 0 !
      EXPECT_LONGS_EQUAL(2, moved.size());
      EXPECT_LONGS_EQUAL(6, (long)TestValueData::ConstructorCount);  // Should be 6 :-)
      EXPECT_LONGS_EQUAL(2, (long)TestValueData::DestructorCount);   // extra insert copies
    }
    EXPECT_LONGS_EQUAL(6, (long)TestValueData::ConstructorCount);
    EXPECT_LONGS_EQUAL(6, (long)TestValueData::DestructorCount);
  }
}

/* ************************************************************************* */
TEST(Values, VectorDynamicInsertFixedRead) {
  Values values;
  Vector v(3); v << 5.0, 6.0, 7.0;
  values.insert(key1, v);
  Vector3 expected(5.0, 6.0, 7.0);
  Vector3 actual = values.at<Vector3>(key1);
  CHECK(assert_equal(expected, actual));
  CHECK_EXCEPTION(values.at<Vector7>(key1), exception);
}

/* ************************************************************************* */
TEST(Values, VectorDynamicInsertDynamicRead) {
  Values values;
  Vector v(3); v << 5.0, 6.0, 7.0;
  values.insert(key1, v);
  Vector expected(3); expected << 5.0, 6.0, 7.0;
  Vector actual = values.at<Vector>(key1);
  LONGS_EQUAL(3, actual.rows());
  LONGS_EQUAL(1, actual.cols());
  CHECK(assert_equal(expected, actual));
}

/* ************************************************************************* */
TEST(Values, VectorFixedInsertFixedRead) {
  Values values;
  Vector3 v; v << 5.0, 6.0, 7.0;
  values.insert(key1, v);
  Vector3 expected; expected << 5.0, 6.0, 7.0;
  Vector3 actual = values.at<Vector3>(key1);
  CHECK(assert_equal(expected, actual));
  CHECK_EXCEPTION(values.at<Vector7>(key1), exception);
}

/* ************************************************************************* */
// NOTE(frank): test is broken, because the scheme it tested was *very* slow.
// TODO(frank): find long-term solution. that works w matlab/python.
//TEST(Values, VectorFixedInsertDynamicRead) {
//  Values values;
//  Vector3 v; v << 5.0, 6.0, 7.0;
//  values.insert(key1, v);
//  Vector expected(3); expected << 5.0, 6.0, 7.0;
//  Vector actual = values.at<Vector>(key1);
//  LONGS_EQUAL(3, actual.rows());
//  LONGS_EQUAL(1, actual.cols());
//  CHECK(assert_equal(expected, actual));
//}

/* ************************************************************************* */
TEST(Values, MatrixDynamicInsertFixedRead) {
  Values values;
  Matrix v(1,3); v << 5.0, 6.0, 7.0;
  values.insert(key1, v);
  Vector3 expected(5.0, 6.0, 7.0);
  CHECK(assert_equal((Vector)expected, values.at<Matrix13>(key1)));
  CHECK_EXCEPTION(values.at<Matrix23>(key1), exception);
}

TEST(Values, Demangle) {
  Values values;
  Matrix13 v; v << 5.0, 6.0, 7.0;
  values.insert(key1, v);
  string expected = "Values with 1 values:\nValue v1: (Eigen::Matrix<double, 1, 3, 1, 1, 3>)\n[\n	5, 6, 7\n]\n\n";

  EXPECT(assert_print_equal(expected, values));
}

/* ************************************************************************* */
TEST(Values, brace_initializer) {
  const Pose2 poseA(1.0, 2.0, 0.3), poseC(.0, .0, .0);
  const Pose3 poseB(Pose2(0.1, 0.2, 0.3));

  {
    Values values;
    EXPECT_LONGS_EQUAL(0, values.size());
    values = { {key1, genericValue(1.0)} };
    EXPECT_LONGS_EQUAL(1, values.size());
    CHECK(values.at<double>(key1) == 1.0);
  }
  {
    Values values = { {key1, genericValue(poseA)}, {key2, genericValue(poseB)} };
    EXPECT_LONGS_EQUAL(2, values.size());
    EXPECT(assert_equal(values.at<Pose2>(key1), poseA));
    EXPECT(assert_equal(values.at<Pose3>(key2), poseB));
  }
  // Test exception: duplicated key:
  {
    Values values;
    CHECK_EXCEPTION((values = {
      {key1, genericValue(poseA)},
      {key2, genericValue(poseB)},
      {key1, genericValue(poseC)} 
      }), std::exception);
  }
}


/* ************************************************************************* */
int main() { TestResult tr; return TestRegistry::runAllTests(tr); }
/* ************************************************************************* */
