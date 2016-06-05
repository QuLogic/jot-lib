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
#ifndef LINEAR_SYS_H_IS_INCLUDED
#define LINEAR_SYS_H_IS_INCLUDED

/*!
 *  \file linear_sys.H
 *  \brief Solution of systems of linear equations and eigenvalue decomposition.
 *  Some are patterned after the code in Numerical Recipes, but with a bit of
 *  the fancy C++ template thing happening.
 *
 *  \note This code is taken almost verbatim from the TriMesh2 library written by
 *  Szymon Rusinkiewicz of Princeton University
 *  (http://www.cs.princeton.edu/gfx/proj/trimesh2/).
 *
 */

#include <cmath>
#include <algorithm>

namespace mlib {

#ifndef likely
#  if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#    define likely(x) (x)
#    define unlikely(x) (x)
#  else
#    define likely(x)   (__builtin_expect((x), 1))
#    define unlikely(x) (__builtin_expect((x), 0))
#  endif
#endif

//! \brief LU decomposition.
template <class T, int N>
inline bool
ludcmp(T a[N][N], int indx[N], T *d = nullptr)
{
   int i, j, k;
   T vv[N];

   if (d)
      *d = 1;
   for (i = 0; i < N; i++) {
      T big = 0.0;
      for (j = 0; j < N; j++) {
         T tmp = std::fabs(a[i][j]);
         if (tmp > big)
            big = tmp;
      }
      if (big == 0.0)
         return false;
      vv[i] = 1.0 / big;
   }
   for (j = 0; j < N; j++) {
      for (i = 0; i < j; i++) {
         T sum = a[i][j];
         for (k = 0; k < i; k++)
            sum -= a[i][k]*a[k][j];
         a[i][j]=sum;
      }
      T big = 0.0;
      int imax = j;
      for (i = j; i < N; i++) {
         T sum = a[i][j];
         for (k = 0; k < j; k++)
            sum -= a[i][k]*a[k][j];
         a[i][j] = sum;
         T tmp = vv[i] * std::fabs(sum);
         if (tmp > big) {
            big = tmp;
            imax = i;
         }
      }
      if (imax != j) {
         for (k = 0; k < N; k++)
            std::swap(a[imax][k], a[j][k]);
         if (d)
            *d = -(*d);
         vv[imax] = vv[j];
      }
      indx[j] = imax;
      if (a[j][j] == 0.0)
         return false;
      if (j != N-1) {
         T tmp = 1.0/(a[j][j]);
         for (i = j+1; i < N; i++)
            a[i][j] *= tmp;
      }
   }
   return true;
}


//! \brief Backsubstitution after ludcmp().
template <class T, int N>
inline void
lubksb(T a[N][N], int indx[N], T b[N])
{
   int ii = -1, i, j;
   for (i = 0; i < N; i++) {
      int ip = indx[i];
      T sum = b[ip];
      b[ip] = b[i];
      if (ii != -1)
         for (j = ii; j < i; j++)
            sum -= a[i][j] * b[j];
      else if (sum)
         ii = i;
      b[i] = sum;
   }
   for (i = N-1; i >= 0; i--) {
      T sum = b[i];
      for (j = i+1; j < N; j++)
         sum -= a[i][j] * b[j];
      b[i] = sum / a[i][i];
   }
}


//! \brief Perform LDL^T decomposition of a symmetric positive definite matrix.
//! 
//! Like Cholesky, but no square roots.  Overwrites lower triangle of matrix.
template <class T, int N>
inline bool
ldltdc(T A[N][N], T rdiag[N])
{
   T v[N-1];
   for (int i = 0; i < N; i++) {
      for (int k = 0; k < i; k++)
         v[k] = A[i][k] * rdiag[k];
      for (int j = i; j < N; j++) {
         T sum = A[i][j];
         for (int k = 0; k < i; k++)
            sum -= v[k] * A[j][k];
         if (i == j) {
            if (unlikely(sum <= T(0)))
               return false;
            rdiag[i] = T(1) / sum;
         } else {
            A[j][i] = sum;
         }
      }
   }

   return true;
}


//! \brief Solve Ax=B after ldltdc().
template <class T, int N>
inline void
ldltsl(T A[N][N], T rdiag[N], T B[N], T x[N])
{
   int i;
   for (i = 0; i < N; i++) {
      T sum = B[i];
      for (int k = 0; k < i; k++)
         sum -= A[i][k] * x[k];
      x[i] = sum * rdiag[i];
   }
   for (i = N - 1; i >= 0; i--) {
      T sum = 0;
      for (int k = i + 1; k < N; k++)
         sum += A[k][i] * x[k];
      x[i] -= sum * rdiag[i];
   }
}


//! \brief Eigenvector decomposition for real, symmetric matrices,
//! a la Bowdler et al. / EISPACK / JAMA.
//! 
//! Entries of d are eigenvalues, sorted smallest to largest.
//! A changed in-place to have its columns hold the corresponding eigenvectors.
//! \note A must be completely filled in on input.
template <int N, class T>
inline void
eigdc(T A[N][N], T d[N])
{
   // Householder
   T e[N];
   for (int j = 0; j < N; j++) {
      d[j] = A[N-1][j];
      e[j] = 0.0;
   }
   for (int i = N-1; i > 0; i--) {
      T scale = 0.0;
      for (int k = 0; k < i; k++)
         scale += std::fabs(d[k]);
      if (scale == 0.0) {
         e[i] = d[i-1];
         for (int j = 0; j < i; j++) {
            d[j] = A[i-1][j];
            A[i][j] = A[j][i] = 0.0;
         }
         d[i] = 0.0;
      } else {
         T h = 0.0;
         T invscale = 1.0 / scale;
         for (int k = 0; k < i; k++) {
            d[k] *= invscale;
            h += sqr(d[k]);
         }
         T f = d[i-1];
         T g = (f > 0.0) ? -std::sqrt(h) : std::sqrt(h);
         e[i] = scale * g;
         h -= f * g;
         d[i-1] = f - g;
         for (int j = 0; j < i; j++)
            e[j] = 0.0;
         for (int j = 0; j < i; j++) {
            f = d[j];
            A[j][i] = f;
            g = e[j] + f * A[j][j];
            for (int k = j+1; k < i; k++) {
               g += A[k][j] * d[k];
               e[k] += A[k][j] * f;
            }
            e[j] = g;
         }
         f = 0.0;
         T invh = 1.0 / h;
         for (int j = 0; j < i; j++) {
            e[j] *= invh;
            f += e[j] * d[j];
         }
         T hh = f / (h + h);
         for (int j = 0; j < i; j++)
            e[j] -= hh * d[j];
         for (int j = 0; j < i; j++) {
            f = d[j];
            g = e[j];
            for (int k = j; k < i; k++)
               A[k][j] -= f * e[k] + g * d[k];
            d[j] = A[i-1][j];
            A[i][j] = 0.0;
         }
         d[i] = h;
      }
   }

   for (int i = 0; i < N-1; i++) {
      A[N-1][i] = A[i][i];
      A[i][i] = 1.0;
      T h = d[i+1];
      if (h != 0.0) {
         T invh = 1.0 / h;
         for (int k = 0; k <= i; k++)
            d[k] = A[k][i+1] * invh;
         for (int j = 0; j <= i; j++) {
            T g = 0.0;
            for (int k = 0; k <= i; k++)
               g += A[k][i+1] * A[k][j];
            for (int k = 0; k <= i; k++)
               A[k][j] -= g * d[k];
         }
      }
      for (int k = 0; k <= i; k++)
         A[k][i+1] = 0.0;
   }
   for (int j = 0; j < N; j++) {
      d[j] = A[N-1][j];
      A[N-1][j] = 0.0;
   }
   A[N-1][N-1] = 1.0;

   // QL
   for (int i = 1; i < N; i++)
      e[i-1] = e[i];
   e[N-1] = 0.0;
   T f = 0.0, tmp = 0.0;
   const T eps = pow(2.0, -52.0);
   for (int l = 0; l < N; l++) {
      tmp = std::max(tmp, std::fabs(d[l]) + std::fabs(e[l]));
      int m = l;
      while (m < N) {
         if (std::fabs(e[m]) <= eps * tmp)
            break;
         m++;
      }
      if (m > l) {
         do {
            T g = d[l];
            T p = (d[l+1] - g) / (e[l] + e[l]);
            T r = hypot(p, 1.0);
            if (p < 0.0)
               r = -r;
            d[l] = e[l] / (p + r);
            d[l+1] = e[l] * (p + r);
            T dl1 = d[l+1];
            T h = g - d[l];
            for (int i = l+2; i < N; i++)
               d[i] -= h; 
            f += h;
            p = d[m];   
            T c = 1.0, c2 = 1.0, c3 = 1.0;
            T el1 = e[l+1], s = 0.0, s2 = 0.0;
            for (int i = m - 1; i >= l; i--) {
               c3 = c2;
               c2 = c;
               s2 = s;
               g = c * e[i];
               h = c * p;
               r = hypot(p, e[i]);
               e[i+1] = s * r;
               s = e[i] / r;
               c = p / r;
               p = c * d[i] - s * g;
               d[i+1] = h + s * (c * g + s * d[i]);
               for (int k = 0; k < N; k++) {
                  h = A[k][i+1];
                  A[k][i+1] = s * A[k][i] + c * h;
                  A[k][i] = c * A[k][i] - s * h;
               }
            }
            p = -s * s2 * c3 * el1 * e[l] / dl1;
            e[l] = s * p;
            d[l] = c * p;
         } while (std::fabs(e[l]) > eps * tmp);
      }
      d[l] += f;
      e[l] = 0.0;
   }

   // Sort
   for (int i = 0; i < N-1; i++) {
      int k = i;
      T p = d[i];
      for (int j = i+1; j < N; j++) {
         if (d[j] < p) {
            k = j;
            p = d[j];
         }
      }
      if (k == i)
         continue;
      d[k] = d[i];
      d[i] = p;
      for (int j = 0; j < N; j++) {
         p = A[j][i];
         A[j][i] = A[j][k];
         A[j][k] = p;
      }
   }
}


//! \brief x <- A * d * A' * b
template <int N, class T>
inline void
eigmult(T A[N][N], T d[N], T b[N], T x[N])
{
   T e[N];
   for (int i = 0; i < N; i++) {
      e[i] = 0.0;
      for (int j = 0; j < N; j++)
         e[i] += A[j][i] * b[j];
      e[i] *= d[i];
   }
   for (int i = 0; i < N; i++) {
      x[i] = 0.0;
      for (int j = 0; j < N; j++)
         x[i] += A[i][j] * e[j];
   }
}

} // namespace mlib

#endif // LINEAR_SYS_H_IS_INCLUDED
