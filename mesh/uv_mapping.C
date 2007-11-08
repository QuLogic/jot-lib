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
#include "std/support.H" 
#include "glew/glew.H" // XXX - remove when debugging phase is over

#include "uv_data.H"
#include "uv_mapping.H"
#include "bmesh.H"
#include "std/config.H"

using namespace mlib;

/*****************************************************************
 * UVMapping
 *****************************************************************/

//XXX -The map dimension should really
//be dynamically determined to optimize
//some binning crtieria

#define DEBUG_DOT_HEIGHT 200
#define MAPPING_SIZE 100
#define MAPPING_BINS MAPPING_SIZE*MAPPING_SIZE

#define DEBUG_SIZE (MAPPING_SIZE + DEBUG_DOT_HEIGHT)*MAPPING_SIZE

/////////////////////////////////////
// Constructor
/////////////////////////////////////
UVMapping::UVMapping(Bface *f) :
   _seed_face(f),
   _face_cnt(0),
   _bin_cnt(0),
   _entry_cnt(0),
   _use_cnt(0),
   _du(-1.0),
   _dv(-1.0),
   _wrap_u(false),
   _wrap_v(false),
   _wrap_bad(false),
   _virgin_debug_image(0),
   _marked_debug_image(0)
{
   
   int k;

   assert(_seed_face);

   _mapping.clear();
   for (k=0 ; k < MAPPING_BINS ; k++) _mapping.add(new ARRAY<Bface*>);

   compute_limits(f);
   compute_mapping(f);
   compute_wrapping(f);

   //If debug is set, generate texture map
   if (Config::get_var_bool("HATCHING_DEBUG_MAPPING",false))
      compute_debug_image();


}

/////////////////////////////////////
// Destructor
/////////////////////////////////////
UVMapping::~UVMapping()
{


   UVdata* uvd;
   int k, i, cnt=0;

   //We should be unreferenced if we get here...
   assert(!_use_cnt);

   //Remove ourself from the uvdata of binned faces
   for (k=0;k<_mapping.num();k++) 
      {
         for (i=0;i<_mapping[k]->num();i++) 
            {
               uvd = UVdata::lookup((*_mapping[k])[i]);
               assert(uvd);
               if (uvd->mapping())
                  {
                     assert(uvd->mapping()==this);
                     uvd->set_mapping(0);
                     cnt++;
                  }
            }
      }

   err_mesg(ERR_LEV_WARN,  
      "UVMapping::~UVMapping() - Removed from %d faces (should have been %d).",
         cnt, _face_cnt);

   for (k=0;k<_mapping.num();k++) delete _mapping[k];
   _mapping.clear();

   if (_virgin_debug_image) delete[] _virgin_debug_image;
   if (_marked_debug_image) delete[] _marked_debug_image;

}

/////////////////////////////////////
// draw_debug()
/////////////////////////////////////
void 
UVMapping::draw_debug() 
{

   int w,h;

   VIEW_SIZE(w,h);         
   
   // before calling glRasterPos2i(),
   // first set up orthographic viewing
   // matrix for points in pixel coords:
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixd(VIEW::peek()->pix_proj().transpose().matrix());

   glRasterPos2i(w-MAPPING_SIZE,0);

   glPushAttrib(GL_ENABLE_BIT);
   glDisable(GL_BLEND);

   glDrawPixels(MAPPING_SIZE,MAPPING_SIZE + DEBUG_DOT_HEIGHT,GL_RGB,GL_UNSIGNED_BYTE,_marked_debug_image);

   glPopAttrib();

   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
}


/////////////////////////////////////
// compute_debug_image()
/////////////////////////////////////
//
// -Creates a bitmap in which each pixel
//  represents a bin
// -The grey level is scaled to white
//  where binning is maximum
// -Empty bins appear red
//
/////////////////////////////////////
void
UVMapping::compute_debug_image()
{

   err_mesg(ERR_LEV_SPAM, "UVMapping::compute_debug_map()");

   int k, max=0;

   assert(!_virgin_debug_image);
   assert(!_marked_debug_image);

   _virgin_debug_image = new unsigned char[3*DEBUG_SIZE];
   assert(_virgin_debug_image);
   _marked_debug_image = new unsigned char[3*DEBUG_SIZE];
   assert(_marked_debug_image);

   assert(_mapping.num() == MAPPING_BINS);

   for (k=0;k<_mapping.num();k++)
      if (_mapping[k]->num() > max)
         max = _mapping[k]->num();

   //The UV binning part

   GLubyte b;
   for (k=0;k<MAPPING_BINS;k++)
      {
         b = (unsigned char)( (double)(0x88)*
                              ( (double)(_mapping[k]->num()) / (double)(max) )  );

         if (b)
            {
               _virgin_debug_image[3*k  ] = b;
               _virgin_debug_image[3*k+1] = b;
               _virgin_debug_image[3*k+2] = b;
            }
         else
            {
               _virgin_debug_image[3*k  ] = 0;
               _virgin_debug_image[3*k+1] = 0x99;
               _virgin_debug_image[3*k+2] = 0xFF;
            }
      }

   //The dot product graph part

   for (k=MAPPING_BINS;k<DEBUG_SIZE;k++)
      {
         b = (unsigned char)0;

         _virgin_debug_image[3*k  ] = b;
         _virgin_debug_image[3*k+1] = b;
         _virgin_debug_image[3*k+2] = b;

      }

   int h = int(floor( ( 0.0 + 1.0 ) * (double) DEBUG_DOT_HEIGHT / 2.0 ));
   if (h==DEBUG_DOT_HEIGHT) h--;
        
   for (int u=0; u<MAPPING_SIZE; u++)
      {
         k = (MAPPING_SIZE + h) * MAPPING_SIZE + u;

         _virgin_debug_image[3*k  ] = 0x90;
         _virgin_debug_image[3*k+1] = 0x90;
         _virgin_debug_image[3*k+2] = 0x90;
      }

   clear_debug_image();
}
/////////////////////////////////////
// clear_debug_image()
/////////////////////////////////////
void
UVMapping::clear_debug_image()
{
   static bool cool = Config::get_var_bool("HATCHING_DEBUG_MAPPING",false);
   assert(cool);

   for (int i=0; i<3*DEBUG_SIZE; i++)
      _marked_debug_image[i] = _virgin_debug_image[i];
}

/////////////////////////////////////
// debug_dot()
/////////////////////////////////////
void
UVMapping::debug_dot(
   double ewe,
   double val, 
   unsigned char r,
   unsigned char g,
   unsigned char b)
{
   static bool cool = Config::get_var_bool("HATCHING_DEBUG_MAPPING",false);
   assert(cool);

   assert(val >= -1.0);
   assert(val <=  1.0);
   
   int u = int(floor( ( ewe - _min_u ) / _du ));
   if (u==MAPPING_SIZE) u--;

   int h = int(floor( ( val + 1.0 ) * (double) DEBUG_DOT_HEIGHT / 2.0 ));
   if (h==DEBUG_DOT_HEIGHT) h--;

   int k = (MAPPING_SIZE + h) * MAPPING_SIZE + u;

   _marked_debug_image[3*k  ] = r;
   _marked_debug_image[3*k+1] = g;
   _marked_debug_image[3*k+2] = b;

}

/////////////////////////////////////
// compute_limits()
/////////////////////////////////////
void
UVMapping::compute_limits(Bface *f)
{
   int k;

   assert(f);
   
   //There oughta be uv here
   UVdata* uvdata = UVdata::lookup(f);
   assert(uvdata);

   //And it should not be mapped yet
   assert(uvdata->mapping()==0);

   //Init uv limits
   _min_u = uvdata->uv(1)[0];
   _max_u = uvdata->uv(1)[0];
   _min_v = uvdata->uv(1)[1];
   _max_v = uvdata->uv(1)[1];

   //Clear the mesh bits
   CBface_list& faces = f->mesh()->faces();
   for (k=0; k< faces.num(); k++) faces[k]->clear_bit(1);

   //Walk from seed face and fill out uv min/max
   recurse(f,&UVMapping::add_limit);

   //Sanity check
   assert( _min_u < _max_u );
   assert( _min_v < _max_v );

   _span_u = _max_u - _min_u;
   _span_v = _max_v - _min_v;
        
   _du = _span_u/(double)MAPPING_SIZE;
   _dv = _span_v/(double)MAPPING_SIZE;

   err_mesg(ERR_LEV_INFO,
      "UVMapping::compute_limits() - u_min=%f u_max=%f v_min=%f v_max=%f u_span=%f v_span=%f",
         _min_u, _max_u, _min_v, _max_v, _span_u, _span_v);
}

/////////////////////////////////////
// compute_mapping()
/////////////////////////////////////
void
UVMapping::compute_mapping(Bface *f)
{
   int k;

   assert(f);
   
   //There oughta be uv here
   UVdata* uvdata = UVdata::lookup(f);
   assert(uvdata);

   //And it better not be mapped yet
   assert(uvdata->mapping()==0);

   //Clear mesh bits
   CBface_list& faces = f->mesh()->faces();
   for (k=0; k< faces.num(); k++) faces[k]->clear_bit(1);

   //Walk from seed face and store faces in region in map
   recurse(f,&UVMapping::add_face);

   err_mesg(ERR_LEV_INFO,
      "UVMapping::generate_mapping() - %d faces added to mapping (from %d in mesh) using %d entries.",
         _face_cnt, faces.num(), _entry_cnt);

   err_mesg(ERR_LEV_INFO,
      "UVMapping::generate_mapping() - %d populated bins of possible %d (%d holes).",
         _bin_cnt, MAPPING_BINS, MAPPING_BINS-_bin_cnt);
}

/////////////////////////////////////
// compute_wrapping()
/////////////////////////////////////
void
UVMapping::compute_wrapping(Bface *f)
{
   int k;

   assert(f);
   
   //There oughta be uv here
   UVdata* uvdata = UVdata::lookup(f);
   assert(uvdata);

   //All your mapping are belong to us!
   assert(uvdata->mapping()==this);

   //Clear mesh bits
   CBface_list& faces = f->mesh()->faces();
   for (k=0; k< faces.num(); k++) faces[k]->clear_bit(1);

   //Walk from seed face and check uv region edges
   recurse_wrapping(f);

   if (_wrap_bad)
      err_mesg(ERR_LEV_WARN, "UVMapping::compute_mapping() - No wrapping set due to anomolous seam.");
   if (_wrap_u)
      err_mesg(ERR_LEV_INFO, "UVMapping::compute_mapping() - Wrapping in u at (%f,%f) discontinuity.", _min_u, _max_u);
   if (_wrap_v)
      err_mesg(ERR_LEV_INFO, "UVMapping::compute_mapping() - Wrapping in v at (%f,%f) discontinuity.", _min_v, _max_v);
   if ( (!_wrap_bad) && (!_wrap_u) && (!_wrap_v) )
      err_mesg(ERR_LEV_INFO, "UVMapping::compute_mapping() - No wrapping detected.");

}

/////////////////////////////////////
// recurse_wrapping()
/////////////////////////////////////
void      
UVMapping::recurse_wrapping(Bface *seed_f)
{           
   int k;

   Bface *f;
   ARRAY<Bface*> faces;

   assert(seed_f);

   faces.push(seed_f);

   while (faces.num()>0)
   {
      //Remove face from end of queue
      f = faces.pop();

      //Skip if already seen
      if (!f->is_set(1))
      {
         f->set_bit(1);

         //If we get here, then this face *should* have uvdata
         //and *should* be allready be mapped to us!
         UVdata* uvdata = UVdata::lookup(f);
         assert(uvdata);
         assert(uvdata->mapping()==this);

         for (k=1; k<=3; k++)
         {
            Bedge *nxt_e = f->e(k);
            assert(nxt_e);
            Bface *nxt_f = nxt_e->other_face(f);
            if (nxt_f)
            {
               UVdata *nxt_uvdata = UVdata::lookup(nxt_f);
               if (nxt_uvdata)
               {
                  UVpt uva = uvdata->uv(k);
                  UVpt uvb = uvdata->uv((k==3)?(1):(k+1));
                  int nxt_k = ( (nxt_f->e1()==nxt_e)?(1):
                                ((nxt_f->e2()==nxt_e)?(2):
                                 ((nxt_f->e3()==nxt_e)?(3):(0))));
                  assert(nxt_k);
                  UVpt nxt_uva = nxt_uvdata->uv(nxt_k);
                  UVpt nxt_uvb = nxt_uvdata->uv((nxt_k==3)?(1):(nxt_k+1));

                  //If neighboring face has uv, and the they match
                  //we recurse into this next face
                  if ((uva==nxt_uvb)&&(uvb==nxt_uva))
                  {
                     //Stick face on start of queue
                     faces.push(nxt_f);
                  }
                  //But if not, let's see if the other face is
                  //part of this mapping. If it is, then we found
                  //a seam.  Find the direction (u or v) and if
                  //its consistent with the _min_u/_max_u (or v)
                  //Then set the wrap flag for u or v appropriately
                  //or just turn all wrapping off if something's amiss
                  else 
                  {
                     //Here's a seam!
                     if (nxt_uvdata->mapping() == this)
                     {               
                        //We support 2 kinds of wrapping:
                        //-Wrap on a line of constant u (wrap at _min_u,_max_u)
                        //-Wrap on a line of constant v (wrap at _min_v,_max_v)
                        //If neither is seen, or if the discontinuity isn't
                        //at the extrema, we found something anomolous, abort!
                        //Note - There can be holes at the seam without problems.

                        if ((uva[0]==uvb[0])&&(nxt_uva[0]==nxt_uvb[0]))
                        {
                           //This looks like wrapping on a line of const. u
                           //Let's make sure the discontinuity is at the extrema

                           if ( (uva[0]==_min_u && nxt_uva[0]==_max_u) ||
                                (uva[0]==_max_u && nxt_uva[0]==_min_u))
                           {
                              //It's all good
                              if (!_wrap_u)
                                 {
                                    err_mesg(ERR_LEV_SPAM, "UVMapping::recurse_wrapping() - Found a valid wrapping seam in u.");
                                    _wrap_u = true;
                                 }
                           }
                           else
                           {
                              //We aren't at the extrema, so set the bad flag
                              //to avoid further checking

                              _wrap_bad = true;
                              _wrap_u = false;
                              _wrap_v = false;

                              err_mesg(ERR_LEV_WARN,
                                 "UVMapping::recurse_wrapping() - Found an INVALID wrapping seam in u: (%f,%f) since u extrema are: (%f,%f)",
                                   uva[0], nxt_uva[0], _min_u, _max_u);
                              err_mesg(ERR_LEV_WARN, "UVMapping::recurse_wrapping() - Aborting all wrapping.");
                           }
                        }
                        else if ((uva[1]==uvb[1])&&(nxt_uva[1]==nxt_uvb[1]))
                        {
                           //This looks like wrapping on a line of const. v
                           //Let's make sure the discontinuity is at the extrema

                           if ( (uva[1]==_min_v && nxt_uva[1]==_max_v) ||
                                (uva[1]==_max_v && nxt_uva[1]==_min_v))
                           {
                              //It's all good
                              if (!_wrap_v)
                              {
                                 err_mesg(ERR_LEV_INFO, "UVMapping::recurse_wrapping() - Found a valid wrapping seam in v.");
                                 _wrap_v = true;
                              }
                           }
                           else
                           {
                              //We aren't at the extrema, so set the bad flag
                              //to avoid further checking

                              _wrap_bad = true;
                              _wrap_u = false;
                              _wrap_v = false;
                                        
                              err_mesg(ERR_LEV_WARN,
                                 "UVMapping::recurse_wrapping() - Found an INVALID wrapping seam in v: (%f,%f) since v extrema are: (%f,%f)",
                                   uva[1], nxt_uva[1], _min_v, _max_v);
                              err_mesg(ERR_LEV_WARN, "UVMapping::recurse_wrapping() - Aborting all wrapping.");
                           }
                        }
                        else
                        {
                           //One or both edges failed to show constant u or v
                           //Abort any further wrapping...

                           _wrap_bad = true;
                           _wrap_u = false;
                           _wrap_v = false;

                           err_mesg(ERR_LEV_WARN,
                              "UVMapping::recurse_wrapping() - Found an INVALID wrapping. The seam wasn't constant in u or v.");
                           err_mesg(ERR_LEV_WARN,
                              "UVMapping::recurse_wrapping() - Edge #1 (%f,%f)-(%f,%f) Edge #2 (%f,%f)-(%f,%f)",
                                 uva[0], uva[1], uvb[0], uvb[1], nxt_uvb[0], nxt_uvb[1], nxt_uva[0], nxt_uva[1]);
                           err_mesg(ERR_LEV_WARN,
                              "UVMapping::recurse_wrapping() - Aborting all wrapping.");
                        }
                     }
                  }
               }
            }
         }
      }
   }
}

/////////////////////////////////////
// recurse()
/////////////////////////////////////
void      
UVMapping::recurse(Bface *seed_f, rec_fun_t fun )
{           
   int k;
//   bool done = false;

   Bface*                  f;
   ARRAY<Bface*>   faces;

   assert(seed_f);

   faces.push(seed_f);

   while (faces.num()>0)
      {
         //Remove oldest face from end of queue
         f = faces.pop();

         //Skip if already seen
         if (!f->is_set(1))
            {
               f->set_bit(1);

               //If we get here, then this face *should* have uvdata
               //and *should* be unmapped
               UVdata* uvdata = UVdata::lookup(f);
               assert(uvdata);
               assert(uvdata->mapping()==0);

               //Do the action (add to map, or update limits, etc.)
               (this->*fun)(f);

               for (k=1; k<=3; k++)
                  {
                     Bedge *nxt_e = f->e(k);
                     assert(nxt_e);
                     Bface *nxt_f = nxt_e->other_face(f);
                     if (nxt_f)
                        {
                           UVdata *nxt_uvdata = UVdata::lookup(nxt_f);
                           if (nxt_uvdata)
                              {
                                 UVpt uva = uvdata->uv(k);
                                 UVpt uvb = uvdata->uv((k==3)?(1):(k+1));
                                 int nxt_k = ( (nxt_f->e1()==nxt_e)?(1):
                                               ((nxt_f->e2()==nxt_e)?(2):
                                                ((nxt_f->e3()==nxt_e)?(3):(0))));
                                 assert(nxt_k);
                                 UVpt nxt_uva = nxt_uvdata->uv(nxt_k);
                                 UVpt nxt_uvb = nxt_uvdata->uv((nxt_k==3)?(1):(nxt_k+1));

                                 //If neighboring face has uv, and the they match
                                 //we recurse into this next face
                                 if ((uva==nxt_uvb)&&(uvb==nxt_uva))
                                    {
                                       //Add to front of queue
                                       faces.push(nxt_f);
                                    }
                                 else {
                                    //Nothing
                                 }
                              }
                        }
                  }
            }
      }
}

/////////////////////////////////////
// add_limits()
/////////////////////////////////////
void 
UVMapping::add_limit(Bface *f)
{
   int k;

   assert(f);

   UVdata* uvdata = UVdata::lookup(f);
   assert(uvdata);
   assert(uvdata->mapping()==0);


   for (k=1; k<=3; k++)
      {
         if ( uvdata->uv(k)[0] < _min_u ) 
            _min_u = uvdata->uv(k)[0];
         if ( uvdata->uv(k)[0] > _max_u ) 
            _max_u = uvdata->uv(k)[0];
         if ( uvdata->uv(k)[1] < _min_v ) 
            _min_v = uvdata->uv(k)[1];
         if ( uvdata->uv(k)[1] > _max_v ) 
            _max_v = uvdata->uv(k)[1];
      }

}


/////////////////////////////////////
// add_face()
/////////////////////////////////////
void
UVMapping::add_face(Bface *f)
{
   int k, u, v;
   int umax=0, vmax=0;
   int umin=MAPPING_SIZE-1, vmin=MAPPING_SIZE-1;
   int entry_count = 0;
   int bin_count = 0;

   assert(f);

   UVdata* uvdata = UVdata::lookup(f);
   assert(uvdata);
   assert(uvdata->mapping()==0);

   //Sanity check
   for (k=1; k<=3; k++)
      {
         assert( uvdata->uv(k)[0] <= _max_u );
         assert( uvdata->uv(k)[1] <= _max_v );
         assert( uvdata->uv(k)[0] >= _min_u );
         assert( uvdata->uv(k)[1] >= _min_v );
      }

   //Find the square set of u,v bins holding face
   for (k=1; k<=3; k++)
      {
         u = int(floor( (uvdata->uv(k)[0] - _min_u) / _du ));
         if (u==MAPPING_SIZE) u--;
         v = int(floor( (uvdata->uv(k)[1] - _min_v ) /_dv ));
         if (v==MAPPING_SIZE) v--;

         if (u<umin) umin=u;
         if (u>umax) umax=u;

         if (v<vmin) vmin=v;
         if (v>vmax) vmax=v;
      }

   //So escape rounding error that would
   //exclude bins that we genuinely intersect
   //we puff out the set of bins by 1 on each side

   if (umin>0) umin--;
   if (vmin>0) vmin--;
   if (umax<MAPPING_SIZE-1) umax++;
   if (vmax<MAPPING_SIZE-1) vmax++;

   //Sanity checks
   assert(umin>=0);
   assert(vmin>=0);

   assert(umax<MAPPING_SIZE);
   assert(vmax<MAPPING_SIZE);

   assert(umax>=umin);
   assert(vmax>=vmin);

   UVpt_list box;
   UVpt_list tri;

   bool isect;

   for (v=vmin; v<=vmax; v++)
      {
         for (u=umin; u<=umax; u++)
            {
               isect = false;
         
               box.clear();
               tri.clear();

               box += UVpt( _min_u +     u *_du , _min_v +     v * _dv );
               box += UVpt( _min_u + (u+1) *_du , _min_v +     v * _dv );
               box += UVpt( _min_u + (u+1) *_du , _min_v + (v+1) * _dv );
               box += UVpt( _min_u +     u *_du , _min_v + (v+1) * _dv );

               tri += uvdata->uv1();
               tri += uvdata->uv2();
               tri += uvdata->uv3();

               //isect if box holds tri or vice versa
               if (box.contains(tri)) isect = true;
               else if (tri.contains(box)) isect = true;
               //or if any edges intersect
               else
                  {
                     for (k=0; k<3; k++)
                        {
                           if (!isect)
                              {
                                 if      (intersect(box[0],box[1],tri[k],tri[(k+1)%3])) isect=true;
                                 else if (intersect(box[1],box[2],tri[k],tri[(k+1)%3])) isect=true;
                                 else if (intersect(box[2],box[3],tri[k],tri[(k+1)%3])) isect=true;
                                 else if (intersect(box[3],box[0],tri[k],tri[(k+1)%3])) isect=true;

                              }
                        }
                  }
         
               if (isect)
                  {
                     entry_count++;
                     _mapping[u+MAPPING_SIZE*v]->add(f);
                     if ( _mapping[u+MAPPING_SIZE*v]->num() == 1 )   bin_count++;
                  }
      
            }
      }

   //By definition , a face imust fall somewhere
   //within the mapping's bin
   assert(entry_count>0);

   uvdata->set_mapping(this);

   //Increment mapped face count
   _face_cnt++;

   //Increment map face entry count
   _entry_cnt += entry_count;

   //Increment unique bin usage count
   _bin_cnt += bin_count;


}

/////////////////////////////////////
// find_face();
/////////////////////////////////////
Bface*   
UVMapping::find_face(CUVpt &uv, Wvec &bc)
{
   static bool debug = Config::get_var_bool("HATCHING_DEBUG_MAPPING",false);
   
   int k,i;
//   double bc1,bc2,bc3;
   UVdata* uvdata;
   Bface *f;

   int u = int(floor( ( uv[0] - _min_u ) / _du ));
   if (u==MAPPING_SIZE) u--;
   int v = int(floor( ( uv[1] - _min_v ) / _dv ));
   if (v==MAPPING_SIZE) v--;

   i = u+MAPPING_SIZE*v;

   for (k=0; k<_mapping[i]->num(); k++)
      {
         f = (*_mapping[i])[k];
         uvdata = UVdata::lookup(f);
         assert(uvdata);

         CUVpt& uv1 = uvdata->uv1();
         CUVpt& uv2 = uvdata->uv2();
         CUVpt& uv3 = uvdata->uv3();
         UVvec uv3_1 = uv3 - uv1;

         double A   = det(uv2 - uv1, uv3_1    );
         double bc1 = det(uv2 - uv , uv3 - uv ) / A;

         if ((bc1<0)||(bc1>1)) continue;
	 
         double bc2 = det(uv  - uv1, uv3_1    ) / A;

         if ((bc2<0)||(bc2>1)) continue;
		
         double bc3 = 1.0 - bc1 - bc2;
		
         if (bc3>=0)
            {
               bc = Wvec(bc1,bc2,bc3);

               if (debug)
                  {
                     _marked_debug_image[3*i  ] = 0xFF;
                     _marked_debug_image[3*i+1] = 0x77;
                     _marked_debug_image[3*i+2] = 0x77;
                  }
			
               return f;
            }

/*
  double A   = 0.5 * det(uvdata->uv2() - uvdata->uv1(), uvdata->uv3() - uvdata->uv1());
  double bc1 = 0.5 * det(uvdata->uv2() - uv,            uvdata->uv3() - uv           ) / A;
  double bc2 = 0.5 * det(uv            - uvdata->uv1(), uvdata->uv3() - uvdata->uv1()) / A;
  double bc3 = 1.0 - bc1 - bc2;

  if ( ((bc1>=0)&&(bc1<=1)) && ((bc2>=0)&&(bc2<=1)) && ((bc3)>=0) )
  {
  bc = Wvec(bc1,bc2,bc3);

  if (debug)
  {
  _marked_debug_image[3*i  ] = 0xFF;
  _marked_debug_image[3*i+1] = 0x77;
  _marked_debug_image[3*i+2] = 0x77;
  }
			
  return f;
  }
*/      

      }
   return 0;
}


/////////////////////////////////////
// intersect()
/////////////////////////////////////
bool
UVMapping::intersect(CUVpt &pt1a, CUVpt &pt1b, CUVpt &pt2a, CUVpt &pt2b)
{

   if ((pt1a==pt2a)||(pt1a==pt2b)||(pt1b==pt2a)||(pt1b==pt2b)) return true;

   double c1a = det(pt1b - pt1a, pt2a - pt1a);
   double c1b = det(pt1b - pt1a, pt2b - pt1a);
   double c2a = det(pt2b - pt2a, pt1a - pt2a);
   double c2b = det(pt2b - pt2a, pt1b - pt2a);



   if  ( ( ((c1a>=-gEpsZeroMath)&&(c1b<=gEpsZeroMath)) || ((c1a<=gEpsZeroMath)&&(c1b>=-gEpsZeroMath)) ) &&
         ( ((c2a>=-gEpsZeroMath)&&(c2b<=gEpsZeroMath)) || ((c2a<=gEpsZeroMath)&&(c2b>=-gEpsZeroMath)) )    )
      return true;

   return false;
}

/////////////////////////////////////
// interpolate()
/////////////////////////////////////
void
UVMapping::interpolate( 
   CUVpt &uv1, double frac1, 
   CUVpt &uv2, double frac2, 
   UVpt &uv )
{

   //Interpolates two uv values using given weights
   //and accounting for discontinuity at seams
	
   //XXX - Not bullet proof.  We check if both points
   //are within 25% of span's proximity to opposite
   //sides of seam.  If so, the lower value is offset by
   //the span, and the final sum is adjusted back into range.
   //e.g. Wrap in u, u ranges 0-1, u1=.2, u2=.9, u1->1.2, 
   //so ave=1.05, final=0.05


   double lo;
   double hi;

   if (!_wrap_u)
      {
         uv[0]  = uv1[0]*frac1 + uv2[0]*frac2;
      }
   else
      {
         lo = _min_u + 0.25*_span_u;
         hi = _max_u - 0.25*_span_u;
	
         if			((uv1[0]>hi) && (uv2[0]<lo))
            {
               uv[0] =           uv1[0]*frac1 + (uv2[0]+_span_u)*frac2;
            }
         else if	((uv1[0]<lo) && (uv2[0]>hi))
            {
               uv[0] = (uv1[0]+_span_u)*frac1 +           uv2[0]*frac2;
            }
         else
            {
               uv[0] = uv1[0]*frac1 + uv2[0]*frac2;	
            }

         if (uv[0]>_max_u) uv[0] = uv[0] - _span_u;
      }
	

   if (!_wrap_v)
      {
         uv[1]  = uv1[1]*frac1 + uv2[1]*frac2;
      }
   else
      {
         lo = _min_v + 0.25*_span_v;
         hi = _max_v - 0.25*_span_v;
	
         if			((uv1[1]>hi) && (uv2[1]<lo))
            {
               uv[1] =           uv1[1]*frac1 + (uv2[1]+_span_v)*frac2;
            }
         else if	((uv1[1]<lo) && (uv2[1]>hi))
            {
               uv[1] = (uv1[1]+_span_v)*frac1 +           uv2[1]*frac2;
            }
         else
            {
               uv[1] = uv1[1]*frac1 + uv2[1]*frac2;	
            }

         if (uv[1]>_max_v) uv[1] = uv[1] - _span_v;
      }
	

}


/////////////////////////////////////
// apply_wrap()
/////////////////////////////////////
//
// -Takes a uv and if u or v are out
//  of bounds and there is wrapping
//  in the respecive direction, the
//  coordinate is moved into range
//
/////////////////////////////////////

void
UVMapping::apply_wrap( 
   UVpt &uv)
{

   if (_wrap_u)
      {
         if (uv[0]<_min_u)
            do
               uv[0] += _span_u;
            while (uv[0]<_min_u);

         else if (uv[0]>_max_u)
            do
               uv[0] -= _span_u;
            while (uv[0]>_max_u);
      }

   if (_wrap_v)
      {
         if (uv[1]<_min_v)
            do
               uv[1] += _span_v;
            while (uv[1]<_min_v);

         else if (uv[1]>_max_v)
            do
               uv[1] -= _span_v;
            while (uv[1]>_max_v);
      }

}
/* end of file uv_mapping.C */
