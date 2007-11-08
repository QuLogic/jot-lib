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
 * png2sm.C:
 * 
 *   convert from png heightmap to mesh
 *
 **********************************************************************/
#include "geom/image.H"
#include "std/config.H"
#include "mesh/mi.H"

static bool debug    = Config::get_var_bool("DEBUG_PNG2SM",false,true);
static bool make_box = Config::get_var_bool("PNG2SM_MAKE_BOX",false,true);

static double yscale = Config::get_var_dbl("PNG_YSCALE",0.16,true);

int 
main(int argc, char *argv[])
{
   if (argc != 2) {
      err_msg("Usage: %s heightmap.png > mesh.sm", argv[0]);
      return 1;
   }

   Image img;
   if (!img.read_png(argv[1])) {
      err_msg("png2sm: can't read PNG file from stdin");
      return 1;
   }

   err_adv(debug, "read %dx%d png", img.width(), img.height());
   
   BMESHptr mesh = new LMESH;

   uint x, z;

   ARRAY<Bvert_list> top_verts(img.height());

   // create verts
   for (z=0; z<img.height(); z++) {
      top_verts += Bvert_list();
      for (x=0; x<img.width(); x++) {
         top_verts.last() += mesh->add_vertex(Wpt(x,img.pixel(x,z),z));
      }
   }

   // create faces:
   for (z=1; z<img.height(); z++) {
      for (x=1; x<img.width(); x++) {
         mesh->add_quad(top_verts[z][x-1], top_verts[z][x],
                        top_verts[z-1][x], top_verts[z-1][x-1]);
      }
   }
   
   if (make_box) {
      ARRAY<Bvert_list> bot_verts(img.height());

      // create verts
      for (z=0; z<img.height(); z++) {
         bot_verts += Bvert_list();
         for (x=0; x<img.width(); x++) {
            bot_verts.last() += mesh->add_vertex(Wpt(x,-1,z));
         }
      }
      // create faces:
      for (z=1; z<img.height(); z++) {
         for (x=1; x<img.width(); x++) {
            mesh->add_quad(bot_verts[z][x-1], bot_verts[z-1][x-1],
                           bot_verts[z-1][x], bot_verts[z][x]);
         }
      }

      // top and bottom ribbons
      for (x=1; x<img.width(); x++) {
            mesh->add_quad(top_verts[0][x-1], top_verts[0][x],
                           bot_verts[0][x], bot_verts[0][x-1]);
      }
      uint n = img.height()-1;
      for (x=1; x<img.width(); x++) {
            mesh->add_quad(bot_verts[n][x-1], bot_verts[n][x],
                           top_verts[n][x], top_verts[n][x-1]);
      }
      // right and left ribbons
      n = img.width()-1;
      for (z=1; z<img.height(); z++) {
            mesh->add_quad(top_verts[z-1][n], top_verts[z][n],
                           bot_verts[z][n], bot_verts[z-1][n]);
      }
      for (z=1; z<img.height(); z++) {
            mesh->add_quad(bot_verts[z-1][0], bot_verts[z][0],
                           top_verts[z][0], top_verts[z-1][0]);
      }
   }

   // recenter:
   mesh->transform(
      Wtransf::translation(Wvec(-0.5*img.width(), 0.0, -0.5*img.height()))
      );

   // rescale height to something reasonable:
   mesh->transform(Wtransf::scaling(1, yscale, 1));

   // uniformly rescale to something likely to be ok for
   // standard jot camera:
   mesh->transform(Wtransf::scaling(0.5));

   if (debug || Config::get_var_bool("JOT_PRINT_MESH"))
      mesh->print();

   mesh->write_stream(cout);

   return 0;
}
