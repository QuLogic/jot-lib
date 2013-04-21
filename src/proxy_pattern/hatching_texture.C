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
/***************************************************************************
hatching_texture.C
***************************************************************************/
#include "proxy_stroke.H"
#include "proxy_texture.H"
#include "hatching_texture.H"
#include "npr/hatching_group_base.H"  //for smooth_gesture
#include "mesh/bfilters.H"

/**********************************************************************
* ProxyTexture:
**********************************************************************/
HatchingTexture::HatchingTexture(Patch* patch) :
BasicTexture(patch, new GLStripCB),
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
   for(int i=0; i < faces.num(); ++i)
   {
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
HatchingTexture::add_stroke(CNDCpt_list& pl, const ARRAY<double>& prl, BaseStroke * proto)
{    
   cerr << "1 ProxySurface::add : we have " << _strokes.size() << endl;
   //   int k;
   //   Bface *f;

   // It happens:
   if (pl.empty()){
      err_msg("PatternGroup:add() - Error: point list is empty!");
      return false;
   }
   if (prl.empty()){
      err_msg("PatternGroup:add() - Error: pressure list is empty!");
      return false;
   }
   if (pl.num() != prl.num()){
      err_msg("PatternGroup:add() - gesture pixel list and pressure list are not same length.");
      return false;
   }

   NDCpt_list              smoothpts;
   ARRAY<double>           smoothprl;
   if (!(HatchingGroupBase::smooth_gesture(pl, smoothpts, prl, smoothprl, 99)))
        return false;
  
   //make uv list 
   UVpt_list uv_list;
   for (int i=0; i < smoothpts.num(); ++i)
   {      
      uv_list += _proxy_texture->proxy_surface()->getUVfromNDC(smoothpts[i]);
   }	
   UVpt base_uv = uv_list.average();
   base_uv[0] = (int)base_uv[0];
   base_uv[1] = (int)base_uv[1];
   
   cerr << "Base uv is " << base_uv << endl;
   //now lets make the stroke relative to the face...
   for (int i=0; i < uv_list.num(); ++i)
   {       
      uv_list[i] -= base_uv;
   }	

   ProxyUVStroke* new_stroke = new ProxyUVStroke(uv_list,_proxy_texture->proxy_surface(),proto);	
   
   
   //StrokeData::add_stroke(f, this, new_stroke);  
   _strokes.push_back(new_stroke);
   cerr << "adding a stroke " << _strokes.size() << endl;
   return true;

}
/*
void
HatchingTexture::connect_draw(ProxyUVStroke* srk,int k, Bface* face)
{
   ProxyUVStroke* s = srk;
   Bface* f =face;
   while(s)
   {
      //s->draw();
      Bface* next_face = _proxy_texture->proxy_surface()->neighbor_face(1, f);		
      if(next_face)
      {
         const vector<ProxyUVStroke*>& strokes = StrokeData::get_strokes(next_face, this);
         assert(!strokes.empty());
         assert(strokes.size() >= uint(k));
         s=strokes[k];
         f=next_face;
      }else{
         s=0;
      }	
   }
}

void 
HatchingTexture::build_stroke(ProxyStroke* rendered_stroke, ProxyUVStroke* stroke_part, int k, Bface* face)
{
   ProxyUVStroke* s = stroke_part;
   Bface* f =face;

   while(s)
   {
      s->stroke_setup(f, rendered_stroke);
      Bface* next_face = _proxy_texture->proxy_surface()->neighbor_face(1, f);		
      if(next_face)
      {
         const vector<ProxyUVStroke*>& strokes = StrokeData::get_strokes(next_face, this);
         assert(!strokes.empty());
         assert(strokes.size() >= uint(k));
         s=strokes[k];
         f=next_face;
      }else{
         s=0;
      }	
   }
}
void 
HatchingTexture::draw_starter_strokes(CBface_list& faces)
{
   for (int i=0; i < faces.num(); ++i) {
      assert(faces[i] && faces[i]->is_quad());
      // Draw each quad only once:
      if (faces[i]->is_quad_rep()) {

         Bface* prev_face = _proxy_texture->proxy_surface()->neighbor_face(3, faces[i]);
         if(!prev_face){
            const vector<ProxyUVStroke*>& strokes = StrokeData::get_strokes(faces[i],this);
            assert(!strokes.empty());


            //strokes[0]->draw_start();
            for (uint j=0; j < strokes.size(); ++j) {
               ProxyStroke* rendered_stroke = new ProxyStroke();
               double w = 4.0;
               //w *= (_proxy_texture->proxy_surface()->scale()); 

               rendered_stroke->set_width(w);

               build_stroke(rendered_stroke, strokes[j], j, faces[i]);
               if(j==0) rendered_stroke->draw_start();
               rendered_stroke->draw(VIEW::peek());
               if(j==(strokes.size()-1)) rendered_stroke->draw_end();
               delete rendered_stroke;

               //start_strokes.push_back(strokes[j]);					
               //	connect_draw(strokes[j], j, faces[i]);
            }
            //strokes[0]->draw_end();
         }
      }
   }
}
void 
HatchingTexture::create_strokes()
{  
   int n = 20;           // 5 "strokes" per half-quad 
   int samples = 100;   // 100 samples per stroke
   //    bool doit= false;
   assert(_proxy_texture && _proxy_texture->proxy_surface());

   //cerr << "Creating strokes" << endl;
   CBface_list& faces = patch()->mesh()->faces();

   // For each face, check if there is already strokes in it, if not, then add strokes   
   for (int i=0; i < faces.num(); ++i) 
   {
      assert(faces[i] && faces[i]->is_quad());
      // Draw each quad only once:
      if (faces[i]->is_quad_rep()) 
      {
         //Bface* next_face = _proxy_texture->proxy_surface()->neighbor_face(1, faces[i]);		
         //first lets make sure the next_face has strokes, if not lets put them in....
        
         //lets put in strokes in current face
         if(!StrokeData::have_strokes(faces[i],this))
         {
            // b-c
            // | | 
            // a-d
            UVpt a(0.01,0.01);
            UVpt b(0.01,0.99);
            UVpt c(0.99,0.99);
            UVpt d(0.99,0.01);
            for (int k=0; k < n; ++k){
               double t = double(k)/n;
               ProxyUVStroke* s = create_one_stroke(interp(a,b,t),interp(d,c,t),samples, 0, faces[i]);
               StrokeData::add_stroke(faces[i],this, s);  
            }
            //cerr<< "face added " << i << endl;
         }	
       
      }
   }
}

ProxyUVStroke* 
HatchingTexture::create_one_stroke(
                                   CUVpt& start,
                                   CUVpt& end,
                                   int num_samples,
                                   BaseStroke* proto,
                                   Bface* f)
{
   UVpt_list new_pts(num_samples);   
   for (int i=0; i < num_samples; ++i) {
      UVpt n = interp(start,end,double(i)/num_samples);
      //n[0] += drand48()/100;
      //n[1] += drand48()/100;
      new_pts += n;
   }
   ProxyUVStroke* s = new ProxyUVStroke(new_pts,_proxy_texture->proxy_surface(),proto, f);
   return s;
}
*/
/*
void 
HatchingTexture::procedural_strokes()
{
int n = 30;
int samples = 100;
NDCpt a(-1.0,-1.0);
NDCpt b(-1.0,1.0);
NDCpt c(1.0,1.0);
NDCpt d(1.0,-1.0);
// b-c
// | | 
// a-d
for (int i=0; i < n; ++i){
double t = (double)i/(double)n;
NDCpt tmp1 = t*b+(1-t)*a;
NDCpt tmp2 = t*b+(1-t)*c;
//add_stroke(tmp1,tmp2,samples, _ui->get_stroke());  
}
for (int i=1; i < n; ++i){
double t = (double)i/(double)n;
NDCpt tmp1 = t*d+(1-t)*a;
NDCpt tmp2 = t*d+(1-t)*c;
//add_stroke(tmp1,tmp2,samples, _ui->get_stroke());  
}

}


bool
HatchingTexture::add(CNDCpt_list& pl,
const ARRAY<double>& prl,
BaseStroke * proto)
{    
cerr << "1 ProxySurface::add : we have " << _strokes.size() << endl;
//   int k;
//   Bface *f;

// It happens:
if (pl.empty()){
err_msg("PatternGroup:add() - Error: point list is empty!");
return false;
}
if (prl.empty()){
err_msg("PatternGroup:add() - Error: pressure list is empty!");
return false;
}
if (pl.num() != prl.num()){
err_msg("PatternGroup:add() - gesture pixel list and pressure list are not same length.");
return false;
}

//  NDCpt_list              smoothpts;
//   ARRAY<double>           smoothprl;
//   if (!(HatchingGroupBase::smooth_gesture(pl, smoothpts, prl, smoothprl, 99)))
//     return false;



// Calculate pixel length of a stroke
//double pix_len = smoothpts.length() * VIEW::peek()->ndc2pix_scale();
// if (pix_len < 8.0)   {
//    err_msg("PatternGroup::add() - Stroke only %f pixels. Probably an accident. Punting...", pix_len);
//    return false;
// }
//  BaseStrokeOffsetLISTptr ol = new BaseStrokeOffsetLIST;

//    ol->set_replicate(0);
//    ol->set_hangover(1);
//    ol->set_pix_len(pix_len);

//    ol->add(BaseStrokeOffset( 0.0, 0.0, smoothprl[0], BaseStrokeOffset::OFFSET_TYPE_BEGIN));
//    for (k=1; k< smoothprl.num(); k++)
//          ol->add(BaseStrokeOffset( (double)k/(double)(smoothprl.num()-1), 0.0, smoothprl[k], BaseStrokeOffset::OFFSET_TYPE_MIDDLE));

//    ol->add(BaseStrokeOffset( 1.0, 0.0, smoothprl[smoothprl.num()-1],   BaseStrokeOffset::OFFSET_TYPE_END));
//NDCZvec_list _pts;   

// for (int i=0; i < pl.num(); ++i)
// {
//    _pts += (pl[i] - _center);
// }	
//Proxy3dStroke* new_stroke = new Proxy3dStroke(_pts,this,proto);	
//_strokes.push_back(new_stroke);
return true;

}

void 
HatchingTexture::add_stroke(NDCpt a, NDCpt b, int pts, BaseStroke* proto)
{

NDCZvec_list _pts;   
for (int i=0; i < pts; ++i)
{
double t = (double)i/(double)pts;
NDCpt tmp = t*b+(1-t)*a;
_pts += (tmp - _center);
}	
Proxy3dStroke* new_stroke = new Proxy3dStroke(_pts,this,proto);	
_strokes.push_back(new_stroke);

}

int
ProxySurface::draw(CVIEWptr& v)
{  
if (!(_patch && _patch->mesh()))
return 0;

// create the proxy surface if needed:
create_proxy_surface();
assert(_proxy_mesh);
update_proxy_surface();

// draw proxy mesh in world coordinates (for debugging):
glMatrixMode(GL_MODELVIEW);
glPushMatrix();
glLoadMatrixd(VIEW::peek()->world_to_eye().transpose().matrix());
//draw_flatshade(v, _proxy_mesh);
draw_wireframe(v, _proxy_mesh);
//draw_halfton(v, _proxy_mesh);
glPopMatrix();

return 0;
}
*/
