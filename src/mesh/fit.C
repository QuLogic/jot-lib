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
 * fit.C:
 *
 *      Read a mesh from stdin (.sm format).
 *
 *      Treating it as the control mesh for a loop subdivision
 *      surface, compute new control point positions so that the limit
 *      location of each new control point exactly equals the original
 *      control point position. (Well, very nearly).
 *
 *      Write the result to stdout.
 *
 *      The default is to use Jacobi iteration. To select Gauss-Seidel
 *      iteration with -g flag. (For a description of these iterations
 *      see: G. H. Golub and C. F. van Loan. Matrtix Computations, 3rd
 *      Edition. Section 10.1.1, page 510.)
 **********************************************************************/
#include "std/stop_watch.H"
#include "mi.H"
#include "lmesh.H"

void
fit(LMESHptr& mesh, bool do_gauss_seidel)
{
   if (mesh->empty())
      return;

   // time this
   stop_watch clock;

   double max_err = mesh->get_bb().dim().length() * 1e-5;
   int n = mesh->nverts();

   // get original control point locations
   Wpt_list C(n);   // original control points
   Wpt_list L(n);   // current limit points
   for (int i=0; i<n; i++) {
      C += mesh->bv(i)->loc();
      L += Wpt::Origin();
   }

   // do 50 iterations...
   double prev_err = 0;
   for (int k=0; k<50; k++) {
      double err = 0;
      if (do_gauss_seidel) {
         // Gauss-Seidel iteration: use updated values from the
         // current iteration as they are computed...
         for (int j=0; j<n; j++) {
            // don't need that L[] array...
            Wpt limit;
            mesh->lv(j)->limit_loc(limit);
            Wvec delt = C[j] - limit;
            err += delt.length();
            mesh->bv(j)->offset_loc(delt);
         }
      } else {
         // compute the new offsets from the offsets computed in the
         // previous iteration
         int j;
         for (j=0; j<n; j++)
            mesh->lv(j)->limit_loc(L[j]);
         for (j=0; j<n; j++) {
            Wvec delt = C[j] - L[j];
            err += delt.length();
            mesh->bv(j)->offset_loc(delt);

         }
      }
      // compute the average error:
      err /= n;
      if (prev_err != 0) {
         err_msg("Iter %d: avg error: %f, reduction: %f",
                 k, err, err/prev_err);
      } else {
         err_msg("Iter %d: avg error: %f", k, err);
      }
      prev_err = err;
      if (err < max_err)
         break;
   }

   err_msg("fitting took %.2f seconds", clock.elapsed_time());
}

int 
main(int argc, char *argv[])
{
   bool do_gauss_seidel = 0;

   if (argc == 2 && str_ptr(argv[1]) == str_ptr("-g"))
      do_gauss_seidel = 1;
   else if(argc != 1)
   {
      err_msg("Usage: %s [ -g ] < mesh.sm > mesh-fit.sm", argv[0]);
      return 1;
   }

   LMESHptr mesh = LMESH::read_jot_stream(cin);
   if (!mesh || mesh->empty())
      return 1; // didn't work

   fit(mesh, do_gauss_seidel);

   mesh->write_stream(cout);

   return 0;
}

/* end of file fit.C */
