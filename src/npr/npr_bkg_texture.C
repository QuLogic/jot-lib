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
 * npr_bkg_texture.C
 **********************************************************************/

#include "gtex/gl_extensions.H"
#include "gtex/paper_effect.H"
#include "geom/gl_view.H"
#include "mesh/uv_data.H"
#include "npr_bkg_texture.H"
#include "npr/hatching_group_fixed.H"
#include "std/config.H"

using namespace mlib;

extern str_ptr JOT_ROOT;

/**********************************************************************
 * Globals
 **********************************************************************/

bool     nbt_paper_flag;       //If paper's being used
bool     nbt_tex_flag;         //If texture's being used
bool     nbt_is_init_vertex_program = false;
bool     nbt_use_vertex_program;
bool     nbt_use_vertex_program_arb;
bool     nbt_use_vertex_program_nv;

GLuint   nbt_bkg_prog_nv;
GLuint   nbt_bkg_prog_arb;

const unsigned char NPRBkgShaderNV[]=
{
                        //c[0]-c[3] modelview
                        //c[4]-c[7] projection
                        //c[12]-c[15] texture0
                        //c[16]-c[19] texture1

// Har har --  Would you believe it!?! To defeat
// perspective correction of texture coords
// we simply neglect to divide (x,y,z,w) by w.
// We employ 4d texture coordinate (u,v,s,t)
// and expect (u,v,s,t) = (x/w,y/w,0,1) = (u',v',0,t')
// but then the GL hardware actually interpolates
// (u'/w,v'/w,0,1/w) and post multiplies by w/t'=w to
// get the perspective corrected result.
// Thus we use (u,v,s,t) = (x,y,0,w) = (wu',wv',0,wt')
// causing the hardware to interpolate *linearly*
// (wu'/w,wv'/w,0,wt'/w) = (u',v',0,t') as desired
// and post multiplying by w/t=w/(wt')=1/t'=1
                        
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
DP4 o[TEX0].x,R2,c[12];\
DP4 o[TEX0].y,R2,c[13];\
DP4 o[TEX0].z,R2,c[14];\
DP4 o[TEX0].w,R2,c[15];\
DP4 o[TEX1].x,R2,c[16];\
DP4 o[TEX1].y,R2,c[17];\
DP4 o[TEX1].z,R2,c[18];\
DP4 o[TEX1].w,R2,c[19];\
MOV o[COL0],v[COL0];\
END"
};

const unsigned char NPRBkgShaderARB[]=
{
"!!ARBvp1.0\n\
\
ATTRIB iPos = vertex.position;\
ATTRIB iCol = vertex.color;\
\
OUTPUT oPos  = result.position;\
OUTPUT oCol  = result.color;\
OUTPUT oTex0 = result.texcoord[0];\
OUTPUT oTex1 = result.texcoord[1];\
\
PARAM  cNDC   = program.env[20];\
\
PARAM  mMVP[4] =  { state.matrix.mvp };\
PARAM  mTex0[4] =  { state.matrix.texture[0] };\
PARAM  mTex1[4] =  { state.matrix.texture[1] };\
\
TEMP   tXY;\
TEMP   tNDC;\
\
DP4 tXY.x, mMVP[0], iPos;\
DP4 tXY.y, mMVP[1], iPos;\
DP4 tXY.z, mMVP[2], iPos;\
DP4 tXY.w, mMVP[3], iPos;\
\
MUL tNDC,tXY,cNDC;\
\
DP4 oTex0.x, mTex0[0], tNDC;\
DP4 oTex0.y, mTex0[1], tNDC;\
DP4 oTex0.z, mTex0[2], tNDC;\
DP4 oTex0.w, mTex0[3], tNDC;\
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

bool nbt_init_vertex_program_nv()
{
   if (GLExtensions::gl_nv_vertex_program_supported())
   {
      err_mesg(ERR_LEV_INFO, "NPRBkgTexture::init() - Can use NV vertex programs!");
#if defined(GL_NV_vertex_program) && !defined(NON_NVIDIA_GFX)
      glGenProgramsNV(1, &nbt_bkg_prog_nv); 
      glBindProgramNV(GL_VERTEX_PROGRAM_NV, nbt_bkg_prog_nv); 
	   glLoadProgramNV(GL_VERTEX_PROGRAM_NV, nbt_bkg_prog_nv, strlen((char * ) NPRBkgShaderNV), NPRBkgShaderNV);
#endif
      return true;
   }
   else
   {
      return false;
   }
}

/////////////////////////////////////
// init_vertex_program_arb()
/////////////////////////////////////

bool nbt_init_vertex_program_arb()
{
   assert(!nbt_is_init_vertex_program);

   if (GLExtensions::gl_arb_vertex_program_supported())
   {
      bool success = false, native = false;
      err_mesg(ERR_LEV_INFO, "NPRBkgTexture::init() - Can use ARB_vertex_program!");
#ifdef GL_ARB_vertex_program
      glGenProgramsARB(1, &nbt_bkg_prog_arb); 
      glBindProgramARB(GL_VERTEX_PROGRAM_ARB, nbt_bkg_prog_arb); 
	   glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                           strlen((char * ) NPRBkgShaderARB), NPRBkgShaderARB);

      success = GLExtensions::gl_arb_vertex_program_loaded("NPRBkgTexture::init() - ", native, NPRBkgShaderARB);
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

void nbt_init_vertex_program()
{
   assert(!nbt_is_init_vertex_program);

   nbt_use_vertex_program_arb = nbt_init_vertex_program_arb();
   nbt_use_vertex_program_nv =  nbt_init_vertex_program_nv();

   if (nbt_use_vertex_program_nv || nbt_use_vertex_program_arb)
   {
      err_mesg(ERR_LEV_INFO, "NPRBkgTexture::init() - Will use vertex programs!");
      nbt_use_vertex_program = true;
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "NPRBkgTexture::init() - Won't be using vertex programs!");
      nbt_use_vertex_program = false;
   }

   nbt_is_init_vertex_program = true;
}


/////////////////////////////////////
// setup_vertex_program_nv()
/////////////////////////////////////
void nbt_setup_vertex_program_nv()
{
#if defined(GL_NV_vertex_program) && !defined(NON_NVIDIA_GFX)

   NDCpt n(XYpt(1,1));

   glBindProgramNV(GL_VERTEX_PROGRAM_NV, nbt_bkg_prog_nv); 

   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 0,  GL_MODELVIEW,  GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 4,  GL_PROJECTION, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 12, GL_TEXTURE,    GL_IDENTITY_NV);

   if (GLExtensions::gl_arb_multitexture_supported())
   {
#ifdef GL_ARB_multitexture
      glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 16, GL_TEXTURE1_ARB, GL_IDENTITY_NV);
#endif
   }

   glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 20, (float)n[0], (float)n[1], 1, 1);

   glEnable(GL_VERTEX_PROGRAM_NV);  //GL_ENABLE_BIT

#endif
}

/////////////////////////////////////
// done_vertex_program_nv()
/////////////////////////////////////
void nbt_done_vertex_program_nv()
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
void nbt_setup_vertex_program_arb()
{
#ifdef GL_ARB_vertex_program
   NDCpt n(XYpt(1,1));

   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, nbt_bkg_prog_arb); 
   glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 20, (float)n[0], (float)n[1], 1, 1);
   glEnable(GL_VERTEX_PROGRAM_ARB);  //GL_ENABLE_BIT
#endif
}

/////////////////////////////////////
// done_vertex_program_arb()
/////////////////////////////////////
void nbt_done_vertex_program_arb()
{
#ifdef GL_ARB_vertex_program
   glDisable(GL_VERTEX_PROGRAM_ARB);  //GL_ENABLE_BIT
#endif
}

/////////////////////////////////////
// setup_vertex_program()
/////////////////////////////////////
void nbt_setup_vertex_program()
{
   assert(nbt_use_vertex_program);

   if (nbt_use_vertex_program_arb)
   {
      nbt_setup_vertex_program_arb();
   }
   else if (nbt_use_vertex_program_nv)
   {
      nbt_setup_vertex_program_nv();
   }
   else
   {
      assert(0);
   }
}

/////////////////////////////////////
// done_vertex_program()
/////////////////////////////////////
void nbt_done_vertex_program()
{
   assert(nbt_use_vertex_program);

   if (nbt_use_vertex_program_arb)
   {
      nbt_done_vertex_program_arb();
   }
   else if (nbt_use_vertex_program_nv)
   {
      nbt_done_vertex_program_nv();
   }
   else
   {
      assert(0);
   }

}

/**********************************************************************
 * NPRBkgTexCB:
 **********************************************************************/
void 
NPRBkgTexCB::faceCB(CBvert* v, CBface*f) 
{
   static bool debug_vis = Config::get_var_bool("HATCHING_DEBUG_VIS",false,true);

   if (!nbt_use_vertex_program)
   {
      if (debug_vis) 
      {
         HatchingSimplexDataFixed *hsdf = HatchingSimplexDataFixed::find(f);
         if (hsdf)
            glColor4d(0.85, 0.85, 1.0, 1.0); 
         else
            glColor4d(1.0, 1.0, 1.0, 1.0);
      } 

      if (nbt_tex_flag) 
         glTexCoord2dv(NDCZpt(v->wloc()).data());

      if (nbt_paper_flag)
         PaperEffect::paper_coord(NDCZpt(v->wloc()).data());

      glVertex3dv(v->loc().data());
   }
   else
   {
      if (debug_vis) 
      {
         HatchingSimplexDataFixed *hsdf = HatchingSimplexDataFixed::find(f);
         if (hsdf)
            glColor4d(0.85, 0.85, 1.0, 1.0);
         else
            glColor4d(1.0, 1.0, 1.0, 1.0);
      } 

      glVertex3dv(v->loc().data());
   }

}

/**********************************************************************
 * NPRSolidTexture:
 **********************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       NPRBkgTexture::_nbt_tags = 0;


/////////////////////////////////////
// draw()
/////////////////////////////////////
int
NPRBkgTexture::draw(CVIEWptr& v) 
{
   static bool debug_vis = Config::get_var_bool("HATCHING_DEBUG_VIS",false,true);

   if (_ctrl)
      return _ctrl->draw(v);

   nbt_tex_flag   = (v->get_bkg_tex() != NULL);
   nbt_paper_flag = v->get_use_paper();

   if (!nbt_is_init_vertex_program) 
      nbt_init_vertex_program();

   if (nbt_paper_flag)
   {
      //Do anything special?
   }

   glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_HINT_BIT |
                GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT);

   GL_COL(v->color(), v->get_alpha()*alpha());           // GL_CURRENT_BIT
   glDisable(GL_LIGHTING);                               // GL_ENABLE_BIT

   if (debug_vis)
      glShadeModel(GL_FLAT);                             // GL_LIGHTING_BIT

   if (nbt_tex_flag) 
   {
      TEXTUREptr tex = v->get_bkg_tex();
      tex->apply_texture(&v->get_bkg_tf());     // GL_TEXTURE_BIT
   }

	glEnable(GL_BLEND);												// GL_ENABLE_BIT
	if (PaperEffect::is_alpha_premult())
		glBlendFunc(GL_ONE, GL_ZERO);								// GL_COLOR_BUFFER_BIT
	else
		glBlendFunc(GL_SRC_ALPHA, GL_ZERO);						// GL_COLOR_BUFFER_BIT

   PaperEffect::begin_paper_effect(nbt_paper_flag);

   int dl = 0;
   if (nbt_use_vertex_program) 
   {
      // Try it with the display list
      if (BasicTexture::dl_valid(v))
      {
         nbt_setup_vertex_program();                     // GL_ENABLE_BIT, ???
         BasicTexture::draw(v);
         nbt_done_vertex_program();                      // GL_ENABLE_BIT, ???

         PaperEffect::end_paper_effect(nbt_paper_flag);

         glPopAttrib();
         return _patch->num_faces();
      }

      // Failed. Create it.
      dl = _dl.get_dl(v, 1, _patch->stamp());
      if (dl)
         glNewList(dl, GL_COMPILE);
   }

   set_face_culling();                                   // GL_ENABLE_BIT
   _patch->draw_tri_strips(_cb);

   if (nbt_use_vertex_program) 
   {
      // End the display list here
      if (_dl.dl(v)) {
         _dl.close_dl(v);
         // Built it, now execute it
         
         nbt_setup_vertex_program();                     // GL_ENABLE_BIT, ???
         BasicTexture::draw(v);
         nbt_done_vertex_program();                      // GL_ENABLE_BIT, ???
      }
      
   }

   PaperEffect::end_paper_effect(nbt_paper_flag);

   glPopAttrib();

   return _patch->num_faces();
}


/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
NPRBkgTexture::tags() const
{
   if (!_nbt_tags) {
      _nbt_tags = new TAGlist;
      *_nbt_tags += OGLTexture::tags();
      *_nbt_tags += new TAG_meth<NPRBkgTexture>(
         "transparent",
         &NPRBkgTexture::put_transparent,
         &NPRBkgTexture::get_transparent,
         0);
      *_nbt_tags += new TAG_meth<NPRBkgTexture>(
         "annotatable",
         &NPRBkgTexture::put_annotate,
         &NPRBkgTexture::get_annotate,
         0);
   }
   return *_nbt_tags;
}

////////////////////////////////////
// put_transparent()
/////////////////////////////////////
void
NPRBkgTexture::put_transparent(TAGformat &d) const
{
   // XXX - Deprecated
}

/////////////////////////////////////
// get_transparent()
/////////////////////////////////////
void
NPRBkgTexture::get_transparent(TAGformat &d)
{
   err_mesg(ERR_LEV_WARN, "NPRBkgTexture::get_transparent() - ***NOTE: Loading OLD format file!***");

   *d >> _transparent;
}

////////////////////////////////////
// put_annotate()
/////////////////////////////////////
void
NPRBkgTexture::put_annotate(TAGformat &d) const
{
   // XXX - Deprecated
}

/////////////////////////////////////
// get_annotate()
/////////////////////////////////////
void
NPRBkgTexture::get_annotate(TAGformat &d)
{
   err_mesg(ERR_LEV_WARN, "NPRBkgTexture::get_annotate() - ***NOTE: Loading OLD format file!***");

   *d >> _annotate;
}


/* end of file npr_bkg_texture.C */
