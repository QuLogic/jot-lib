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
/*********************************************************************
 * Legacy toon shader, 1D textures, no pixel shaders
 **********************************************************************/

#include "gtex/gl_extensions.H"
#include "gtex/paper_effect.H"
#include "geom/gl_view.H"
#include "std/config.H"
#include "toon_texture_1D.H"

/**********************************************************************
 * Globals
 * changed (appended with "_1D") 
 * so it doesn't conflict with new toon_texture globals
 **********************************************************************/

bool     ntt_paper_flag_1D;       //If paper's being used
Wpt      ntt_light_pos_1D;        //Used to fetch light pos
Wvec     ntt_light_dir_1D;        //Used to fetch light dir
bool     ntt_positional_1D;       //Used to flag if pos or dir

bool     ntt_is_init_vertex_program_1D = false;
bool     ntt_use_vertex_program_1D = false;
bool     ntt_use_vertex_program_arb_1D = false;
bool     ntt_use_vertex_program_nv_1D = false;

GLuint   ntt_toon_prog_nv_1D;
GLuint   ntt_toon_prog_arb_1D;

//these are unchanged as they are inside the object
LIST<str_ptr>*    ToonTexture_1D::_toon_texture_names = 0;
LIST<TEXTUREptr>* ToonTexture_1D::_toon_texture_ptrs = 0;
LIST<str_ptr>*    ToonTexture_1D::_toon_texture_remap_orig_names = 0;
LIST<str_ptr>*    ToonTexture_1D::_toon_texture_remap_new_names = 0;

/*****************************************************************
 * Texture Remapping
 *****************************************************************/

char *toon_remap_base_1D = "nprdata/toon_textures/";
char *toon_remap_fnames_1D[][2] = 
{
//   {"dark-8.png",    "1D--dark-8.png"},
//   {"mydot4.png",    "2D--dash-normal-8-32.png"},
//   {"one_d.png",     "1D--gauss-narrow-8.png"},
   {NULL,            NULL}
};

/*****************************************************************
 * Shaders
 *****************************************************************/

const unsigned char NPRToonShaderNV_1D[]=
//c[0]-c[3] modelview
//c[4]-c[7] projection
//c[12]-c[15] texture0
//c[16]-c[19] texture1
//c[20] xy(1,1) in ndc
//c[21] light pos
//c[22] light dir
//c[23] pos->1,1,1,1 dir->0,0,0,0
{
   "!!VP1.0\
DP4 R0.x,v[OPOS],c[0];\
DP4 R0.y,v[OPOS],c[1];\
DP4 R0.z,v[OPOS],c[2];\
DP4 R0.w,v[OPOS],c[3];\
DP4 R1.x,R0,c[4];\
DP4 R1.y,R0,c[5];\
DP4 R1.z,R0,c[6];\
DP4 R1.w,R0,c[7];\
MOV o[HPOS],R1;\
MUL R2,R1,c[20];\
DP4 o[TEX1].x,R2,c[16];\
DP4 o[TEX1].y,R2,c[17];\
DP4 o[TEX1].z,R2,c[18];\
DP4 o[TEX1].w,R2,c[19];\
ADD R4,c[21],-v[OPOS];\
DP3 R4.w,R4,R4;\
RSQ R4.w,R4.w;\
MUL R4,R4,R4.w;\
SLT R11,R11,R11;\
SGE R10,R11,c[23];\
MUL R5,c[23],R4;\
MAD R5,R10,c[22],R5;\
DP3 R6.x,v[NRML],R5;\
MOV R6.yzw,R11;\
DP4 o[TEX0].x,R6,c[12];\
DP4 o[TEX0].y,R6,c[13];\
MOV o[COL0],v[COL0];\
END"
};


const unsigned char NPRToonShaderARB_1D[]=
//c[20] xy(1,1) in ndc
//c[21] light pos
//c[22] light dir
//c[23] pos->1,1,1,1 dir->0,0,0,0
{
   "!!ARBvp1.0\n\
\
ATTRIB iPos  = vertex.position;\
ATTRIB iNorm = vertex.normal;\
ATTRIB iCol  = vertex.color;\
\
OUTPUT oPos  = result.position;\
OUTPUT oCol  = result.color;\
OUTPUT oTex0 = result.texcoord[0];\
OUTPUT oTex1 = result.texcoord[1];\
\
PARAM  cZero      = {0.0,0.0,0.0,0.0};\
PARAM  cOne       = {1.0,1.0,1.0,1.0};\
PARAM  cNDC       = program.env[20];\
PARAM  cLPos      = program.env[21];\
PARAM  cLDir      = program.env[22];\
PARAM  cLPosFlag  = program.env[23];\
\
PARAM  mMVP[4] =  { state.matrix.mvp };\
PARAM  mTex0[4] =  { state.matrix.texture[0] };\
PARAM  mTex1[4] =  { state.matrix.texture[1] };\
\
TEMP   tXY;\
TEMP   tNDC;\
TEMP   tLDirFlag;\
TEMP   tLPosVec;\
TEMP   tLFinalVec;\
TEMP   tLDot;\
\
DP4 tXY.x, mMVP[0], iPos;\
DP4 tXY.y, mMVP[1], iPos;\
DP4 tXY.z, mMVP[2], iPos;\
DP4 tXY.w, mMVP[3], iPos;\
\
MUL tNDC,tXY,cNDC;\
\
ADD tLPosVec,  cLPos,   -iPos;\
DP3 tLPosVec.w,tLPosVec, tLPosVec;\
RSQ tLPosVec.w,tLPosVec.w;\
MUL tLPosVec,  tLPosVec, tLPosVec.w;\
\
SGE tLDirFlag,cZero,cLPosFlag;\
\
MUL tLFinalVec,cLPosFlag,tLPosVec;\
MAD tLFinalVec,tLDirFlag,cLDir,tLFinalVec;\
\
DP3 tLDot.x,iNorm,tLFinalVec;\
ABS tLDot.x,tLDot.x;\
MOV tLDot.yz,cZero;\
MOV tLDot.w, cOne;\
\
DP4 oTex0.x, mTex0[0], tLDot;\
DP4 oTex0.y, mTex0[1], tLDot;\
DP4 oTex0.z, mTex0[2], tLDot;\
DP4 oTex0.w, mTex0[3], tLDot;\
\
DP4 oTex1.x, mTex1[0], tNDC;\
DP4 oTex1.y, mTex1[1], tNDC;\
DP4 oTex1.z, mTex1[2], tNDC;\
DP4 oTex1.w, mTex1[3], tNDC;\
\
MOV oPos, tXY;\
MOV oCol, iCol;\
\
END"

};


/////////////////////////////////////
// init_vertex_program_nv()
/////////////////////////////////////

bool ntt_init_vertex_program_nv_1D()
{
#ifdef NON_NVIDIA_GFX
   return false;
#else
   if (GLExtensions::gl_nv_vertex_program_supported())
      {
         err_mesg(ERR_LEV_INFO, "ToonTexture_1D::init() - Can use NV vertex programs!");
#ifdef GL_NV_vertex_program
         glGenProgramsNV(1, &ntt_toon_prog_nv_1D); 
         glBindProgramNV(GL_VERTEX_PROGRAM_NV, ntt_toon_prog_nv_1D); 
         glLoadProgramNV(GL_VERTEX_PROGRAM_NV, ntt_toon_prog_nv_1D, strlen((char * ) NPRToonShaderNV_1D), NPRToonShaderNV_1D);
#endif
         return true;
      }
   else
      {
         return false;
      }
#endif
}

/////////////////////////////////////
// init_vertex_program_arb()
/////////////////////////////////////

bool ntt_init_vertex_program_arb_1D()
{
   assert(!ntt_is_init_vertex_program_1D);

   if (GLExtensions::gl_arb_vertex_program_supported())
      {
         bool success = false, native = false;
         err_mesg(ERR_LEV_INFO, "ToonTexture_1D::init() - Can use ARB vertex programs!");
#ifdef GL_ARB_vertex_program
         glGenProgramsARB(1, &ntt_toon_prog_arb_1D); 
         glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_arb_1D); 
         glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                            strlen((char * ) NPRToonShaderARB_1D), NPRToonShaderARB_1D);
         success = GLExtensions::gl_arb_vertex_program_loaded("ToonTexture_1D::init() - ", native, NPRToonShaderARB_1D);
#endif
         return success;
      }
   else
      {
         return false;
      }
}

/////////////////////////////////////
// init_vertex_program()
/////////////////////////////////////

void ntt_init_vertex_program_1D()
{
   assert(!ntt_is_init_vertex_program_1D);

   ntt_use_vertex_program_arb_1D = ntt_init_vertex_program_arb_1D();
   ntt_use_vertex_program_nv_1D =  ntt_init_vertex_program_nv_1D();

   if (ntt_use_vertex_program_nv_1D || ntt_use_vertex_program_arb_1D)
      {
         err_mesg(ERR_LEV_INFO, "ToonTexture_1D::init() - Will use vertex programs!");
         ntt_use_vertex_program_1D = true;
      }
   else
      {
         err_mesg(ERR_LEV_INFO, "ToonTexture_1D::init() - Won't be using vertex programs!");
         ntt_use_vertex_program_1D = false;
      }

   ntt_is_init_vertex_program_1D = true;
}

/////////////////////////////////////
// setup_vertex_program_nv()
/////////////////////////////////////
void ntt_setup_vertex_program_nv_1D()
{
#if defined(GL_NV_vertex_program) && !defined(NON_NVIDIA_GFX)

   NDCpt n(XYpt(1,1));

   glBindProgramNV(GL_VERTEX_PROGRAM_NV, ntt_toon_prog_nv_1D); 

   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 0, GL_MODELVIEW, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 4, GL_PROJECTION, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 12, GL_TEXTURE, GL_IDENTITY_NV);
   if (GLExtensions::gl_arb_multitexture_supported())
      {
#ifdef GL_ARB_multitexture
         glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 16, GL_TEXTURE1_ARB, GL_IDENTITY_NV);
#endif
      }

   glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 20, (float)n[0], (float)n[1], 1, 1);

   glProgramParameter4dNV(GL_VERTEX_PROGRAM_NV, 21,  
                          ntt_light_pos_1D[0], ntt_light_pos_1D[1], ntt_light_pos_1D[2], 1.0f);
   glProgramParameter4dNV(GL_VERTEX_PROGRAM_NV, 22,  
                          ntt_light_dir_1D[0], ntt_light_dir_1D[1], ntt_light_dir_1D[2], 0.0f);
   if (ntt_positional_1D)
      glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 23, 1, 1, 1, 1);
   else
      glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 23, 0, 0, 0, 0);

   glEnable(GL_VERTEX_PROGRAM_NV);  //GL_ENABLE_BIT

#endif
}

/////////////////////////////////////
// done_vertex_program_nv()
/////////////////////////////////////
void ntt_done_vertex_program_nv_1D()
{
#if defined(GL_NV_vertex_program) && !defined(NON_NVIDIA_GFX)
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 0,  GL_NONE, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 4,  GL_NONE, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 12, GL_NONE, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 16, GL_NONE, GL_IDENTITY_NV);
   glDisable(GL_VERTEX_PROGRAM_NV);  //GL_ENABLE_BIT
#endif
}


/////////////////////////////////////
// setup_vertex_program_arb()
/////////////////////////////////////
void ntt_setup_vertex_program_arb_1D()
{
#ifdef GL_ARB_vertex_program
   NDCpt n(XYpt(1,1));

   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_arb_1D); 

   glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 20, n[0], n[1], 1.0, 1.0);

   glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 21, 
                              ntt_light_pos_1D[0], ntt_light_pos_1D[1], ntt_light_pos_1D[2], 1.0);
   glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 22,  
                              ntt_light_dir_1D[0], ntt_light_dir_1D[1], ntt_light_dir_1D[2], 0.0);
   if (ntt_positional_1D)
      glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 23, 1.0, 1.0, 1.0, 1.0);
   else
      glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 23, 0.0, 0.0, 0.0, 0.0);


   glEnable(GL_VERTEX_PROGRAM_ARB);  //GL_ENABLE_BIT

#endif
}

/////////////////////////////////////
// done_vertex_program_arb()
/////////////////////////////////////
void ntt_done_vertex_program_arb_1D()
{
#ifdef GL_ARB_vertex_program
   glDisable(GL_VERTEX_PROGRAM_ARB);  //GL_ENABLE_BIT
#endif
}

/////////////////////////////////////
// setup_vertex_program()
/////////////////////////////////////
void ntt_setup_vertex_program_1D()
{
   assert(ntt_use_vertex_program_1D);

   if (ntt_use_vertex_program_arb_1D)
      {
         ntt_setup_vertex_program_arb_1D();
      }
   else if (ntt_use_vertex_program_nv_1D)
      {
         ntt_setup_vertex_program_nv_1D();
      }
   else
      {
         assert(0);
      }
}

/////////////////////////////////////
// done_vertex_program()
/////////////////////////////////////
void ntt_done_vertex_program_1D()
{
   assert(ntt_use_vertex_program_1D);

   if (ntt_use_vertex_program_arb_1D)
      {
         ntt_done_vertex_program_arb_1D();
      }
   else if (ntt_use_vertex_program_nv_1D)
      {
         ntt_done_vertex_program_nv_1D();
      }
   else
      {
         assert(0);
      }

}



/**********************************************************************
 * ToonTexCB:
 **********************************************************************/
void 
ToonTexCB_1D::faceCB(CBvert* v, CBface*f) 
{
   Wvec n;
   f->vert_normal(v,n);

   if (!ntt_use_vertex_program_1D)
      {
         if (ntt_positional_1D)
            glTexCoord2d(n * (ntt_light_pos_1D - v->loc()).normalized(), 0);
         else
            glTexCoord2d(n * ntt_light_dir_1D, 0);
   
         if (ntt_paper_flag_1D)
            PaperEffect::paper_coord(NDCZpt(v->wloc()).data());

         glVertex3dv(v->loc().data());
      }
   else
      {
         glNormal3dv(n.data());
         glVertex3dv(v->loc().data());
      }

}

/**********************************************************************
 * ToonTexture_1D:
 **********************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       ToonTexture_1D::_ntt_tags = 0;

/////////////////////////////////////
// update_lights()
/////////////////////////////////////
void
ToonTexture_1D::update_lights(CVIEWptr& v) 
{
   if (_light_index == -1)
      {
         ntt_positional_1D = !_light_dir;
         if (ntt_positional_1D)
            {
               Wpt pos(_light_coords[0],_light_coords[1],_light_coords[2]);
               if (_light_cam)
                  ntt_light_pos_1D = v->cam()->xform().inverse() * pos;
               else
                  ntt_light_pos_1D = pos;
               ntt_light_pos_1D = _patch->inv_xform() * ntt_light_pos_1D;
            }
         else
            {
               if (_light_cam)
                  ntt_light_dir_1D = v->cam()->xform().inverse() * _light_coords;
               else
                  ntt_light_dir_1D = _light_coords;
               ntt_light_dir_1D = (_patch->inv_xform() * ntt_light_dir_1D).normalized();
            }
      }
   else
      {
         ntt_positional_1D = v->light_get_positional(_light_index);
         if (ntt_positional_1D)
            {
               if (v->light_get_in_cam_space(_light_index))
                  ntt_light_pos_1D = v->cam()->xform().inverse() * 
                     v->light_get_coordinates_p(_light_index);
               else
                  ntt_light_pos_1D = v->light_get_coordinates_p(_light_index);
               ntt_light_pos_1D = _patch->inv_xform() * ntt_light_pos_1D;
            }
         else
            {
               if (v->light_get_in_cam_space(_light_index))
                  ntt_light_dir_1D = v->cam()->xform().inverse() * 
                     v->light_get_coordinates_v(_light_index);
               else
                  ntt_light_dir_1D = v->light_get_coordinates_v(_light_index);
               ntt_light_dir_1D = (_patch->inv_xform() * ntt_light_dir_1D).normalized();
            }
      }
}

/////////////////////////////////////
// draw()
/////////////////////////////////////
int
ToonTexture_1D::draw(CVIEWptr& v) 
{
   GL_VIEW::print_gl_errors("ToonTexture_1D::draw - Begin");

   int dl;
   
   if (_ctrl)
      return _ctrl->draw(v);

   ntt_paper_flag_1D = (_use_paper==1)?(true):(false);

   if (!ntt_is_init_vertex_program_1D) 
      ntt_init_vertex_program_1D();

   if (ntt_paper_flag_1D)
      {
         // The enclosing NPRTexture will have already
         // rendered the mesh in 'background' mode.
         // We should enable blending and draw with
         // z <= mode...
      }

   update_tex();
   update_lights(v);

   glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | 
                GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   GL_COL(_color, _alpha*alpha());      // GL_CURRENT_BIT
   glDisable(GL_LIGHTING);              // GL_ENABLE_BIT

   if (_tex) {
      _tex->apply_texture();            // GL_TEXTURE_BIT, GL_ENABLE_BIT
   }

   //if ((_ntt_paper_flag_1D) || (_alpha<1.0))
   //XXX - Just always blend, there may be alpha
   //in the texture...
   static bool OPAQUE_COMPOSITE =
      Config::get_var_bool("OPAQUE_COMPOSITE",false,true);

   if ((v->get_render_mode() == VIEW::TRANSPARENT_MODE) && (OPAQUE_COMPOSITE)) {
      glDisable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ZERO);
   } else {
      glEnable(GL_BLEND);                               //GL_ENABLE_BIT
      if (PaperEffect::is_alpha_premult())
         glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);   //GL_COLOR_BUFFER_BIT
      else
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //GL_COLOR_BUFFER_BIT
   }

   //Don't write depth if not using the background texture
   if (!_transparent)
      {
         glDepthMask(GL_FALSE);                             //GL_DEPTH_BUFFER_BIT
      }

   NDCZpt origin(0,0,0);
   if (ntt_paper_flag_1D && _travel_paper)
      {
         Wpt_list wbox;
         NDCZpt_list nbox;
      

         (_patch->xform() * _patch->mesh()->get_bb()).points(wbox);
         while (wbox.num() > 0) nbox += wbox.pop();
      
         static bool SCREEN_BOX = Config::get_var_bool("SCREEN_BOX",false,true);

         if (SCREEN_BOX)
            {
               double minx = nbox[0][0];
               double maxx = nbox[0][0];
               double miny = nbox[0][1];
               double maxy = nbox[0][1];
               for (int i=1; i<nbox.num(); i++)
                  {
                     if (nbox[i][0] < minx) minx = nbox[i][0];
                     if (nbox[i][0] > maxx) maxx = nbox[i][0];
                     if (nbox[i][1] < miny) miny = nbox[i][1];
                     if (nbox[i][1] > maxy) maxy = nbox[i][1];
                     origin[0] = (minx + maxx)/2.0;
                     origin[1] = (miny + maxy)/2.0;
                  }
            }
         else
            {
               origin = nbox.average();
            }
      }
   PaperEffect::begin_paper_effect(ntt_paper_flag_1D, origin[0], origin[1]);

   if (ntt_use_vertex_program_1D) 
      {
      

         // Try it with the display list
         if (BasicTexture::dl_valid(v))
            {
               ntt_setup_vertex_program_1D();       // GL_ENABLE_BIT, ???
               BasicTexture::draw(v);
               ntt_done_vertex_program_1D();

               PaperEffect::end_paper_effect(ntt_paper_flag_1D);

               glPopAttrib();

               return _patch->num_faces();
            }

         // Failed. Create it.
         dl = _dl.get_dl(v, 1, _patch->stamp());
         if (dl)
            glNewList(dl, GL_COMPILE);
      }

   set_face_culling();                  // GL_ENABLE_BIT
   _patch->draw_tri_strips(_cb);

   if (ntt_use_vertex_program_1D) 
      {
         // End the display list here
         if (_dl.dl(v)) {
            _dl.close_dl(v);

            // Built it, now execute it
            ntt_setup_vertex_program_1D();       // GL_ENABLE_BIT, ???
            BasicTexture::draw(v);
            ntt_done_vertex_program_1D();
         }
      }

   PaperEffect::end_paper_effect(ntt_paper_flag_1D);

   glPopAttrib();

   GL_VIEW::print_gl_errors("ToonTexture_1D::draw - End");

   return _patch->num_faces();
}

/////////////////////////////////////
// update()
/////////////////////////////////////


void
ToonTexture_1D::update_tex(void)
{

   int ind;

   if (!_toon_texture_names)
      {
         _toon_texture_names = new LIST<str_ptr>; assert(_toon_texture_names);
         _toon_texture_ptrs = new LIST<TEXTUREptr>; assert(_toon_texture_ptrs);

         _toon_texture_remap_orig_names = new LIST<str_ptr>; assert(_toon_texture_remap_orig_names);
         _toon_texture_remap_new_names = new LIST<str_ptr>; assert(_toon_texture_remap_new_names);

         int i = 0;
         while (toon_remap_fnames_1D[i][0] != NULL)
            {
               _toon_texture_remap_orig_names->add(str_ptr(toon_remap_base_1D) + toon_remap_fnames_1D[i][0]);
               _toon_texture_remap_new_names->add(str_ptr(toon_remap_base_1D) + toon_remap_fnames_1D[i][1]);
               i++;
            }
      }

   str_ptr tf = _tex_name;

   if (tf == NULL_STR)
      {
         assert(_tex == NULL);
         //_tex_name = NULL_STR;
         //_tex = NULL;
      }
   else if (_tex == NULL)
      {
         if ((ind = _toon_texture_names->get_index(tf)) != BAD_IND)
            {
               //Finding original name in cache...

               //If its a failed texture...
               if ((*_toon_texture_ptrs)[ind] == NULL)
                  {
                     //...see if it was remapped...
                     int ii = _toon_texture_remap_orig_names->get_index(tf);
                     //...and change to looking up the remapped name            
                     if (ii != BAD_IND)
                        {
                           str_ptr old_tf = tf;
                           tf = (*_toon_texture_remap_new_names)[ii];

                           ind = _toon_texture_names->get_index(tf);

                           err_mesg(ERR_LEV_SPAM, 
                                    "ToonTexture_1D::set_texture() - Previously remapped --===<<[[{{ (%s) ---> (%s) }}]]>>===--", 
                                    **(Config::JOT_ROOT()+old_tf), **(Config::JOT_ROOT()+tf) );
                        }
                  }

               //Now see if the final name yields a good texture...
               if ((*_toon_texture_ptrs)[ind] != NULL)
                  {
                     _tex = (*_toon_texture_ptrs)[ind];
                     _tex_name = tf;
                     err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::set_texture() - Using cached copy of texture.");
                  }
               else
                  {
                     err_mesg(ERR_LEV_INFO, "ToonTexture_1D::set_texture() - **ERROR** Previous caching failure: '%s'...", **tf);
                     _tex = NULL;
                     _tex_name = NULL_STR;
                  }
            }
         //Haven't seen this name before...
         else
            {
               err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::set_texture() - Not in cache...");
      
               Image i(Config::JOT_ROOT()+tf);

               //Can't load the texture?
               if (i.empty())
                  {
                     //...check for a remapped file...
                     int ii = _toon_texture_remap_orig_names->get_index(tf);

                     //...and use that name instead....
                     if (ii != BAD_IND)
                        {
                           //...but also indicate that the original name is bad...

                           _toon_texture_names->add(tf);
                           _toon_texture_ptrs->add(NULL);

                           str_ptr old_tf = tf;
                           tf = (*_toon_texture_remap_new_names)[ii];

                           err_mesg(ERR_LEV_ERROR, 
                                    "ToonTexture_1D::set_texture() - Remapping --===<<[[{{ (%s) ---> (%s) }}]]>>===--", 
                                    **(Config::JOT_ROOT()+old_tf), **(Config::JOT_ROOT()+tf) );

                           i.load_file(**(Config::JOT_ROOT()+tf));
                        }
                  }

               //If the final name loads, store the cached texture...
               if (!i.empty())
                  {
                     TEXTUREglptr t = new TEXTUREgl();

                     t->set_save_img(true);
                     t->set_wrap_s(GL_CLAMP_TO_EDGE);
                     t->set_wrap_t(GL_CLAMP_TO_EDGE);
                     t->set_image(i.copy(),i.width(),i.height(),i.bpp());
         

                     _toon_texture_names->add(tf);
                     _toon_texture_ptrs->add(t);

                     err_mesg(ERR_LEV_INFO, "ToonTexture_1D::set_texture() - Cached: (w=%d h=%d bpp=%u) %s",
                              i.width(), i.height(), i.bpp(), **(Config::JOT_ROOT()+tf));;

                     _tex = t;
                     _tex_name = tf;
                  }
               //Otherwise insert a failed NULL
               else
                  {
                     err_mesg(ERR_LEV_ERROR, "ToonTexture_1D::set_texture() - *****ERROR***** Failed loading to cache: '%s'...", **(Config::JOT_ROOT()+tf));
         
                     _toon_texture_names->add(tf);
                     _toon_texture_ptrs->add(NULL);

                     _tex = NULL;
                     _tex_name = NULL_STR;
                  }
            }   
      }



}

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
ToonTexture_1D::tags() const
{
   if (!_ntt_tags) {
      _ntt_tags = new TAGlist;
      *_ntt_tags += OGLTexture::tags();

      *_ntt_tags += new TAG_val<ToonTexture_1D,int>(
         "use_paper",
         &ToonTexture_1D::use_paper_);
      *_ntt_tags += new TAG_val<ToonTexture_1D,int>(
         "travel_paper",
         &ToonTexture_1D::travel_paper_);
      *_ntt_tags += new TAG_val<ToonTexture_1D,COLOR>(
         "COLOR",
         &ToonTexture_1D::color_);
      *_ntt_tags += new TAG_val<ToonTexture_1D,double>(
         "alpha",
         &ToonTexture_1D::alpha_);

      *_ntt_tags += new TAG_val<ToonTexture_1D,int>(
         "light_index",
         &ToonTexture_1D::light_index_);
      *_ntt_tags += new TAG_val<ToonTexture_1D,int>(
         "light_dir",
         &ToonTexture_1D::light_dir_);
      *_ntt_tags += new TAG_val<ToonTexture_1D,int>(
         "light_cam",
         &ToonTexture_1D::light_cam_);
      *_ntt_tags += new TAG_val<ToonTexture_1D,Wvec>(
         "light_coords",
         &ToonTexture_1D::light_coords_);


      *_ntt_tags += new TAG_meth<ToonTexture_1D>(
         "texture",
         &ToonTexture_1D::put_tex_name,
         &ToonTexture_1D::get_tex_name,
         1);
      *_ntt_tags += new TAG_meth<ToonTexture_1D>(
         "layer_name",
         &ToonTexture_1D::put_layer_name,
         &ToonTexture_1D::get_layer_name,
         1);


      *_ntt_tags += new TAG_meth<ToonTexture_1D>(
         "transparent",
         &ToonTexture_1D::put_transparent,
         &ToonTexture_1D::get_transparent,
         0);
      *_ntt_tags += new TAG_meth<ToonTexture_1D>(
         "annotate",
         &ToonTexture_1D::put_annotate,
         &ToonTexture_1D::get_annotate,
         0);

   }
   return *_ntt_tags;
}
////////////////////////////////////
// put_layer_name()
/////////////////////////////////////
void
ToonTexture_1D::put_layer_name(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::put_layer_name()");
        
   d.id();
   if (get_layer_name() == NULL_STR)
      {
         err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::put_layer_name() - Wrote NULL string.");
         *d << "NULL_STR";
         *d << " ";
      }
   else
      {
         *d << **(get_layer_name());
         *d << " ";
         err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::put_layer_name() - Wrote string: '%s'", **get_tex_name());
      }
   d.end_id();
}

/////////////////////////////////////
// get_layer_name()
/////////////////////////////////////

void
ToonTexture_1D::get_layer_name(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::get_layer_name()");

   //XXX - May need something to handle filenames with spaces

   str_ptr str, lay, space;
   *d >> str;      
   if (!(*d).ascii()) *d >> space; 

   if (str == "NULL_STR") 
      {
         lay = NULL_STR;
         err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::get_layer_name() - Loaded NULL string.");
      }
   else
      {
         lay = str;
         err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::get_layer_name() - Loaded string: '%s'", **lay);
      }
   set_layer_name(lay);

}

////////////////////////////////////
// put_texname()
/////////////////////////////////////
void
ToonTexture_1D::put_tex_name(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::put_tex_name()");

   //XXX - May need something to handle filenames with spaces

   d.id();
   if (_tex_name == NULL_STR)
      {
         err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::put_tex_name() - Wrote NULL string.");
         *d << "NULL_STR";
         *d << " ";
      }
   else
      {
         *d << **(get_tex_name());
         *d << " ";
         err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::put_tex_name() - Wrote string: '%s'", **get_tex_name());
      }
   d.end_id();
}

/////////////////////////////////////
// get_tex_name()
/////////////////////////////////////

void
ToonTexture_1D::get_tex_name(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::get_tex_name()");

   //XXX - May need something to handle filenames with spaces

   str_ptr str, space;
   *d >> str;      
   if (!(*d).ascii()) *d >> space; 

   if (str == "NULL_STR") 
      {
         str = NULL_STR;
         err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::get_tex_name() - Loaded NULL string.");
      }
   else
      {
         err_mesg(ERR_LEV_SPAM, "ToonTexture_1D::get_tex_name() - Loaded string: '%s'", **str);
      }
   set_tex_name(str);

}


////////////////////////////////////
// put_transparent()
/////////////////////////////////////
void
ToonTexture_1D::put_transparent(TAGformat &d) const
{
   // XXX - Deprecated
}

/////////////////////////////////////
// get_transparent()
/////////////////////////////////////
void
ToonTexture_1D::get_transparent(TAGformat &d)
{
   err_mesg(ERR_LEV_WARN, "ToonTexture_1D::get_transparent() - ***NOTE: Loading OLD format file!***");

   *d >> _transparent;
}

////////////////////////////////////
// put_annotate()
/////////////////////////////////////
void
ToonTexture_1D::put_annotate(TAGformat &d) const
{
   // XXX - Deprecated
}

/////////////////////////////////////
// get_annotate()
/////////////////////////////////////
void
ToonTexture_1D::get_annotate(TAGformat &d)
{
   err_mesg(ERR_LEV_WARN, "ToonTexture_1D::get_annotate() - ***NOTE: Loading OLD format file!***");
   
   *d >> _annotate;
}

// end of file toon_texture.C
