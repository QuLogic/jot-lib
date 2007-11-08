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
/**********************************************************************
 * align.C:
 *
 *      read an .sm file, transform it so its principal directions
 *      align with world X, Y, Z axes.
 *
 **********************************************************************/
#include "std/config.H"
#include "mi.H"

inline double 
cross_mult(int i, int j, CARRAY<Wvec>& v)
{
   double ret = 0;
   for (int k=0; k<v.num(); k++)
      ret += v[k][i] * v[k][j];
   return ret;
}

Wvec
sym_mat_eigenvalues(const WMat3& A)
{
   // Algorithm from "Eigenvalues of a symmetric 3x3 matrix"
   //   by Oliver K. Smith
   //   http://portal.acm.org/citation.cfm?doid=355578.366316

   if (!A.is_symmetric()) {
      cerr << "sym_mat_eigenvalues: error: matrix is not symmetric:"
           << endl
           << A
           << endl;
      return Wvec();
   }

   // m is 1/3 of the trace of A
   double m = A.trace()/3;

   cerr << "m: " << m
        << endl;

   // B = (A - Im)
   WMat3 B = A - WMat3()*m;

   cerr << "B: "
        << endl
        << B
        << endl;

   // q is 1/2 the determinant of B
   double q = B.det()/2;

   cerr << "q: " << q
        << endl;

   // p is 1/6 the sum of squares of B
   double p = (B[0].length_sqrd() +
               B[1].length_sqrd() +
               B[2].length_sqrd())/6;

   cerr << "p: " << p
        << endl;

   double theta = atan(sqrt(fabs(pow(p,3) - sqr(q)))/q)/3;

   cerr << "theta: " << theta
        << endl;

   // compute eigenvalues:
   Wvec l; 
   l[0] = m + 2*sqrt(p)*cos(theta);
   l[1] = m - sqrt(p)*(cos(theta) + sqrt(3)*sin(theta));
   l[2] = m - sqrt(p)*(cos(theta) - sqrt(3)*sin(theta));

   // they should all be positive
   assert(l[0] > -epsAbsMath() &&
          l[1] > -epsAbsMath() &&
          l[2] > -epsAbsMath());
   l[0] = max(l[0],0.0);
   l[1] = max(l[1],0.0);
   l[2] = max(l[2],0.0);

   // set near-zero values to zero:
   if (isZero(l[0])) l[0] = 0;
   if (isZero(l[1])) l[1] = 0;
   if (isZero(l[2])) l[2] = 0;

   // re-order so l[0] >= l[1] >= l[2]:
   if (l[0]<l[1]) swap(l[0],l[1]);
   if (l[0]<l[2]) swap(l[0],l[2]);
   if (l[1]<l[2]) swap(l[1],l[2]);

   // set near-equal values equal:
   if (isEqual(l[0],l[1])) l[1] = l[0];
   if (isEqual(l[1],l[2])) l[2] = l[1];
   
   cerr << "eigenvalues: "
        << l[0] << ", " << l[1] << ", " << l[2]
        << endl;

   return l;
}

inline Wvec
kernel_vec(const WMat3& M)
{
   // return a vector perpendicular to all 3 rows

   // only supposed to call this on a singular matrix
   if (fabs(M.det()) > 1e-5) {
      cerr << "kernel_vec: warning: matrix is not singular:"
           << endl
           << M
           << endl
           << "determinant: "
           << M.det()
           << endl;
      return Wvec();
   }

   // get row vectors, changed to unit length or null:
   Wvec r0 = M.row(0).normalized();
   Wvec r1 = M.row(1).normalized();
   Wvec r2 = M.row(2).normalized();

   // re-order to push null ones to the end:
   if (r0.is_null()) swap(r0,r1);
   if (r0.is_null()) swap(r0,r2);
   if (r1.is_null()) swap(r1,r2);

   Wvec ret = cross(r0,r1).normalized();
   if (ret.is_null())
      ret = cross(r0,r2).normalized();
   if (ret.is_null())
      ret = Wvec::X();
   assert(isZero(ret*r0) && isZero(ret*r1) && isZero(ret*r2));
   return ret;
}

WMat3
sym_mat_eigenvectors(const WMat3& A)
{
   // returns eigenvectors as rows of the returned matrix

   Wvec  l = sym_mat_eigenvalues(A);
   WMat3 I = WMat3::Identity();

   // all eigenvalues equal -- every direction is an eigenvector:
   if (l[0] == l[1] && l[0] == l[2])
      return I;

   Wvec X, Y, Z;

   if (l[0] == l[1]) {
      // 1st two eigenvalues equal:
      //   first 2 eigenvectors must be perpendicular to 3rd
      Z = kernel_vec(A - I*l[2]); // 3rd eigenvector
      Y = Z.perpend();
      X = cross(Y,Z);
   } else if (l[1] == l[2]) {
      // last two eigenvalues equal:
      //   last 2 eigenvectors must be perpendicular to first
      X = kernel_vec(A - I*l[0]); // 1st eigenvector
      Y = X.perpend();
      Z = cross(X,Y);
   } else {
      // all 3 are different
      X = kernel_vec(A - I*l[0]);
      Y = kernel_vec(A - I*l[1]);
      Z = kernel_vec(A - I*l[2]);
   }

   if (cross(X,Y)*Z < 0) {
      cerr << "fixing order to be right-handed" << endl;
      swap(Y,Z);
      assert(cross(X,Y)*Z > 0);
   }

   return WMat3(X,Y,Z);
}

Wtransf
compute_xf(CARRAY<Wvec>& v)
{
   WMat3 A;

   // build the symmetric matrix:
   double s = 0;
   for (int i=0; i<3; i++) {
      for (int j=i; j<3; j++) {
         A[i][j] = A[j][i] = cross_mult(i,j,v);
         s = max(s, fabs(A[i][j]));
      }
   }
   // divide by largest value:
   if (s > 1e-5)
      A = A/s;

   cerr << "A: "
        << endl
        << A
        << endl;

   WMat3 V = sym_mat_eigenvectors(A);

   // this just handles the rotation...
   return Wtransf(V.row(0),V.row(1),V.row(2)).transpose();
}


Wtransf
compute_xf(CWpt_list& p)
{
   Wpt    c = p.average();
   double s = p.spread();

   ARRAY<Wvec> v(p.num());
   for (int i=0; i<p.num(); i++) {
      v += (p[i] - c)/s;
   }
   assert(v.num() == p.num());
   return Wtransf::translation(c) * compute_xf(v) * Wtransf::translation(-c);
}

int 
main(int argc, char *argv[])
{
   if (argc != 1) {
      cerr << "Usage: " << argv[0] << " < input.sm > output.sm" << endl;
      return 1;
   }

   BMESHptr mesh = BMESH::read_jot_stream(cin);
   if (!mesh || mesh->empty())
      return 1; // didn't work

   Wtransf xf = compute_xf(mesh->verts().pts());

   MOD::tick();
   mesh->transform(xf, MOD());

   mesh->write_stream(cout);

   return 0;
}

// end of file align.C
