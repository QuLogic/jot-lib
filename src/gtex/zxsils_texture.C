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
 * sils_texture.C
 **********************************************************************/

#include "std/config.H"
#include "geom/gl_view.H"
#include "zxsils_texture.H"

using mlib::Wpt;

/**********************************************************************
 * ZcrossTexture:
 **********************************************************************/
int
ZcrossTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);

   // Ensure zcross strips are current, and get them
   _patch->mesh()->build_zcross_strips();

   // Get a reference for low-overhead:
   //const ZcrossPath& zx_sils = _patch->zx_sils();

   //needs to have a current sils (simon)
   const ZcrossPath& zx_sils = _patch->cur_zx_sils();
   // Stop now, before making partial GL calls, if nothing is going on
   if (zx_sils.empty())
      return 1;

   // Set line width, (optionally) enable antialiasing, and save GL state
   GLfloat w = float(v->line_scale()*_width);
   static bool antialias = Config::get_var_bool("ANTIALIAS_SILS",true,true);
   static bool drawback = Config::get_var_bool("DRAW_BACKFACING",false,true);
   if (antialias) {
      // push attributes, enable line smoothing, and set line width
      GL_VIEW::init_line_smooth(w, GL_CURRENT_BIT);
   } else {
      // push state, set line width
      glPushAttrib(GL_LINE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
      glLineWidth(w); // GL_LINE_BIT
   }

   glDisable(GL_BLEND);
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT
   GL_COL(_color, alpha());     // GL_CURRENT_BIT


   // right now we're only doing OpenGL, ignoring CB
   int n = zx_sils.num();
   bool started = false;
   bool lvis = false;
   bool vis = false;

   static bool nodots = Config::get_var_bool("ZX_NO_DOTS",false,true);
   static bool nocolor = Config::get_var_bool("ZX_NO_COLOR",false,true);
   //XXX - Hack. Not good when a loop isn't closed.
   // But I'm in a hurry to close proper loops...
   static bool closed = Config::get_var_bool("ZX_CLOSED",false,true);

   srand48(0);
   if (!nocolor)
      glColor3d ( drand48(), drand48(), drand48() );
   else
      glColor3d ( 0.0, 0.0, 0.0 );

   Wpt wp;

   if (!nodots) {
      glPointSize(5.0);
      glBegin ( GL_POINTS ) ;
      for ( int k =0 ; k < n; k++ ) {
         glColor3d ( 0.0 , 0.0 , 1.0 );
         if ( zx_sils.face(k) == NULL )
            glColor3d ( 1.0, 0, 0 );
         glVertex3dv ( zx_sils.point(k).data() );
      }
      glEnd();
   }
   if ( !drawback ) {
      for (int i=0; i < n; i++) {

         vis = zx_sils.grad(i);

         // start new line strip if needed:
         if ( vis ) {
            //we are in a visible section of the curve
            if (!started ) {
               if ( zx_sils.face(i) ) {
                  glBegin(GL_LINE_STRIP);
                  zx_sils.face(i)->bc2pos ( zx_sils.bc(i) , wp );
                  glVertex3dv( wp.data());
                  started = true;
               }
            } else {
               if ( zx_sils.face(i) && i < n-1 ) {
                  zx_sils.face(i)->bc2pos ( zx_sils.bc(i) , wp );
                  glVertex3dv( wp.data());
               } else {
                  zx_sils.face(i-1)->bc2pos ( zx_sils.bc(i) , wp );
                  glVertex3dv( wp.data());
                  glEnd();
                  started = false;
               }
            }
         } else if ( lvis && started ) {
            //this point is not visible, but the last one was.
            zx_sils.face(i-1)->bc2pos ( zx_sils.bc(i-1) , wp );
            glVertex3dv( wp.data());
            glEnd();
            started=false;
         }
         lvis = vis;
         //if the face was null, we started a new loop
         if ( !zx_sils.face(i) ) {
            if (!nocolor)
               glColor3d ( drand48(), drand48(), drand48() );
            else
               glColor3d ( 0.0, 0.0, 0.0);
         }
      }
   } else {  //draw using barycentric values
      Wpt first;
      for (int i=0; i < n; i++) {
         vis = true;
         if ( vis ) {
            //we are in a visible section of the curve
            if (!started ) {
               if ( zx_sils.face(i) ) {
                  glBegin(GL_LINE_STRIP);
                  zx_sils.face(i)->bc2pos ( zx_sils.bc(i) , wp );
                  if (!nocolor) {
                     if ( !zx_sils.grad(i) )
                        glColor3d( 1.0, 0.0 ,0.0 );
                     else
                        glColor3d( 0.0 , 0.0, 1.0 );
                  } else {
                     glColor3d( 0.0 , 0.0, 0.0 );
                  }
                  first = wp;
                  glVertex3dv( wp.data());
                  started = true;
               }
            } else {
               if (!nocolor) {
                  if ( !zx_sils.grad(i) )
                     glColor3d( 1.0, 0.0 ,0.0 );
                  else
                     glColor3d( 0.0 , 0.0, 1.0 );
               } else
                  glColor3d( 0.0 , 0.0, 0.0 );

               if ( zx_sils.face(i) && i < n-1 ) {
                  zx_sils.face(i)->bc2pos ( zx_sils.bc(i) , wp );
                  glVertex3dv( zx_sils.point(i).data());
               } else {
                  zx_sils.face(i-1)->bc2pos ( zx_sils.bc(i) , wp );
                  glVertex3dv( zx_sils.point(i-1).data());
                  if (closed)
                     glVertex3dv( first.data());
                  glEnd();
                  started = false;
               }
            }
         } else if ( lvis && started ) {
            //this point is not visible, but the last one was.
            zx_sils.face(i-1)->bc2pos ( zx_sils.bc(i-1) , wp );
            glVertex3dv( zx_sils.point(i-1).data());
            glEnd();
            started=false;
         }
         lvis = vis;
      }
   }

   // Restore GL state:
   if (antialias)
      GL_VIEW::end_line_smooth();
   else
      glPopAttrib();

   return 1;
}

// end of file sils_texture.C
