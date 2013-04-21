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
#include "pattern/eigen_solver.H"

using namespace mlib;

void
EigenSolver::solve(VEXEL& vec1, double& val1, VEXEL& vec2, double& val2) const{
  // compute svd

  double* diag = new double[2];
  double* subd = new double[2];
  double** kmat = new double*[2];
  kmat[0] = new double[2];
  kmat[1] = new double[2];
  compute_svd(diag, subd, kmat);
  vec1[0] = kmat[0][0];
  vec1[1] = kmat[1][0];
  vec2[0] = kmat[0][1];
  vec2[1] = kmat[1][1];
  val1 = diag[0];
  val2 = diag[1];

  // sort eigenvalues
  if (val1<val2){
    double tmp_val = val1;
    VEXEL tmp_vec = vec1;
    val1 = val2;
    vec1 = vec2;
    val2 = tmp_val;
    vec2 = tmp_vec;
  }
}


bool
EigenSolver::compute_svd(double* diag, double* subd, double** kmat) const{
  diag[0] = _cov_xx;
  diag[1] = _cov_yy;
  subd[0] = _cov_xy;
  subd[1] = 0.0;
  kmat[0][0] = kmat[1][1] = 1.0;
  kmat[0][1] = kmat[1][0] = 0.0;
  
  const int iMaxIter = 32;
  const int dims = 2;
  
  for (int i0 = 0; i0 < dims; i0++) {
    int i1;
    for (i1 = 0; i1 < iMaxIter; i1++){
      int i2;
      for (i2 = i0; i2 <= dims-2; i2++) {
	double f_tmp = abs(diag[i2]) + abs(diag[i2+1]);
	if ( abs(subd[i2]) + f_tmp == f_tmp )
	  break;
      }
      if (i2 == i0) {
	break;
      }
      
      double fG = (diag[i0+1] - diag[i0])/(2.0*subd[i0]);
      double fR = sqrt(fG*fG+1.0);
      if (fG < 0.0) {
	fG = diag[i2]-diag[i0]+subd[i0]/(fG-fR);
      } else {
	fG = diag[i2]-diag[i0]+subd[i0]/(fG+fR);
      }
      double fSin = 1.0, fCos = 1.0, fP = 0.0;
      for (int i3 = i2-1; i3 >= i0; i3--) {
	double fF = fSin*subd[i3];
	double fB = fCos*subd[i3];
	if (abs(fF) >= abs(fG)) {
	  fCos = fG/fF;
	  fR = sqrt(fCos*fCos+1.0);
	  subd[i3+1] = fF*fR;
	  fSin = 1.0/fR;
	  fCos *= fSin;
	} else {
	  fSin = fF/fG;
	  fR = sqrt(fSin*fSin+1.0);
	  subd[i3+1] = fG*fR;
	  fCos = 1.0/fR;
	  fSin *= fCos;
	}
	fG = diag[i3+1]-fP;
	fR = (diag[i3]-fG)*fSin+2.0*fB*fCos;
	fP = fSin*fR;
	diag[i3+1] = fG+fP;
	fG = fCos*fR-fB;
	
	for (int i4 = 0; i4 < dims; i4++) {
	  fF = kmat[i4][i3+1];
	  kmat[i4][i3+1] = fSin*kmat[i4][i3]+fCos*fF;
	  kmat[i4][i3] = fCos*kmat[i4][i3]-fSin*fF;
	}
      }
      diag[i0] -= fP;
      subd[i0] = fG;
      subd[i2] = 0.0;
    }
        
    if (i1 == iMaxIter) {
      return false;
    }
  }
  
  return true;  
}
