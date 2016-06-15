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
#include "decal_stroke_pool.H"
#include "gtex/gl_extensions.H"

void
DecalStrokePool::draw_flat(CVIEWptr& v) {


   //This really fails with noise... so...
   assert(get_num_protos()==1);

   if (_hide) return;
   OutlineStroke *prot = get_active_prototype();
   if (!prot) return;

   // A OutlineStroke::draw_start() call has a different effect depending
   // on whether or not the stroke has texture.  So textured and
   // untextured strokes must be rendered in different passes.

   // Check whether we have textured or untextured strokes, or both
   // kinds of strokes.

   bool have_textured_strokes = false;
   bool have_untextured_strokes = false;
   int i;
   for (i = 0; i < _num_strokes_used; i++) {
      if (at(i)->get_texture())
         have_textured_strokes = true;
      else
         have_untextured_strokes = true;
   }

   if (have_untextured_strokes) {

      // If proto has a texture, temporarily unset the texture so it
      // calls draw_start() appropriately for untextured strokes.

      TEXTUREptr proto_tex = prot->get_texture();
      const string proto_tex_file = prot->get_texture_file();

      if (prot->get_texture()) {
         prot->set_texture(nullptr, "");
      }

      prot->draw_start();

      glDepthFunc(GL_ALWAYS);
      for (i = 0; i < _num_strokes_used; i++) {
         if (!at(i)->get_texture())
            at(i)->draw(v);
      }
         
      glDepthFunc(GL_LESS);

      prot->draw_end();

      // If we saved the proto's texture, reset it now.
      if (proto_tex){      
         prot->set_texture(proto_tex, proto_tex_file);
      }

   }
   
   if (have_textured_strokes) {

      // Have each textured stroke call its own draw_begin().
      // XXX -- this is a little inefficient.

      for (int j = 0; j < _num_strokes_used; j++) {
         if (at(j)->get_texture()) {
            at(j)->draw_start();
            glDepthFunc(GL_ALWAYS);
            at(j)->draw(v);
            glDepthFunc(GL_LESS);
            at(j)->draw_end();
         }
      }
   }
}
