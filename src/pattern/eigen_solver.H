/*****************************************************************
 * This file is part of jot-lib (or "jot" for short):
 *   <http://code.google.com/p/jot-lib/>
 * 
 * jot-lib is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * jot-lib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with jot-lib.  If not, see <http://www.gnu.org/licenses/>.`
 *****************************************************************/
#ifndef EIGEN_SOLVER_H
#define EIGEN_SOLVER_H

#include "mlib/points.H"

class EigenSolver{
public:
  EigenSolver(double cov_xx, double cov_xy, double cov_yy) 
    : _cov_xx(cov_xx), _cov_xy(cov_xy), _cov_yy(cov_yy) {}

  void solve(mlib::VEXEL& vec1, double& val1, mlib::VEXEL& vec2, double& val2) const;

private:
  bool compute_svd(double* diag, double* subd, double** kmat) const;
  

private:
  double _cov_xx;
  double _cov_xy;
  double _cov_yy;
};

#endif // EIGEN_SOLVER_H
