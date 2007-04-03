#include <cassert>
#include <vector>
#include <algorithm>
#include <CGAL/basic.h>
#include <CGAL/QP_models.h>
#include <CGAL/QP_functions.h>

// choose exact integral type
#ifndef CGAL_USE_GMP
#include <CGAL/MP_Float.h>
typedef CGAL::MP_Float ET;
#else
#include <CGAL/Gmpz.h>
typedef CGAL::Gmpz ET;
#endif

// program and solution types
typedef CGAL::Nonnegative_linear_program_from_pointers<int> Program;
typedef CGAL::Quadratic_program_solution<ET> Solution;

// we demonstrate the optimality certificate: if the
// linear program
// min c^T x 
//       A x <= b
//         x >= 0
// has an optimal solution x*, then there exists y (in fact an optimal
// solution of the dual linear program) such that
//         y >= 0
//    y^T A  >= -c^T,
//    y^T b   = -c^T x*
//
// In the following instance, the linear program has an optimal solution, 
// since it is feasible (take (1,0)) and bounded, due to the first 
// constraint:
// min  x_1 + 2x_2 
//      x_1 + 3x_2  <=  1
//     -x_1 +  x_2  <= -1
//      x_1,   x_2  >=  0

int main() {
  int  Ax1[] = { 1, -1};                        // column for x1
  int  Ax2[] = { 3,  1};                        // column for x2
  int*   A[] = {Ax1, Ax2};                      // A comes columnwise
  int    b[] = { 1, -1};                        // right-hand side
  CGAL::Comparison_result 
    r[] = {CGAL::SMALLER, CGAL::SMALLER};       // constraints are "<="
  int    c[] = { 1,  2};                        // objective function

  // now construct the linear program; the first two parameters are
  // the number of variables and the number of constraints (rows of A)
  Program lp (2, 2, A, b, r, c);

  // solve the program, using ET as the exact type
  Solution s = CGAL::solve_nonnegative_linear_program(lp, ET());

  // get certificate for optimality
  assert (s.status() == CGAL::QP_OPTIMAL);
  std::vector<CGAL::Quotient<ET> > x;
  std::copy (s.variable_values_begin(),
	     s.variable_values_end(), std::back_inserter(x));
  std::vector<CGAL::Quotient<ET> > y;
  std::copy (s.optimality_certificate_begin(), 
	     s.optimality_certificate_end(), std::back_inserter(y));
  
  // check y >= 0
  assert (y[0] >= 0);
  assert (y[1] >= 0);
  // check y^T A >= -c^T 
  assert (y[0] * A[0][0] + y[1] * A[0][1] >= -c[0]);
  assert (y[0] * A[1][0] + y[1] * A[1][1] >= -c[1]);
  // check y^T b = c^T x*
  assert (y[0] * b[0] + y[1] * b[1] == -c[0] * x[0] - c[1] * x[1]);

  return 0;
}
