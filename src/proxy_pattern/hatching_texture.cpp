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
#include "proxy_stroke.hpp"
#include "proxy_texture.hpp"
#include "hatching_texture.hpp"
#include "npr/hatching_group_base.hpp"  //for smooth_gesture
#include "mesh/bfilters.hpp"

/**********************************************************************
* ProxyTexture:
**********************************************************************/
HatchingTexture::HatchingTexture(Patch* patch) :
BasicTexture(patch, new GLStripCB),
_proxy_texture(0),
_draw_proxy_mesh(false)
{

}

HatchingTexture::~HatchingTexture()
{  
}

void
HatchingTexture::set_patch(Patch* p) {
   GTexture::set_patch(p);  

}

void
HatchingTexture::set_proxy_texture(ProxyTexture* pt)
{
   _proxy_texture = pt;
}  




int
HatchingTexture::draw(CVIEWptr& v)
{
   if(!_proxy_texture || !_proxy_texture->proxy_surface())
      return 0;

   if(!patch())
      return 0;
   // Set up for drawing in NDCZ coords:
   draw_start();
   
   CBface_list& faces = patch()->mesh()->faces();
  // cerr << "faces has " << faces.num() << endl;
   for (Bface_list::size_type i=0; i < faces.size(); ++i) {
      if(faces[i]->is_quad_rep()){
      for (uint j=0; j < _strokes.size(); ++j) {
               //ProxyStroke* rendered_stroke = new ProxyStroke();
               //double w = 4.0;
               //w *= (_proxy_texture->patch()->get_z_accum().length()); 

               //rendered_stroke->set_width(w);

               _strokes[j]->stroke_setup(faces[i],_draw_proxy_mesh);
               
               if(j==0) _strokes[j]->draw_start();
               _strokes[j]->draw(v);
               if(j==( _strokes.size()-1)) _strokes[j]->draw_end();
               
               //delete rendered_stroke;

               //start_strokes.push_back(strokes[j]);					
               //	connect_draw(strokes[j], j, faces[i]);
      }
      }
      if(_draw_proxy_mesh){
         glPushAttrib(GL_CURRENT_BIT |GL_ENABLE_BIT);
         GL_VIEW::init_line_smooth(3.0, GL_CURRENT_BIT);
         GL_COL(Color::blue3,1.0);         // GL_CURRENT_BIT
       
         Bvert_list verts;
         faces[i]->get_quad_verts(verts);
         glBegin(GL_LINE_STRIP);   
         glVertex2dv(((verts[0])->ndc()).data());
         glVertex2dv(((verts[1])->ndc()).data());
         glVertex2dv(((verts[2])->ndc()).data());
         glVertex2dv(((verts[3])->ndc()).data());
         glVertex2dv(((verts[0])->ndc()).data());      
         glEnd();
         GL_VIEW::end_line_smooth();
         glPopAttrib();
      }
   }
   

   

   //Bedge_list edges = patch()->edges();
   //UnreachedSimplexFilter    unreached;
   //StrongEdgeFilter          strong;
   //PatchEdgeFilter           mine(patch()->cur_patch());
   //EdgeStrip(edges,mine).draw(_cb);

   /*
   for (uint j=0; j < _strokes.size(); ++j) {
               ProxyStroke* rendered_stroke = new ProxyStroke();
               double w = 4.0;
               w *= (_proxy_texture->patch()->get_z_accum().length()); 

               rendered_stroke->set_width(w);

               _strokes[j]->stroke_setup(rendered_stroke);
               
               if(j==0) rendered_stroke->draw_start();
               rendered_stroke->draw(VIEW::peek());
               if(j==(_strokes.size()-1)) rendered_stroke->draw_end();
               delete rendered_stroke;

               //start_strokes.push_back(strokes[j]);					
               //	connect_draw(strokes[j], j, faces[i]);
   }
   */
   //cerr << "We have strokes: " << _strokes.size() << endl;
   //create_strokes();

   //vector<ProxyUVStroke*> starter_strokes;
   //draw_starter_strokes(patch()->mesh()->faces());

   //if(!starter_strokes.empty()){
   //	starter_strokes[0]->draw_start();
   //	for (uint j=0; j < starter_strokes.size(); ++j) 
   //	{
   //		ProxyUVStroke* srk = starter_strokes[j];
   //		connect_draw(ProxyUVStroke* srk);

   //srk->draw();
   //cerr << j << " : " << " "<< " : " << srk->get_next_s() << endl;
   //if(srk->get_next_s())
   //	srk->get_next_s()->draw();

   //while(srk)
   //{
   //	assert(srk);
   //	srk->draw();
   //	srk = srk->get_next_s();			
   //} 
   //	}
   //	starter_strokes[0]->draw_end();
   //}

   /*
   CBface_list& faces = patch()->mesh()->faces();
   for (int i=0; i < faces.num(); ++i) {
   assert(faces[i] && faces[i]->is_quad());
   // Draw each quad only once:
   if (faces[i]->is_quad_rep()) {
   const vector<ProxyUVStroke*>& strokes = StrokeData::get_strokes(faces[i], _proxy_texture->proxy_surface());

   assert(!strokes.empty());

   //XXX need to change this so we don't call start/end on each quad...
   strokes[0]->draw_start();
   for (uint j=0; j < strokes.size(); ++j) {
   strokes[j]->draw(faces[i]);
   }      
   strokes[0]->draw_end();

   }
   }
   */
   // Undo state for drawing in NDCZ:
   draw_end();

   return 0;
}

int
HatchingTexture::draw_final(CVIEWptr& v)
{
   return 0;
}

void 
HatchingTexture::draw_start()
{
   // Set projection and modelview matrices for drawing in NDCZ:

   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixd(VIEW::peek()->ndc_proj().transpose().matrix());

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();
}

void  
HatchingTexture::draw_end()
{
   // Restore matrices

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
}

bool 
HatchingTexture::add_stroke(CNDCpt_list& pl, const vector<double>& prl, BaseStroke * proto)
{    
   cerr << "1 ProxySurface::add : we have " << _strokes.size() << endl;

   // It happens:
   if (pl.empty()) {
      err_msg("PatternGroup:add() - Error: point list is empty!");
      return false;
   }
   if (prl.empty()) {
      err_msg("PatternGroup:add() - Error: pressure list is empty!");
      return false;
   }
   if (pl.size() != prl.size()) {
      err_msg("PatternGroup:add() - gesture pixel list and pressure list are not same length.");
      return false;
   }

   NDCpt_list              smoothpts;
   vector<double>          smoothprl;
   if (!(HatchingGroupBase::smooth_gesture(pl, smoothpts, prl, smoothprl, 99)))
        return false;
  
   //make uv list 
   UVpt_list uv_list;
   for (NDCpt_list::size_type i=0; i < smoothpts.size(); ++i) {
      uv_list.push_back(_proxy_texture->proxy_surface()->getUVfromNDC(smoothpts[i]));
   }
   UVpt base_uv = uv_list.average();
   base_uv[0] = (int)base_uv[0];
   base_uv[1] = (int)base_uv[1];

   cerr << "Base uv is " << base_uv << endl;
   //now lets make the stroke relative to the face...
   for (UVpt_list::size_type i=0; i < uv_list.size(); ++i) {
      uv_list[i] -= base_uv;
   }

   ProxyUVStroke* new_stroke = new ProxyUVStroke(uv_list,_proxy_texture->proxy_surface(),proto);	

   _strokes.push_back(new_stroke);
   cerr << "adding a stroke " << _strokes.size() << endl;
   return true;
}

