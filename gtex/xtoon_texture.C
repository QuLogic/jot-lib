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
#include "gtex/gl_extensions.H"
#include "gtex/paper_effect.H"
#include "geom/gl_view.H"
#include "std/config.H"
#include "xtoon_texture.H"

#include <iostream>

using namespace std;
using namespace mlib;

/**********************************************************************
 * Globals
 **********************************************************************/

bool     ntt_paper_flag;       //If paper's being used
Wpt      ntt_light_pos;        //Used to fetch light pos
Wvec     ntt_light_dir;        //Used to fetch light dir
bool     ntt_positional;       //Used to flag if pos or dir
int      ntt_detail_map = XToonTexture::User;
double   ntt_target_length;
double   ntt_max_factor;
double   ntt_smooth_factor;
int      ntt_smooth_rings = 3;
bool     ntt_smooth_enabled = false;
bool     ntt_elliptic_enabled = false;
bool     ntt_spheric_enabled = false;
bool     ntt_cylindric_enabled = false;
bool     ntt_curv_enabled = false;
Wtransf  ntt_prev_matrix;
Wtransf  ntt_cur_matrix;
bool     ntt_inv_detail = false;

bool     ntt_is_init_vertex_program = false;
bool     ntt_use_vertex_program = false;
bool     ntt_use_vertex_program_arb = false;
bool     ntt_use_vertex_program_nv = false;

GLuint   ntt_toon_prog_nv;
GLuint   ntt_toon_prog_detail_arb, ntt_toon_prog_depth_arb, ntt_toon_prog_focus_arb, ntt_toon_prog_flow_arb;
GLuint   ntt_toon_prog_curvature_arb, ntt_toon_prog_orientation_arb, ntt_toon_prog_specularity_arb;
GLuint   ntt_toon_prog_lookup_arb, ntt_toon_prog_orientation_lookup_arb, ntt_toon_prog_specularity_lookup_arb;

LIST<str_ptr>*    XToonTexture::_toon_texture_names = 0;
LIST<TEXTUREptr>* XToonTexture::_toon_texture_ptrs = 0;
LIST<str_ptr>*    XToonTexture::_toon_texture_remap_orig_names = 0;
LIST<str_ptr>*    XToonTexture::_toon_texture_remap_new_names = 0;

/*****************************************************************
 * Texture Remapping
 *****************************************************************/

char *toon_remap_base = "nprdata/toon_textures/";
char *toon_remap_fnames[][2] = 
{
//   {"dark-8.png",    "1D--dark-8.png"},
//   {"mydot4.png",    "2D--dash-normal-8-32.png"},
//   {"one_d.png",     "1D--gauss-narrow-8.png"},
   {NULL,            NULL}
};

/*****************************************************************
 * Shaders
 *****************************************************************/

const unsigned char NPRToonShaderNV[]=
                        //env[0]-env[3] modelview
                        //env[4]-env[7] projection
                        //env[12]-env[15] texture0
                        //env[16]-env[19] texture1
                        //env[20] xy(1,1) in ndc
                        //env[21] light pos
                        //env[22] light dir
                        //env[23] pos->1,1,1,1 dir->0,0,0,0
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
MOV R6.y,R1.z;\
MOV R6.zw,R11;\
DP4 o[TEX0].x,R6,c[12];\
DP4 o[TEX0].y,R6,c[13];\
MOV o[COL0],v[COL0];\
END"
};




const unsigned char NPRToonShaderDetailARB[]=
                        //env[21] light pos
                        //env[22] light dir
                        //env[23] pos->1,1,1,1 dir->0,0,0,0
                        //env[24] target length
                        //env[25] max factor
                        //env[26] smooth factor
{
"!!ARBvp1.0\n\
\
ATTRIB iPos          = vertex.position;\
ATTRIB iNorm         = vertex.normal;\
ATTRIB iCol          = vertex.color;\
ATTRIB iSmoothedNorm = vertex.attrib[1];\
\
OUTPUT oPos  = result.position;\
OUTPUT oCol  = result.color;\
OUTPUT oTex0 = result.texcoord[0];\
OUTPUT oTex1 = result.texcoord[1];\
OUTPUT oTex2 = result.texcoord[2];\
\
PARAM  cZero      = {0.0,0.0,0.0,0.0};\
PARAM  cOne       = {1.0,1.0,1.0,1.0};\
PARAM  cLPos      = program.env[21];\
PARAM  cLDir      = program.env[22];\
PARAM  cLPosFlag  = program.env[23];\
PARAM  cTargetLen = program.env[24];\
PARAM  cMaxFactor = program.env[25];\
PARAM  cSmooth    = program.env[26];\
\
PARAM  mMVP[4]  =  { state.matrix.mvp };\
PARAM  mTex0[4] =  { state.matrix.texture[0] };\
PARAM  mTex1[4] =  { state.matrix.texture[1] };\
PARAM  mTex2[4] =  { state.matrix.texture[2] };\
\
TEMP   tXY;\
TEMP   tLDirFlag;\
TEMP   tLPosVec;\
TEMP   tLFinalVec;\
TEMP   tDetail;\
TEMP   tNorm;\
\
DP4 tXY.x, mMVP[0], iPos;\
DP4 tXY.y, mMVP[1], iPos;\
DP4 tXY.z, mMVP[2], iPos;\
DP4 tXY.w, mMVP[3], iPos;\
""\
SGE tLDirFlag,cZero,cLPosFlag;\
\
SUB tLPosVec,  cLPos,   iPos;\
DP3 tLPosVec.w,tLPosVec, tLPosVec;\
RSQ tLPosVec.w,tLPosVec.w;\
MUL tLPosVec,  tLPosVec, tLPosVec.w;\
\
MUL tLFinalVec,cLPosFlag,tLPosVec;\
MAD tLFinalVec,tLDirFlag,cLDir,tLFinalVec;\
\
MOV tDetail, cTargetLen;\
\
DP4 oTex0.x, mTex0[0], tLFinalVec;\
DP4 oTex0.y, mTex0[1], tLFinalVec;\
DP4 oTex0.z, mTex0[2], tLFinalVec;\
DP4 oTex0.w, mTex0[3], tLFinalVec;\
\
DP4 oTex1.x, mTex1[0], iNorm;\
DP4 oTex1.y, mTex1[1], iNorm;\
DP4 oTex1.z, mTex1[2], iNorm;\
DP4 oTex1.w, mTex1[3], iNorm;\
\
DP4 oTex2.x, mTex2[0], tDetail;\
DP4 oTex2.y, mTex2[1], tDetail;\
DP4 oTex2.z, mTex2[2], tDetail;\
DP4 oTex2.w, mTex2[3], tDetail;\
\
MOV oPos, tXY;\
MOV oCol, iCol;\
\
END"
};


const unsigned char NPRToonShaderDepthARB[]=
                        //env[20] xy(1,1) in ndc
                        //env[21] light pos
                        //env[22] light dir
                        //env[23] pos->1,1,1,1 dir->0,0,0,0
                        //env[24] target length
                        //env[25] max factor
                        //env[26] smooth factor
{
"!!ARBvp1.0\n\
\
ATTRIB iPos          = vertex.position;\
ATTRIB iNorm         = vertex.normal;\
ATTRIB iCol          = vertex.color;\
ATTRIB iSmoothedNorm = vertex.attrib[1];\
\
OUTPUT oPos  = result.position;\
OUTPUT oCol  = result.color;\
OUTPUT oTex0 = result.texcoord[0];\
OUTPUT oTex1 = result.texcoord[1];\
OUTPUT oTex2 = result.texcoord[2];\
\
PARAM  cZero      = {0.0,0.0,0.0,0.0};\
PARAM  cOne       = {1.0,1.0,1.0,1.0};\
PARAM  cLPos      = program.env[21];\
PARAM  cLDir      = program.env[22];\
PARAM  cLPosFlag  = program.env[23];\
PARAM  cTargetLen = program.env[24];\
PARAM  cMaxFactor = program.env[25];\
PARAM  cSmooth    = program.env[26];\
\
PARAM  mMV[4]   =  { state.matrix.modelview };\
PARAM  mMVP[4]  =  { state.matrix.mvp };\
PARAM  mTex0[4] =  { state.matrix.texture[0] };\
PARAM  mTex1[4] =  { state.matrix.texture[1] };\
PARAM  mTex2[4] =  { state.matrix.texture[2] };\
\
TEMP   tEye;\
TEMP   tRate;\
TEMP   tXY;\
TEMP   tLDirFlag;\
TEMP   tLPosVec;\
TEMP   tLFinalVec;\
TEMP   tDetail;\
TEMP   tNorm;\
\
DP4 tEye.x, mMV[0], iPos;\
DP4 tEye.y, mMV[1], iPos;\
DP4 tEye.z, mMV[2], iPos;\
DP4 tEye.w, mMV[3], iPos;\
\
DP4 tXY.x, mMVP[0], iPos;\
DP4 tXY.y, mMVP[1], iPos;\
DP4 tXY.z, mMVP[2], iPos;\
DP4 tXY.w, mMVP[3], iPos;\
""\
SGE tLDirFlag,cZero,cLPosFlag;\
\
SUB tLPosVec,  cLPos,   iPos;\
DP3 tLPosVec.w,tLPosVec, tLPosVec;\
RSQ tLPosVec.w,tLPosVec.w;\
MUL tLPosVec,  tLPosVec, tLPosVec.w;\
\
MUL tLFinalVec,cLPosFlag,tLPosVec;\
MAD tLFinalVec,tLDirFlag,cLDir,tLFinalVec;\
\
LOG tRate,cMaxFactor.x;\
RCP tRate,tRate.z;\
RCP tDetail,cTargetLen.x;\
MUL tDetail,tDetail,-tEye.z;\
LOG tDetail,tDetail.x;\
MUL tDetail,tDetail.z,tRate;\
MIN tDetail, tDetail, cOne;\
MAX tDetail, tDetail, cZero;\
\
SUB tNorm,iSmoothedNorm,iNorm;\
MUL tNorm,tNorm,cSmooth.x;\
ADD tNorm,tNorm,iNorm;\
DP3 tNorm.w,tNorm,tNorm;\
RSQ tNorm.w,tNorm.w;\
MUL tNorm,tNorm,tNorm.w;\
\
DP4 oTex0.x, mTex0[0], tLFinalVec;\
DP4 oTex0.y, mTex0[1], tLFinalVec;\
DP4 oTex0.z, mTex0[2], tLFinalVec;\
DP4 oTex0.w, mTex0[3], tLFinalVec;\
\
DP4 oTex1.x, mTex1[0], tNorm;\
DP4 oTex1.y, mTex1[1], tNorm;\
DP4 oTex1.z, mTex1[2], tNorm;\
DP4 oTex1.w, mTex1[3], tNorm;\
\
DP4 oTex2.x, mTex2[0], tDetail;\
DP4 oTex2.y, mTex2[1], tDetail;\
DP4 oTex2.z, mTex2[2], tDetail;\
DP4 oTex2.w, mTex2[3], tDetail;\
\
MOV oPos, tXY;\
MOV oCol, iCol;\
\
END"
};

const unsigned char NPRToonShaderFocusARB[]=
                        //env[20] xy(1,1) in ndc
                        //env[21] light pos
                        //env[22] light dir
                        //env[23] pos->1,1,1,1 dir->0,0,0,0
                        //env[24] target length
                        //env[25] max factor
                        //env[26] smooth factor
{
"!!ARBvp1.0\n\
\
ATTRIB iPos          = vertex.position;\
ATTRIB iNorm         = vertex.normal;\
ATTRIB iCol          = vertex.color;\
ATTRIB iSmoothedNorm = vertex.attrib[1];\
\
OUTPUT oPos  = result.position;\
OUTPUT oCol  = result.color;\
OUTPUT oTex0 = result.texcoord[0];\
OUTPUT oTex1 = result.texcoord[1];\
OUTPUT oTex2 = result.texcoord[2];\
\
PARAM  cZero      = {0.0,0.0,0.0,0.0};\
PARAM  cOne       = {1.0,1.0,1.0,1.0};\
PARAM  cLPos      = program.env[21];\
PARAM  cLDir      = program.env[22];\
PARAM  cLPosFlag  = program.env[23];\
PARAM  cTargetLen = program.env[24];\
PARAM  cMaxFactor = program.env[25];\
PARAM  cSmooth    = program.env[26];\
\
PARAM  mMVIT[4]   =  { state.matrix.modelview.invtrans };\
PARAM  mMVP[4]  =  { state.matrix.mvp };\
PARAM  mTex0[4] =  { state.matrix.texture[0] };\
PARAM  mTex1[4] =  { state.matrix.texture[1] };\
PARAM  mTex2[4] =  { state.matrix.texture[2] };\
PARAM  mFocus[4]=  { state.matrix.program[0] };\
\
TEMP   tEye;\
TEMP   tFocus;\
TEMP   tRate;\
TEMP   tXY;\
TEMP   tLDirFlag;\
TEMP   tLPosVec;\
TEMP   tLFinalVec;\
TEMP   tDetail;\
TEMP   tNorm;\
TEMP   tDist;\
TEMP   tInf;\
TEMP   tSup;\
TEMP   tSign;\
TEMP   zMin;\
TEMP   zMax;\
TEMP   tDinf;\
TEMP   tDsup;\
\
DP4 tXY.x, mMVP[0], iPos;\
DP4 tXY.y, mMVP[1], iPos;\
DP4 tXY.z, mMVP[2], iPos;\
DP4 tXY.w, mMVP[3], iPos;\
""\
SGE tLDirFlag,cZero,cLPosFlag;\
\
SUB tLPosVec,  cLPos,   iPos;\
DP3 tLPosVec.w,tLPosVec, tLPosVec;\
RSQ tLPosVec.w,tLPosVec.w;\
MUL tLPosVec,  tLPosVec, tLPosVec.w;\
\
MUL tLFinalVec,cLPosFlag,tLPosVec;\
MAD tLFinalVec,tLDirFlag,cLDir,tLFinalVec;\
\
SUB tEye, mMVIT[3], iPos;\
DP3 tEye.x, tEye, tEye;\
POW tEye.x, tEye.x, 0.5;\
\
SGE tInf, mFocus[0].x, tEye.x;\
MAD tSign, 2.0, -tInf, 1.0;\
MUL zMin, tSign, cTargetLen.x;\
MUL zMax, zMin, cMaxFactor.x;\
ADD zMin, mFocus[0].x, zMin;\
ADD zMax, mFocus[0].x, zMax;\
\
RCP tRate, zMax;\
MUL tDinf, tRate, tEye.x;\
LOG tDinf, tDinf;\
MOV tDinf, tDinf.z;\
MUL tRate, tRate, zMin;\
LOG tRate, tRate;\
RCP tRate, tRate.z;\
MUL tDinf, tRate, tDinf;\
SUB tDinf, cOne,  tDinf;\
\
RCP tRate, zMin;\
MUL tDsup, tRate, tEye.x;\
LOG tDsup, tDsup;\
MOV tDsup, tDsup.z;\
MUL tRate, tRate, zMax;\
LOG tRate, tRate;\
RCP tRate, tRate.z;\
MUL tDsup, tRate, tDsup;\
\
MUL tDinf, tDinf, tInf;\
SUB tSup,  cOne,  tInf;\
MUL tDsup, tDsup, tSup;\
ADD tDetail, tDsup, tDinf;\
MIN tDetail, tDetail, cOne;\
MAX tDetail, tDetail, cZero;\
\
SUB tNorm,iSmoothedNorm,iNorm;\
MUL tNorm,tNorm,cSmooth.x;\
ADD tNorm,tNorm,iNorm;\
DP3 tNorm.w,tNorm,tNorm;\
RSQ tNorm.w,tNorm.w;\
MUL tNorm,tNorm,tNorm.w;\
\
DP4 oTex0.x, mTex0[0], tLFinalVec;\
DP4 oTex0.y, mTex0[1], tLFinalVec;\
DP4 oTex0.z, mTex0[2], tLFinalVec;\
DP4 oTex0.w, mTex0[3], tLFinalVec;\
\
DP4 oTex1.x, mTex1[0], tNorm;\
DP4 oTex1.y, mTex1[1], tNorm;\
DP4 oTex1.z, mTex1[2], tNorm;\
DP4 oTex1.w, mTex1[3], tNorm;\
\
DP4 oTex2.x, mTex2[0], tDetail;\
DP4 oTex2.y, mTex2[1], tDetail;\
DP4 oTex2.z, mTex2[2], tDetail;\
DP4 oTex2.w, mTex2[3], tDetail;\
\
MOV oPos, tXY;\
MOV oCol, iCol;\
\
END"
};

/*
// MUL zMax, zMin, cMaxFactor.x;\



// \
// LOG tRate,cMaxFactor.x;\
// RCP tRate,tRate.z;\
// RCP tDetail,cTargetLen.x;\
// SUB tEye, tEye.x, mFocus[0].x;\
// MAX tEye, tEye, -tEye;\
// MUL tDetail,tDetail,tEye;\
// LOG tDetail,tDetail.x;\
// MUL tDetail,tDetail.z,tRate;\
// MIN tDetail, tDetail, cOne;\
// MAX tDetail, tDetail, cZero;\
*/
const unsigned char NPRToonShaderFlowARB[]=
                        //env[20] xy(1,1) in ndc
                        //env[21] light pos
                        //env[22] light dir
                        //env[23] pos->1,1,1,1 dir->0,0,0,0
                        //env[24] target length
                        //env[25] max factor
                        //env[26] smooth factor
{
"!!ARBvp1.0\n\
\
ATTRIB iPos          = vertex.position;\
ATTRIB iNorm         = vertex.normal;\
ATTRIB iCol          = vertex.color;\
ATTRIB iSmoothedNorm = vertex.attrib[1];\
\
OUTPUT oPos  = result.position;\
OUTPUT oCol  = result.color;\
OUTPUT oTex0 = result.texcoord[0];\
OUTPUT oTex1 = result.texcoord[1];\
OUTPUT oTex2 = result.texcoord[2];\
\
PARAM  cZero      = {0.0,0.0,0.0,0.0};\
PARAM  cOne       = {1.0,1.0,1.0,1.0};\
PARAM  cLPos      = program.env[21];\
PARAM  cLDir      = program.env[22];\
PARAM  cLPosFlag  = program.env[23];\
PARAM  cTargetLen = program.env[24];\
PARAM  cMaxFactor = program.env[25];\
PARAM  cSmooth    = program.env[26];\
\
PARAM  mMV[4]   =  { state.matrix.modelview };\
PARAM  mMVP[4]  =  { state.matrix.mvp };\
PARAM  mTex0[4] =  { state.matrix.texture[0] };\
PARAM  mTex1[4] =  { state.matrix.texture[1] };\
PARAM  mTex2[4] =  { state.matrix.texture[2] };\
PARAM  mPrevM[4]=  { state.matrix.program[0] };\
PARAM  mCurM[4] =  { state.matrix.program[1] };\
\
TEMP   tXY;\
TEMP   tLDirFlag;\
TEMP   tLPosVec;\
TEMP   tLFinalVec;\
TEMP   tPrevPix;\
TEMP   tCurPix;\
TEMP   tDetail;\
TEMP   tNorm;\
\
DP4 tXY.x, mMVP[0], iPos;\
DP4 tXY.y, mMVP[1], iPos;\
DP4 tXY.z, mMVP[2], iPos;\
DP4 tXY.w, mMVP[3], iPos;\
""\
SGE tLDirFlag,cZero,cLPosFlag;\
\
SUB tLPosVec,  cLPos,   iPos;\
DP3 tLPosVec.w,tLPosVec, tLPosVec;\
RSQ tLPosVec.w,tLPosVec.w;\
MUL tLPosVec,  tLPosVec, tLPosVec.w;\
\
MUL tLFinalVec,cLPosFlag,tLPosVec;\
MAD tLFinalVec,tLDirFlag,cLDir,tLFinalVec;\
\
DP4 tPrevPix.x, mPrevM[0], iPos;\
DP4 tPrevPix.y, mPrevM[1], iPos;\
DP4 tPrevPix.z, mPrevM[2], iPos;\
DP4 tPrevPix.w, mPrevM[3], iPos;\
RCP tPrevPix.w, tPrevPix.w;\
MUL tPrevPix, tPrevPix, tPrevPix.w;\
DP4 tCurPix.x, mCurM[0], iPos;\
DP4 tCurPix.y, mCurM[1], iPos;\
DP4 tCurPix.z, mCurM[2], iPos;\
DP4 tCurPix.w, mCurM[3], iPos;\
RCP tCurPix.w, tCurPix.w;\
MUL tCurPix, tCurPix, tCurPix.w;\
SUB tDetail, tCurPix, tPrevPix;\
DP3 tDetail.x, tDetail, tDetail;\
POW tDetail, tDetail.x, 0.5;\
SUB tNorm, cMaxFactor.x, cOne;\
MUL tNorm, tNorm, cTargetLen.x;\
RCP tNorm, tNorm.x;\
SUB tDetail, tDetail, cTargetLen.x;\
MUL tDetail, tDetail, tNorm;\
MIN tDetail, tDetail, cOne;\
MAX tDetail, cZero, tDetail;\
\
SUB tNorm,iSmoothedNorm,iNorm;\
MUL tNorm,tNorm,cSmooth.x;\
ADD tNorm,tNorm,iNorm;\
DP3 tNorm.w,tNorm,tNorm;\
RSQ tNorm.w,tNorm.w;\
MUL tNorm,tNorm,tNorm.w;\
\
DP4 oTex0.x, mTex0[0], tLFinalVec;\
DP4 oTex0.y, mTex0[1], tLFinalVec;\
DP4 oTex0.z, mTex0[2], tLFinalVec;\
DP4 oTex0.w, mTex0[3], tLFinalVec;\
\
DP4 oTex1.x, mTex1[0], tNorm;\
DP4 oTex1.y, mTex1[1], tNorm;\
DP4 oTex1.z, mTex1[2], tNorm;\
DP4 oTex1.w, mTex1[3], tNorm;\
\
DP4 oTex2.x, mTex2[0], tDetail;\
DP4 oTex2.y, mTex2[1], tDetail;\
DP4 oTex2.z, mTex2[2], tDetail;\
DP4 oTex2.w, mTex2[3], tDetail;\
\
MOV oPos, tXY;\
MOV oCol, iCol;\
\
END"
};




const unsigned char NPRToonShaderCurvatureARB[]=
                        //env[20] xy(1,1) in ndc
                        //env[21] light pos
                        //env[22] light dir
                        //env[23] pos->1,1,1,1 dir->0,0,0,0
                        //env[24] target length
                        //env[25] max factor
                        //env[26] smooth factor
{
"!!ARBvp1.0\n\
\
ATTRIB iPos          = vertex.position;\
ATTRIB iNorm         = vertex.normal;\
ATTRIB iCol          = vertex.color;\
ATTRIB iSmoothedNorm = vertex.attrib[1];\
ATTRIB iCurv         = vertex.attrib[5];\
\
OUTPUT oPos  = result.position;\
OUTPUT oCol  = result.color;\
OUTPUT oTex0 = result.texcoord[0];\
OUTPUT oTex1 = result.texcoord[1];\
OUTPUT oTex2 = result.texcoord[2];\
\
PARAM  cZero      = {0.0,0.0,0.0,0.0};\
PARAM  cOne       = {1.0,1.0,1.0,1.0};\
PARAM  cLPos      = program.env[21];\
PARAM  cLDir      = program.env[22];\
PARAM  cLPosFlag  = program.env[23];\
PARAM  cTargetLen = program.env[24];\
PARAM  cMaxFactor = program.env[25];\
PARAM  cSmooth    = program.env[26];\
\
PARAM  mMVP[4]  =  { state.matrix.mvp };\
PARAM  mTex0[4] =  { state.matrix.texture[0] };\
PARAM  mTex1[4] =  { state.matrix.texture[1] };\
PARAM  mTex2[4] =  { state.matrix.texture[2] };\
\
TEMP   tXY;\
TEMP   tLDirFlag;\
TEMP   tLPosVec;\
TEMP   tLFinalVec;\
TEMP   tDetail;\
TEMP   tNorm;\
\
DP4 tXY.x, mMVP[0], iPos;\
DP4 tXY.y, mMVP[1], iPos;\
DP4 tXY.z, mMVP[2], iPos;\
DP4 tXY.w, mMVP[3], iPos;\
""\
SGE tLDirFlag,cZero,cLPosFlag;\
\
SUB tLPosVec,  cLPos,   iPos;\
DP3 tLPosVec.w,tLPosVec, tLPosVec;\
RSQ tLPosVec.w,tLPosVec.w;\
MUL tLPosVec,  tLPosVec, tLPosVec.w;\
\
MUL tLFinalVec,cLPosFlag,tLPosVec;\
MAD tLFinalVec,tLDirFlag,cLDir,tLFinalVec;\
""\
RCP tDetail, cTargetLen.x;\
MUL tDetail, iCurv.z, tDetail;\
SUB tDetail, cOne, tDetail;\
MAX tDetail, tDetail, cZero;\
MIN tDetail, tDetail, cOne;\
\
SUB tNorm,iSmoothedNorm,iNorm;\
MUL tNorm,tNorm,cSmooth.x;\
ADD tNorm,tNorm,iNorm;\
DP3 tNorm.w,tNorm,tNorm;\
RSQ tNorm.w,tNorm.w;\
MUL tNorm,tNorm,tNorm.w;\
\
DP4 oTex0.x, mTex0[0], tLFinalVec;\
DP4 oTex0.y, mTex0[1], tLFinalVec;\
DP4 oTex0.z, mTex0[2], tLFinalVec;\
DP4 oTex0.w, mTex0[3], tLFinalVec;\
\
DP4 oTex1.x, mTex1[0], tNorm;\
DP4 oTex1.y, mTex1[1], tNorm;\
DP4 oTex1.z, mTex1[2], tNorm;\
DP4 oTex1.w, mTex1[3], tNorm;\
\
DP4 oTex2.x, mTex2[0], tDetail;\
DP4 oTex2.y, mTex2[1], tDetail;\
DP4 oTex2.z, mTex2[2], tDetail;\
DP4 oTex2.w, mTex2[3], tDetail;\
\
MOV oPos, tXY;\
MOV oCol, iCol;\
\
END"
};





const unsigned char NPRToonShaderOrientationARB[]=
                        //env[20] xy(1,1) in ndc
                        //env[21] light pos
                        //env[22] light dir
                        //env[23] pos->1,1,1,1 dir->0,0,0,0
                        //env[24] target length
                        //env[25] max factor
                        //env[26] smooth factor
{
"!!ARBvp1.0\n\
\
ATTRIB iPos          = vertex.position;\
ATTRIB iNorm         = vertex.normal;\
ATTRIB iCol          = vertex.color;\
ATTRIB iSmoothedNorm = vertex.attrib[1];\
\
OUTPUT oPos  = result.position;\
OUTPUT oCol  = result.color;\
OUTPUT oTex0 = result.texcoord[0];\
OUTPUT oTex1 = result.texcoord[1];\
OUTPUT oTex2 = result.texcoord[2];\
\
PARAM  cZero      = {0.0,0.0,0.0,0.0};\
PARAM  cOne       = {1.0,1.0,1.0,1.0};\
PARAM  cLPos      = program.env[21];\
PARAM  cLDir      = program.env[22];\
PARAM  cLPosFlag  = program.env[23];\
PARAM  cTargetLen = program.env[24];\
PARAM  cMaxFactor = program.env[25];\
PARAM  cSmooth    = program.env[26];\
\
PARAM  mMVP[4]  =  { state.matrix.mvp };\
PARAM  mMVIT[4] =  { state.matrix.modelview.invtrans };\
PARAM  mTex0[4] =  { state.matrix.texture[0] };\
PARAM  mTex1[4] =  { state.matrix.texture[1] };\
PARAM  mTex2[4] =  { state.matrix.texture[2] };\
\
TEMP   tXY;\
TEMP   tLDirFlag;\
TEMP   tLPosVec;\
TEMP   tLFinalVec;\
TEMP   tView;\
TEMP   tNorm;\
\
DP4 tXY.x, mMVP[0], iPos;\
DP4 tXY.y, mMVP[1], iPos;\
DP4 tXY.z, mMVP[2], iPos;\
DP4 tXY.w, mMVP[3], iPos;\
""\
SGE tLDirFlag,cZero,cLPosFlag;\
\
SUB tLPosVec,  cLPos,   iPos;\
DP3 tLPosVec.w,tLPosVec, tLPosVec;\
RSQ tLPosVec.w,tLPosVec.w;\
MUL tLPosVec,  tLPosVec, tLPosVec.w;\
\
MUL tLFinalVec,cLPosFlag,tLPosVec;\
MAD tLFinalVec,tLDirFlag,cLDir,tLFinalVec;\
\
SUB tView, mMVIT[3], iPos;\
DP3 tView.w, tView, tView;\
RSQ tView.w, tView.w;\
MUL tView, tView, tView.w;\
\
SUB tNorm,iSmoothedNorm,iNorm;\
MUL tNorm,tNorm,cSmooth.x;\
ADD tNorm,tNorm,iNorm;\
DP3 tNorm.w,tNorm,tNorm;\
RSQ tNorm.w,tNorm.w;\
MUL tNorm,tNorm,tNorm.w;\
\
DP4 oTex0.x, mTex0[0], tLFinalVec;\
DP4 oTex0.y, mTex0[1], tLFinalVec;\
DP4 oTex0.z, mTex0[2], tLFinalVec;\
DP4 oTex0.w, mTex0[3], tLFinalVec;\
\
DP4 oTex1.x, mTex1[0], tNorm;\
DP4 oTex1.y, mTex1[1], tNorm;\
DP4 oTex1.z, mTex1[2], tNorm;\
DP4 oTex1.w, mTex1[3], tNorm;\
\
DP4 oTex2.x, mTex2[0], tView;\
DP4 oTex2.y, mTex2[1], tView;\
DP4 oTex2.z, mTex2[2], tView;\
DP4 oTex2.w, mTex2[3], tView;\
\
MOV oPos, tXY;\
MOV oCol, iCol;\
\
END"
};


const unsigned char NPRToonShaderSpecularityARB[]=
                        //env[20] xy(1,1) in ndc
                        //env[21] light pos
                        //env[22] light dir
                        //env[23] pos->1,1,1,1 dir->0,0,0,0
                        //env[24] target length
                        //env[25] max factor
                        //env[26] smooth factor
{
"!!ARBvp1.0\n\
\
ATTRIB iPos          = vertex.position;\
ATTRIB iNorm         = vertex.normal;\
ATTRIB iCol          = vertex.color;\
ATTRIB iSmoothedNorm = vertex.attrib[1];\
\
OUTPUT oPos  = result.position;\
OUTPUT oCol  = result.color;\
OUTPUT oTex0 = result.texcoord[0];\
OUTPUT oTex1 = result.texcoord[1];\
OUTPUT oTex2 = result.texcoord[2];\
\
PARAM  cZero      = {0.0,0.0,0.0,0.0};\
PARAM  cOne       = {1.0,1.0,1.0,1.0};\
PARAM  cLPos      = program.env[21];\
PARAM  cLDir      = program.env[22];\
PARAM  cLPosFlag  = program.env[23];\
PARAM  cTargetLen = program.env[24];\
PARAM  cMaxFactor = program.env[25];\
PARAM  cSmooth    = program.env[26];\
\
PARAM  mMVP[4]  =  { state.matrix.mvp };\
PARAM  mMVIT[4] =  { state.matrix.modelview.invtrans };\
PARAM  mTex0[4] =  { state.matrix.texture[0] };\
PARAM  mTex1[4] =  { state.matrix.texture[1] };\
PARAM  mTex2[4] =  { state.matrix.texture[2] };\
\
TEMP   tXY;\
TEMP   tLDirFlag;\
TEMP   tLPosVec;\
TEMP   tLFinalVec;\
TEMP   tView;\
TEMP   tNorm;\
\
DP4 tXY.x, mMVP[0], iPos;\
DP4 tXY.y, mMVP[1], iPos;\
DP4 tXY.z, mMVP[2], iPos;\
DP4 tXY.w, mMVP[3], iPos;\
""\
SGE tLDirFlag,cZero,cLPosFlag;\
\
SUB tLPosVec,  cLPos,   iPos;\
DP3 tLPosVec.w,tLPosVec, tLPosVec;\
RSQ tLPosVec.w,tLPosVec.w;\
MUL tLPosVec,  tLPosVec, tLPosVec.w;\
\
MUL tLFinalVec,cLPosFlag,tLPosVec;\
MAD tLFinalVec,tLDirFlag,cLDir,tLFinalVec;\
\
SUB tView, mMVIT[3], iPos;\
DP3 tView.w, tView, tView;\
RSQ tView.w, tView.w;\
MUL tView, tView, tView.w;\
\
SUB tNorm,iSmoothedNorm,iNorm;\
MUL tNorm,tNorm,cSmooth.x;\
ADD tNorm,tNorm,iNorm;\
DP3 tNorm.w,tNorm,tNorm;\
RSQ tNorm.w,tNorm.w;\
MUL tNorm,tNorm,tNorm.w;\
\
DP4 oTex0.x, mTex0[0], tLFinalVec;\
DP4 oTex0.y, mTex0[1], tLFinalVec;\
DP4 oTex0.z, mTex0[2], tLFinalVec;\
DP4 oTex0.w, mTex0[3], tLFinalVec;\
\
DP4 oTex1.x, mTex1[0], tNorm;\
DP4 oTex1.y, mTex1[1], tNorm;\
DP4 oTex1.z, mTex1[2], tNorm;\
DP4 oTex1.w, mTex1[3], tNorm;\
\
DP4 oTex2.x, mTex2[0], tView;\
DP4 oTex2.y, mTex2[1], tView;\
DP4 oTex2.z, mTex2[2], tView;\
DP4 oTex2.w, mTex2[3], tView;\
\
MOV oPos, tXY;\
MOV oCol, iCol;\
\
END"
};



const unsigned char NPRToonShaderLookupARB[]=
                        //env[20] inverse detail
{
"!!ARBfp1.0\n\
\
ATTRIB iLight     = fragment.texcoord[0];\
ATTRIB iNormal    = fragment.texcoord[1];\
ATTRIB iDetail    = fragment.texcoord[2];\
\
PARAM  cZero      = {0.0,0.0,0.0,0.0};\
PARAM  cOne       = {1.0,1.0,1.0,1.0};\
PARAM  cInvDetail = program.env[20];\
\
OUTPUT oCol  = result.color;\
\
TEMP tLight;\
TEMP tNormal;\
TEMP tDetail;\
TEMP toonCoords;\
\
MOV tLight, iLight;\
DP3 tLight.w, tLight, tLight;\
RCP tLight.w, tLight.w;\
MUL tLight, tLight, tLight.w;\
\
MOV tNormal, iNormal;\
DP3 tNormal.w, tNormal, tNormal;\
RCP tNormal.w, tNormal.w;\
MUL tNormal, tNormal, tNormal.w;\
\
SUB tDetail, iDetail.x, cInvDetail.x;\
MAX tDetail, tDetail, -tDetail;\
\
DP3 toonCoords.x, tLight, tNormal;\
MAX toonCoords.x, toonCoords.x, cZero;\
MOV toonCoords.y, tDetail.x;\
MOV toonCoords.z, cZero;\
MOV toonCoords.w, cOne;\
\
TEX oCol, toonCoords, texture[0], 2D;\
\
END"
};


const unsigned char NPRToonShaderOrientationLookupARB[]=
                        //env[20] inverse detail
                        //env[21] target length
{
"!!ARBfp1.0\n\
\
ATTRIB iLight  = fragment.texcoord[0];\
ATTRIB iNormal = fragment.texcoord[1];\
ATTRIB iView   = fragment.texcoord[2];\
\
PARAM  cZero      = {0.0,0.0,0.0,0.0};\
PARAM  cOne       = {1.0,1.0,1.0,1.0};\
PARAM  cInvDetail = program.env[20];\
PARAM  cTargetLen = program.env[21];\
\
OUTPUT oCol  = result.color;\
\
TEMP tLight;\
TEMP tNormal;\
TEMP tView;\
TEMP tDetail;\
TEMP toonCoords;\
\
MOV tLight, iLight;\
DP3 tLight.w, tLight, tLight;\
RSQ tLight.w, tLight.w;\
MUL tLight, tLight, tLight.w;\
\
MOV tNormal, iNormal;\
DP3 tNormal.w, tNormal, tNormal;\
RSQ tNormal.w, tNormal.w;\
MUL tNormal, tNormal, tNormal.w;\
\
MOV tView, iView;\
DP3 tView.w, tView, tView;\
RSQ tView.w, tView.w;\
MUL tView, tView, tView.w;\
\
DP3 tDetail, tView, tNormal;\
POW tDetail, tDetail.x, cTargetLen.x;\
SUB tDetail, cOne, tDetail;\
MAX tDetail, tDetail, cZero;\
MIN tDetail, tDetail, cOne;\
SUB tDetail, tDetail.x, cInvDetail.x;\
MAX tDetail, tDetail, -tDetail;\
\
DP3 toonCoords.x, tLight, tNormal;\
MAX toonCoords.x, toonCoords.x, cZero;\
MOV toonCoords.y, tDetail.x;\
MOV toonCoords.z, cZero;\
MOV toonCoords.w, cOne;\
\
TEX oCol, toonCoords, texture[0], 2D;\
\
END"
};


const unsigned char NPRToonShaderSpecularityLookupARB[]=
                        //env[20] inverse detail
                        //env[21] target length
{
"!!ARBfp1.0\n\
\
ATTRIB iLight  = fragment.texcoord[0];\
ATTRIB iNormal = fragment.texcoord[1];\
ATTRIB iView   = fragment.texcoord[2];\
\
PARAM  cZero      = {0.0,0.0,0.0,0.0};\
PARAM  cOne       = {1.0,1.0,1.0,1.0};\
PARAM  cHalf      = {0.5,0.5,0.5,0.5};\
PARAM  cInvDetail = program.env[20];\
PARAM  cTargetLen = program.env[21];\
\
OUTPUT oCol  = result.color;\
\
TEMP tLight;\
TEMP tNormal;\
TEMP tView;\
TEMP tHalf;\
TEMP tDetail;\
TEMP toonCoords;\
\
MOV tLight, iLight;\
DP3 tLight.w, tLight, tLight;\
RSQ tLight.w, tLight.w;\
MUL tLight, tLight, tLight.w;\
\
MOV tNormal, iNormal;\
DP3 tNormal.w, tNormal, tNormal;\
RSQ tNormal.w, tNormal.w;\
MUL tNormal, tNormal, tNormal.w;\
\
MOV tView, iView;\
DP3 tView.w, tView, tView;\
RSQ tView.w, tView.w;\
MUL tView, tView, tView.w;\
\
ADD tHalf, tView, tLight;\
DP3 tHalf.w, tHalf, tHalf;\
RSQ tHalf.w, tHalf.w;\
MUL tHalf, tHalf, tHalf.w;\
\
MOV tDetail, cZero;\
DP3 tDetail.x, tHalf, tNormal;\
MAX tDetail, tDetail, cZero;\
POW tDetail, tDetail.x, cTargetLen.x;\
SUB tDetail, cOne, tDetail.x;\
MAX tDetail, tDetail, cZero;\
MIN tDetail, tDetail, cOne;\
SUB tDetail, tDetail.x, cInvDetail.x;\
MAX tDetail, tDetail, -tDetail;\
\
DP3 toonCoords.x, tLight, tNormal;\
MAX toonCoords.x, toonCoords.x, cZero;\
MOV toonCoords.y, tDetail.x;\
MOV toonCoords.z, cZero;\
MOV toonCoords.w, cOne;\
\
TEX oCol, toonCoords, texture[0], 2D;\
\
END"
};




/////////////////////////////////////
// init_vertex_program_nv()
/////////////////////////////////////

bool ntt_init_vertex_program_nv()
{
#ifdef NON_NVIDIA_GFX
  return false;
#else
  if (GLExtensions::gl_nv_vertex_program_supported())
    {
      err_mesg(ERR_LEV_INFO, "XToonTexture::init() - Can use NV vertex programs!");
#ifdef GL_NV_vertex_program
      glGenProgramsNV(1, &ntt_toon_prog_nv); 
      glBindProgramNV(GL_VERTEX_PROGRAM_NV, ntt_toon_prog_nv); 
      glLoadProgramNV(GL_VERTEX_PROGRAM_NV, ntt_toon_prog_nv, strlen((char * ) NPRToonShaderNV), NPRToonShaderNV);
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

bool ntt_init_vertex_program_arb()
{
   assert(!ntt_is_init_vertex_program);

   if (GLExtensions::gl_arb_vertex_program_supported()) {
     bool success = false, native = false;
     err_mesg(ERR_LEV_INFO, "XToonTexture::init() - Can use ARB vertex programs!");
#ifdef GL_ARB_vertex_program
     glGenProgramsARB(1, &ntt_toon_prog_detail_arb); 
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_detail_arb); 
     glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
			strlen((char * ) NPRToonShaderDetailARB), NPRToonShaderDetailARB);
     success = GLExtensions::gl_arb_vertex_program_loaded("XToonTexture::init() - ", native, NPRToonShaderDetailARB);
     
     glGenProgramsARB(1, &ntt_toon_prog_depth_arb); 
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_depth_arb); 
     glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
			strlen((char * ) NPRToonShaderDepthARB), NPRToonShaderDepthARB);
     success = GLExtensions::gl_arb_vertex_program_loaded("XToonTexture::init() - ", native, NPRToonShaderDepthARB);
     
     glGenProgramsARB(1, &ntt_toon_prog_focus_arb); 
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_focus_arb); 
     glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
			strlen((char * ) NPRToonShaderFocusARB), NPRToonShaderFocusARB);
     success = GLExtensions::gl_arb_vertex_program_loaded("XToonTexture::init() - ", native, NPRToonShaderFocusARB);
     
     glGenProgramsARB(1, &ntt_toon_prog_flow_arb); 
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_flow_arb); 
     glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
			strlen((char * ) NPRToonShaderFlowARB), NPRToonShaderFlowARB);
     success = GLExtensions::gl_arb_vertex_program_loaded("XToonTexture::init() - ", native, NPRToonShaderFlowARB);
     
     glGenProgramsARB(1, &ntt_toon_prog_curvature_arb); 
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_curvature_arb); 
     glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
			strlen((char * ) NPRToonShaderCurvatureARB), NPRToonShaderCurvatureARB);
     success = GLExtensions::gl_arb_vertex_program_loaded("XToonTexture::init() - ", native, NPRToonShaderCurvatureARB);
     
     glGenProgramsARB(1, &ntt_toon_prog_orientation_arb); 
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_orientation_arb); 
     glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
			strlen((char * ) NPRToonShaderOrientationARB), NPRToonShaderOrientationARB);
     success = GLExtensions::gl_arb_vertex_program_loaded("XToonTexture::init() - ", native, NPRToonShaderOrientationARB);
     
     glGenProgramsARB(1, &ntt_toon_prog_specularity_arb); 
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_specularity_arb); 
     glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
			strlen((char * ) NPRToonShaderSpecularityARB), NPRToonShaderSpecularityARB);
     success = GLExtensions::gl_arb_vertex_program_loaded("XToonTexture::init() - ", native, NPRToonShaderSpecularityARB);

     
#endif
   } else{
     return false;
   }


   if (GLExtensions::gl_arb_fragment_program_supported()) {
     bool success = false, native = false;
     err_mesg(ERR_LEV_INFO, "XToonTexture::init() - Can use ARB vertex programs!");
#ifdef GL_ARB_fragment_program     

     glGenProgramsARB(1, &ntt_toon_prog_lookup_arb); 
     glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ntt_toon_prog_lookup_arb); 
     glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
			strlen((char * ) NPRToonShaderLookupARB), NPRToonShaderLookupARB);
     success = GLExtensions::gl_arb_vertex_program_loaded("XToonTexture::init() - ", native, NPRToonShaderLookupARB);

     glGenProgramsARB(1, &ntt_toon_prog_orientation_lookup_arb); 
     glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ntt_toon_prog_orientation_lookup_arb); 
     glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
			strlen((char * ) NPRToonShaderOrientationLookupARB), NPRToonShaderOrientationLookupARB);
     success = GLExtensions::gl_arb_vertex_program_loaded("XToonTexture::init() - ", native, NPRToonShaderOrientationLookupARB);

     glGenProgramsARB(1, &ntt_toon_prog_specularity_lookup_arb); 
     glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ntt_toon_prog_specularity_lookup_arb); 
     glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
			strlen((char * ) NPRToonShaderSpecularityLookupARB), NPRToonShaderSpecularityLookupARB);
     success = GLExtensions::gl_arb_vertex_program_loaded("XToonTexture::init() - ", native, NPRToonShaderSpecularityLookupARB);

#endif
     return success;
   } else{
     return false;
   }

}

/////////////////////////////////////
// init_vertex_program()
/////////////////////////////////////

void ntt_init_vertex_program() {
   assert(!ntt_is_init_vertex_program);

   ntt_use_vertex_program_arb = ntt_init_vertex_program_arb();
   ntt_use_vertex_program_nv =  ntt_init_vertex_program_nv();

   if (ntt_use_vertex_program_nv || ntt_use_vertex_program_arb){
     err_mesg(ERR_LEV_INFO, "XToonTexture::init() - Will use vertex programs!");
     ntt_use_vertex_program = true;
   } else {
     err_mesg(ERR_LEV_INFO, "XToonTexture::init() - Won't be using vertex programs!");
     ntt_use_vertex_program = false;
   }

   ntt_is_init_vertex_program = true;
}

/////////////////////////////////////
// setup_vertex_program_nv()
/////////////////////////////////////
void ntt_setup_vertex_program_nv()
{
#if defined(GL_NV_vertex_program) && !defined(NON_NVIDIA_GFX)

   NDCpt n(XYpt(1,1));

   glBindProgramNV(GL_VERTEX_PROGRAM_NV, ntt_toon_prog_nv); 

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
			  ntt_light_pos[0], ntt_light_pos[1], ntt_light_pos[2], 1.0f);
   glProgramParameter4dNV(GL_VERTEX_PROGRAM_NV, 22,  
			  ntt_light_dir[0], ntt_light_dir[1], ntt_light_dir[2], 0.0f);
   if (ntt_positional)
      glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 23, 1, 1, 1, 1);
   else
      glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 23, 0, 0, 0, 0);

   glEnable(GL_VERTEX_PROGRAM_NV);  //GL_ENABLE_BIT

#endif
}

/////////////////////////////////////
// done_vertex_program_nv()
/////////////////////////////////////
void ntt_done_vertex_program_nv()
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
void ntt_setup_vertex_program_arb()
{
  

#ifdef GL_ARB_vertex_program
   NDCpt n(XYpt(1,1));
   
   // vertex program setup
   if (ntt_detail_map == XToonTexture::Depth){
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_depth_arb); 
   } else if (ntt_detail_map == XToonTexture::Orientation){
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_orientation_arb); 
   } else if (ntt_detail_map == XToonTexture::Focus){
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_focus_arb); 
   } else if (ntt_detail_map == XToonTexture::Specularity){
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_orientation_arb); 
   } else if (ntt_detail_map == XToonTexture::Flow){
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_flow_arb); 
   } else if (ntt_detail_map == XToonTexture::Curvature){
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_curvature_arb); 
   } else { // User or default
     glBindProgramARB(GL_VERTEX_PROGRAM_ARB, ntt_toon_prog_detail_arb); 
   }

   glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 20, n[0], n[1], 1.0, 1.0);
   glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 21, 
			      ntt_light_pos[0], ntt_light_pos[1], ntt_light_pos[2], 1.0);
   glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 22,  
			      ntt_light_dir[0], ntt_light_dir[1], ntt_light_dir[2], 0.0);

   if (ntt_positional) {
   	glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 23, 1.0, 1.0, 1.0, 1.0);
   } else {
      glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 23, 0.0, 0.0, 0.0, 0.0);
   }

   glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 24, ntt_target_length, 1.0, 1.0, 1.0);
   glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 25, ntt_max_factor, 1.0, 1.0, 1.0);
   glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 26, ntt_smooth_factor, 1.0, 1.0, 1.0);
   glEnable(GL_VERTEX_PROGRAM_ARB);  //GL_ENABLE_BIT

   // pixel program setup
   if (ntt_detail_map == XToonTexture::Orientation){
     glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ntt_toon_prog_orientation_lookup_arb); 
     glProgramEnvParameter4dARB(GL_FRAGMENT_PROGRAM_ARB, 21, ntt_target_length, 1.0, 1.0, 1.0);
   } else if (ntt_detail_map == XToonTexture::Specularity){
     glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ntt_toon_prog_specularity_lookup_arb); 
     glProgramEnvParameter4dARB(GL_FRAGMENT_PROGRAM_ARB, 21, ntt_target_length, 1.0, 1.0, 1.0);
   } else {
     glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ntt_toon_prog_lookup_arb); 
   }

   glProgramEnvParameter4dARB(GL_FRAGMENT_PROGRAM_ARB, 20, ntt_inv_detail, 1.0, 1.0, 1.0);
   glEnable(GL_FRAGMENT_PROGRAM_ARB);  //GL_ENABLE_BIT
  

#endif
}

/////////////////////////////////////
// done_vertex_program_arb()
/////////////////////////////////////
void ntt_done_vertex_program_arb()
{
#ifdef GL_ARB_vertex_program
   glDisable(GL_VERTEX_PROGRAM_ARB);  //GL_ENABLE_BIT
   glDisable(GL_FRAGMENT_PROGRAM_ARB);  //GL_ENABLE_BIT
#endif
}

/////////////////////////////////////
// setup_vertex_program()
/////////////////////////////////////
void ntt_setup_vertex_program()
{
   assert(ntt_use_vertex_program);

   if (ntt_use_vertex_program_arb)
   {
      ntt_setup_vertex_program_arb();
   }
   else if (ntt_use_vertex_program_nv)
   {
      ntt_setup_vertex_program_nv();
   }
   else
   {
      assert(0);
   }
}

/////////////////////////////////////
// done_vertex_program()
/////////////////////////////////////
void ntt_done_vertex_program()
{
   assert(ntt_use_vertex_program);

   if (ntt_use_vertex_program_arb)
   {
      ntt_done_vertex_program_arb();
   }
   else if (ntt_use_vertex_program_nv)
   {
      ntt_done_vertex_program_nv();
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
ToonTexCB::faceCB(CBvert* v, CBface*f) {
  Wvec n;
  f->vert_normal(v,n);
  
  if (!ntt_use_vertex_program){ 

    // use software computations
    double detail;
    if (ntt_detail_map == XToonTexture::User){
      detail = ntt_target_length;

    } else if (ntt_detail_map == XToonTexture::Depth) {
      Wpt eye = VIEW::peek_cam()->data()->from();
      Wvec at_v = VIEW::peek_cam()->data()->at_v();
      double d = (v->wloc()-eye)*at_v;
      detail = log(d/ntt_target_length)/log(ntt_max_factor);     

    } else if (ntt_detail_map == XToonTexture::Focus) {
      Wpt eye = VIEW::peek_cam()->data()->from();
      Wpt focus = VIEW::peek_cam()->data()->center();
      double d_v = (v->wloc()-eye).length();
      double d_c = (focus-eye).length();
      if (d_v>d_c) { // Dsup
	double d_min = d_c + ntt_target_length;
	double d_max = d_c + ntt_max_factor*ntt_target_length;
   	detail = log(d_v/d_min)/log(d_max/d_min);
      } else { // Dinf
	double d_min = d_c - ntt_target_length;
	double d_max = d_c - ntt_max_factor*ntt_target_length;
   	detail = 1.0 - log(d_v/d_max)/log(d_min/d_max);
      }

    } else if (ntt_detail_map == XToonTexture::Flow){
      Wpt prev = ntt_prev_matrix*v->wloc();
      Wpt cur = ntt_cur_matrix*v->wloc();
      double d = sqrt( (prev[0]-cur[0])*(prev[0]-cur[0]) +
		      (prev[1]-cur[1])*(prev[1]-cur[1]) );
      detail = (d-ntt_target_length)/((ntt_max_factor-1)*ntt_target_length);

    } else if (ntt_detail_map == XToonTexture::Orientation){      
      Wpt eye = VIEW::peek_cam()->data()->from();
      Wvec view_vec = eye-v->wloc();
      double n_v = n * view_vec.normalized();
      detail = 1 - n_v*n_v / ntt_target_length;

    } else if (ntt_detail_map == XToonTexture::Specularity){
      Wpt eye = VIEW::peek_cam()->data()->from();
      Wvec view_vec = (eye-v->wloc()).normalized();
      Wvec light_vec = (ntt_positional) ? (ntt_light_pos - v->wloc()).normalized() : ntt_light_dir;
      Wvec half_vec = (view_vec+light_vec)*0.5;
      double n_h = n * half_vec;
      if (n_h<0.0){
	n_h = 0.0;
      }
      detail = 1 - pow(n_h, ntt_target_length);
     

    } else if (ntt_detail_map == XToonTexture::Curvature && ntt_curv_enabled){
      double G = v->k1()*v->k2();
      detail = 1.0-abs(G)/ntt_target_length;	

    } else {
      detail = 0.0;
    }


    // clamping
    if (detail < 0.0){
      detail = 0.0;
    } else if (detail > 1.0){
      detail = 1.0;
    }
    
    // inverting
    if (ntt_inv_detail){
      detail = 1.0-detail;
    }

    // compute texture coordinates
    if (ntt_positional) {
      glTexCoord2d(n * (ntt_light_pos - v->loc()).normalized(), detail);
    } else {
      glTexCoord2d(n * ntt_light_dir, detail);
    }
      
    // apply paper effect
    if (ntt_paper_flag){
      PaperEffect::paper_coord(NDCZpt(v->wloc()).data());
    }
    
    glVertex3dv(v->loc().data());

  } else {  // use graphics hardware

     Wvec sn;
     if (ntt_smooth_enabled) {
       sn = v->get_all_faces().n_ring_faces(ntt_smooth_rings).avg_normal();
     } else if (ntt_elliptic_enabled){
       BMESH* mesh = v->mesh();
       Wvec c_to_v = v->wloc() - mesh->get_bb().center();
       Wvec dim = mesh->get_bb().dim();
       double a = dim[0]*0.5;
       double b = dim[1]*0.5;
       double c = dim[2]*0.5;
       sn = Wvec(c_to_v[0]/a, c_to_v[1]/b, c_to_v[2]/c).normalized();
     } else if (ntt_spheric_enabled){
       BMESH* mesh = v->mesh();
       Wpt c = mesh->get_bb().center();
       sn = (v->wloc()-c).normalized();
     } else if (ntt_cylindric_enabled){
       BMESH* mesh = v->mesh();
       Wpt c = mesh->get_bb().center();
       Wvec axis;
       Wvec dim = mesh->get_bb().dim();
       if (dim[0]>dim[1] && dim[0]>dim[2]){
	 axis = dim.X();
       } else if (dim[1]>dim[0] && dim[1]>dim[2]){
	 axis = dim.Y();	 
       } else {
	 axis = dim.Z();	 
       }
       Wpt v_proj = c + ((v->wloc()-c)*axis) * axis;
       sn = (v->wloc()-v_proj).normalized();
     } else {
       sn = n;
     }
     
     // get curvature info
     double k1, k2, G, M;
     if (ntt_curv_enabled){
       k1 = v->k1();
       k2 = v->k2();
       G = k1*k2;
       M = 0.5*(k1+k2);
     } else {
       G = M = k1 = k2 = DBL_MAX;
     }
     
     glNormal3dv(n.data());
     glVertexAttrib4dARB(1, sn[0], sn[1], sn[2], 1.0);
     glVertexAttrib4dARB(5, abs(k1), abs(k2), abs(G), abs(M));
     glVertex3dv(v->loc().data());
   }

}

/**********************************************************************
 * XToonTexture:
 **********************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       XToonTexture::_ntt_tags = 0;

/////////////////////////////////////
// update_lights()
/////////////////////////////////////
void
XToonTexture::update_lights(CVIEWptr& v) 
{
   if (_light_index == -1)
   {
      ntt_positional = !_light_dir;
      if (ntt_positional)
      {
         Wpt pos(_light_coords[0],_light_coords[1],_light_coords[2]);
         if (_light_cam)
            ntt_light_pos = v->cam()->xform().inverse() * pos;
         else
            ntt_light_pos = pos;
         ntt_light_pos = _patch->inv_xform() * ntt_light_pos;
      }
      else
      {
         if (_light_cam)
            ntt_light_dir = v->cam()->xform().inverse() * _light_coords;
         else
            ntt_light_dir = _light_coords;
         ntt_light_dir = (_patch->inv_xform() * ntt_light_dir).normalized();
      }
   }
   else
   {
      ntt_positional = v->light_get_positional(_light_index);
      if (ntt_positional)
      {
         if (v->light_get_in_cam_space(_light_index))
            ntt_light_pos = v->cam()->xform().inverse() * 
                                    v->light_get_coordinates_p(_light_index);
         else
            ntt_light_pos = v->light_get_coordinates_p(_light_index);
         ntt_light_pos = _patch->inv_xform() * ntt_light_pos;
      }
      else
      {
         if (v->light_get_in_cam_space(_light_index))
            ntt_light_dir = v->cam()->xform().inverse() * 
                                 v->light_get_coordinates_v(_light_index);
         else
            ntt_light_dir = v->light_get_coordinates_v(_light_index);
         ntt_light_dir = (_patch->inv_xform() * ntt_light_dir).normalized();
      }
   }
}

/////////////////////////////////////
// draw()
/////////////////////////////////////
int
XToonTexture::draw(CVIEWptr& v) 
{
  stop_watch chrono;
  // computeC

   GL_VIEW::print_gl_errors("XToonTexture::draw - Begin");

   int dl;
   
   if (_ctrl)
      return _ctrl->draw(v);

   ntt_paper_flag = (_use_paper==1)?(true):(false);

   if (!ntt_is_init_vertex_program) { 
     ntt_cur_matrix = VIEW::peek()->wpt_to_pix_proj();
     ntt_init_vertex_program();
   }

   if (ntt_paper_flag)
   {
      // The enclosing NPRTexture will have already
      // rendered the mesh in 'background' mode.
      // We should enable blending and draw with
      // z <= mode...
   }

   update_tex();
   update_lights(v);
   update_cam();

   glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | 
                GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   GL_COL(_color, _alpha*alpha());                       //GL_CURRENT_BIT
   glDisable(GL_LIGHTING);                               //GL_ENABLE_BIT

   if (_tex) {
      _tex->apply_texture();                             //GL_TEXTURE_BIT
   }

   //if ((_ntt_paper_flag) || (_alpha<1.0))
   //XXX - Just always blend, there may be alpha
   //in the texture...
   static bool OPAQUE_COMPOSITE = Config::get_var_bool("OPAQUE_COMPOSITE",false,true);

   if ((v->get_render_mode() == VIEW::TRANSPARENT_MODE) && (OPAQUE_COMPOSITE)) {
      glDisable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ZERO);
   } else {
      glEnable(GL_BLEND);                               //GL_ENABLE_BIT
//       if (PaperEffect::is_alpha_premult()) {
// 	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);   //GL_COLOR_BUFFER_BIT
//       } else {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //GL_COLOR_BUFFER_BIT
//       }
   }

   //Don't write depth if not using the background texture
   if (!_transparent)
   {
      glDepthMask(GL_FALSE);                             //GL_DEPTH_BUFFER_BIT
   }

   NDCZpt origin(0,0,0);
   if (ntt_paper_flag && _travel_paper)
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
//    PaperEffect::begin_paper_effect(ntt_paper_flag, origin[0], origin[1]);

   if (ntt_use_vertex_program) 
   {
      

      // Try it with the display list
      if (BasicTexture::dl_valid(v) && 
	  !(_update_smoothing || _update_elliptic ||
	    _update_spheric ||  _update_cylindric ||
	    _update_curvatures)) {
	ntt_setup_vertex_program();       // GL_ENABLE_BIT, ???
	BasicTexture::draw(v);
	ntt_done_vertex_program();
	
	//          PaperEffect::end_paper_effect(ntt_paper_flag);
	
	glPopAttrib();
	
	_frame_rate += chrono.elapsed_time();
	_nb_stat_frames ++;
	return _patch->num_faces();
      }
      

      // Failed. Create it.
      dl = _dl.get_dl(v, 1, _patch->stamp());
      if (dl)
         glNewList(dl, GL_COMPILE);
   }

   set_face_culling();                  // GL_ENABLE_BIT
   _patch->draw_tri_strips(_cb);

   if (ntt_use_vertex_program) 
   {
      // End the display list here
      if (_dl.dl(v)) {
         _dl.close_dl(v);

	 ntt_smooth_enabled = false;
	 ntt_elliptic_enabled = false;
	 ntt_spheric_enabled = false;
	 ntt_cylindric_enabled = false;
 	 _update_smoothing = false;
  	 _update_elliptic = false;
	 _update_spheric = false;
 	 _update_cylindric = false;
 	 _update_curvatures = false;

         // Built it, now execute it
         ntt_setup_vertex_program();       // GL_ENABLE_BIT, ???
         BasicTexture::draw(v);
         ntt_done_vertex_program();
      }
   }

//    PaperEffect::end_paper_effect(ntt_paper_flag);

   glPopAttrib();

   GL_VIEW::print_gl_errors("XToonTexture::draw - End");
   return _patch->num_faces();
}

/////////////////////////////////////
// update()
/////////////////////////////////////


void
XToonTexture::update_tex(void)
{

   int ind;

   if (!_toon_texture_names)
   {
      _toon_texture_names = new LIST<str_ptr>; assert(_toon_texture_names);
      _toon_texture_ptrs = new LIST<TEXTUREptr>; assert(_toon_texture_ptrs);

      _toon_texture_remap_orig_names = new LIST<str_ptr>; assert(_toon_texture_remap_orig_names);
      _toon_texture_remap_new_names = new LIST<str_ptr>; assert(_toon_texture_remap_new_names);

      int i = 0;
      while (toon_remap_fnames[i][0] != NULL)
      {
         _toon_texture_remap_orig_names->add(str_ptr(toon_remap_base) + toon_remap_fnames[i][0]);
         _toon_texture_remap_new_names->add(str_ptr(toon_remap_base) + toon_remap_fnames[i][1]);
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
                  "XToonTexture::set_texture() - Previously remapped --===<<[[{{ (%s) ---> (%s) }}]]>>===--", 
                     **(Config::JOT_ROOT()+old_tf), **(Config::JOT_ROOT()+tf) );
            }
         }

         //Now see if the final name yields a good texture...
         if ((*_toon_texture_ptrs)[ind] != NULL)
         {
            _tex = (*_toon_texture_ptrs)[ind];
            _tex_name = tf;
            err_mesg(ERR_LEV_SPAM, "XToonTexture::set_texture() - Using cached copy of texture.");
         }
         else
         {
            err_mesg(ERR_LEV_INFO, "XToonTexture::set_texture() - **ERROR** Previous caching failure: '%s'...", **tf);
            _tex = NULL;
            _tex_name = NULL_STR;
         }
      }
      //Haven't seen this name before...
      else
      {
         err_mesg(ERR_LEV_SPAM, "XToonTexture::set_texture() - Not in cache...");
      
         Image i(**(Config::JOT_ROOT()+tf));

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
                  "XToonTexture::set_texture() - Remapping --===<<[[{{ (%s) ---> (%s) }}]]>>===--", 
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

            err_mesg(ERR_LEV_INFO, "XToonTexture::set_texture() - Cached: (w=%d h=%d bpp=%u) %s",
               i.width(), i.height(), i.bpp(), **(Config::JOT_ROOT()+tf));;

            _tex = t;
            _tex_name = tf;
	      }
         //Otherwise insert a failed NULL
	      else
	      {
            err_mesg(ERR_LEV_ERROR, "XToonTexture::set_texture() - *****ERROR***** Failed loading to cache: '%s'...", **(Config::JOT_ROOT()+tf));
         
            _toon_texture_names->add(tf);
            _toon_texture_ptrs->add(NULL);

            _tex = NULL;
            _tex_name = NULL_STR;
	      }
      }   
   }



}


/*
void
XToonTexture::update_tex(void)
{
   int ind;

   if (_tex_name == NULL_STR)
   {
      assert(_tex == NULL);
   }
   else if ((_tex_name != NULL_STR) && (_tex == NULL))
   {
      if (!_toon_texture_names)
      {
         _toon_texture_names = new LIST<str_ptr>;     assert(_toon_texture_names);
         _toon_texture_ptrs = new LIST<TEXTUREptr>;   assert(_toon_texture_ptrs);
      }

      if ((ind = _toon_texture_names->get_index(_tex_name)) != BAD_IND)
      {
         if ((*_toon_texture_ptrs)[ind] != NULL)
         {
            _tex = (*_toon_texture_ptrs)[ind];
            //_tex_name = _tex_name;
         }
         else
         {
            err_mesg(ERR_LEV_INFO, "XToonTexture::update_tex() - *****ERROR***** Previous caching failure: '%s'",
                          **(Config::JOT_ROOT() + _tex_name));
            _tex = NULL;
            _tex_name = NULL_STR;
         }
      }
      else
      {
         err_mesg(ERR_LEV_SPAM, "XToonTexture::update_tex() - Not in cache...");
      
         Image i(Config::JOT_ROOT() + _tex_name);
         if (!i.empty())
	      {
		      TEXTUREglptr t = new TEXTUREgl("");

            t->set_save_img(true);
            t->set_wrap_s(GL_CLAMP_TO_EDGE);
            t->set_wrap_t(GL_CLAMP_TO_EDGE);
            t->set_image(i.copy(),i.width(),i.height(),i.bpp());

            _toon_texture_names->add(_tex_name);
            _toon_texture_ptrs->add(t);

            err_mesg(ERR_LEV_INFO, "XToonTexture::update_tex() - Cached: (WIDTH=%d HEIGHT=%d BPP=%u) %s", 
                           i.width(), i.height(), i.bpp(), **(Config::JOT_ROOT() + _tex_name));
            _tex = t;
            //_tex_name = _tex_name;
	      }
	      else
	      {
            err_mesg(ERR_LEV_ERROR, "XToonTexture::update_tex() - *****ERROR***** Failed loading to cache: '%s'",
                          **(Config::JOT_ROOT() + _tex_name));

            _toon_texture_names->add(_tex_name);
            _toon_texture_ptrs->add(NULL);

            _tex = NULL;
            _tex_name = NULL_STR;
	      }
      }   
   }

}
*/



void XToonTexture::update_cam(){
   if (_detail_map == Depth) {
     double min_depth = 1.0;
     double max_depth = 1000.0;
     ntt_target_length = max_depth*_target_length + min_depth*(1.0f-_target_length);

   } else if (_detail_map == Focus) {
     if (ntt_use_vertex_program_arb){
       Wpt eye = VIEW::peek_cam()->data()->from();
       Wpt focus = VIEW::peek_cam()->data()->center();
       double focus_dist = (focus-eye).length(); 
       Wvec focus_vec (focus_dist, focus_dist, focus_dist);
       Wtransf focus_matrix (focus_vec, focus_vec, focus_vec, focus_vec);
       glMatrixMode(GL_MATRIX0_ARB);
       glLoadMatrixd(focus_matrix.transpose().matrix());
       glMatrixMode(GL_MODELVIEW);           
     }     

     double min_depth = 1.0;
     double max_depth = 100.0;
     ntt_target_length = max_depth*_target_length + min_depth*(1.0f-_target_length);

   } else if (_detail_map == Flow){

     ntt_prev_matrix = ntt_cur_matrix;
     if (ntt_use_vertex_program_arb) {
       glMatrixMode(GL_MATRIX0_ARB);
       glLoadMatrixd(ntt_prev_matrix.transpose().matrix());
       glMatrixMode(GL_MODELVIEW);    
     }
     
     ntt_cur_matrix = VIEW::peek()->wpt_to_pix_proj();
     if (ntt_use_vertex_program_arb) {
       glMatrixMode(GL_MATRIX1_ARB);
       glLoadMatrixd(ntt_cur_matrix.transpose().matrix());       
       glMatrixMode(GL_MODELVIEW);    
     }

     double max_screen_dist = 2.0;
     ntt_target_length = _target_length*max_screen_dist;     
   } else if (_detail_map == Curvature){
     double max_curvature = 1/(100.0f*_patch->mesh()->curvature()->feature_size());
     ntt_target_length = _target_length * max_curvature;
   } else if (_detail_map == Orientation){
//      float max_orientation = 2.0;
//      ntt_target_length = _target_length * max_orientation;
     ntt_target_length = 1/_target_length - 1.0f;
   } else if (_detail_map == Specularity){
     ntt_target_length = 1.0/(1.0-_target_length);
   } else {
     ntt_target_length = _target_length;
    }

   ntt_max_factor = _max_factor;
   ntt_smooth_factor = _smooth_factor;
   ntt_detail_map = _detail_map;
}





/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
XToonTexture::tags() const
{
   if (!_ntt_tags) {
      _ntt_tags = new TAGlist;
      *_ntt_tags += OGLTexture::tags();

      *_ntt_tags += new TAG_val<XToonTexture,int>(
         "use_paper",
         &XToonTexture::use_paper_);
      *_ntt_tags += new TAG_val<XToonTexture,int>(
         "travel_paper",
         &XToonTexture::travel_paper_);
      *_ntt_tags += new TAG_val<XToonTexture,COLOR>(
         "COLOR",
         &XToonTexture::color_);
      *_ntt_tags += new TAG_val<XToonTexture,double>(
         "alpha",
         &XToonTexture::alpha_);

      *_ntt_tags += new TAG_val<XToonTexture,int>(
         "light_index",
         &XToonTexture::light_index_);
      *_ntt_tags += new TAG_val<XToonTexture,int>(
         "light_dir",
         &XToonTexture::light_dir_);
      *_ntt_tags += new TAG_val<XToonTexture,int>(
         "light_cam",
         &XToonTexture::light_cam_);
      *_ntt_tags += new TAG_val<XToonTexture,Wvec>(
         "light_coords",
         &XToonTexture::light_coords_);


      *_ntt_tags += new TAG_meth<XToonTexture>(
         "texture",
         &XToonTexture::put_tex_name,
         &XToonTexture::get_tex_name,
         1);
      *_ntt_tags += new TAG_meth<XToonTexture>(
         "layer_name",
         &XToonTexture::put_layer_name,
         &XToonTexture::get_layer_name,
         1);


      *_ntt_tags += new TAG_meth<XToonTexture>(
         "transparent",
         &XToonTexture::put_transparent,
         &XToonTexture::get_transparent,
         0);
      *_ntt_tags += new TAG_meth<XToonTexture>(
         "annotate",
         &XToonTexture::put_annotate,
         &XToonTexture::get_annotate,
         0);

   }
   return *_ntt_tags;
}
////////////////////////////////////
// put_layer_name()
/////////////////////////////////////
void
XToonTexture::put_layer_name(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "XToonTexture::put_layer_name()");
        
   d.id();
   if (get_layer_name() == NULL_STR)
   {
      err_mesg(ERR_LEV_SPAM, "XToonTexture::put_layer_name() - Wrote NULL string.");
      *d << "NULL_STR";
      *d << " ";
   }
   else
   {
      *d << **(get_layer_name());
      *d << " ";
      err_mesg(ERR_LEV_SPAM, "XToonTexture::put_layer_name() - Wrote string: '%s'", **get_tex_name());
   }
   d.end_id();
}

/////////////////////////////////////
// get_layer_name()
/////////////////////////////////////

void
XToonTexture::get_layer_name(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "XToonTexture::get_layer_name()");

   //XXX - May need something to handle filenames with spaces

   str_ptr str, lay, space;
   *d >> str;      
   if (!(*d).ascii()) *d >> space; 

   if (str == "NULL_STR") 
   {
      lay = NULL_STR;
      err_mesg(ERR_LEV_SPAM, "XToonTexture::get_layer_name() - Loaded NULL string.");
   }
   else
   {
      lay = str;
      err_mesg(ERR_LEV_SPAM, "XToonTexture::get_layer_name() - Loaded string: '%s'", **lay);
   }
   set_layer_name(lay);

}

////////////////////////////////////
// put_texname()
/////////////////////////////////////
void
XToonTexture::put_tex_name(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "XToonTexture::put_tex_name()");

   //XXX - May need something to handle filenames with spaces

   d.id();
   if (_tex_name == NULL_STR)
   {
      err_mesg(ERR_LEV_SPAM, "XToonTexture::put_tex_name() - Wrote NULL string.");
      *d << "NULL_STR";
      *d << " ";
   }
   else
   {
      *d << **(get_tex_name());
      *d << " ";
      err_mesg(ERR_LEV_SPAM, "XToonTexture::put_tex_name() - Wrote string: '%s'", **get_tex_name());
   }
   d.end_id();
}

/////////////////////////////////////
// get_tex_name()
/////////////////////////////////////

void
XToonTexture::get_tex_name(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "XToonTexture::get_tex_name()");

   //XXX - May need something to handle filenames with spaces

   str_ptr str, space;
   *d >> str;      
   if (!(*d).ascii()) *d >> space; 

   if (str == "NULL_STR") 
   {
      str = NULL_STR;
      err_mesg(ERR_LEV_SPAM, "XToonTexture::get_tex_name() - Loaded NULL string.");
   }
   else
   {
      err_mesg(ERR_LEV_SPAM, "XToonTexture::get_tex_name() - Loaded string: '%s'", **str);
   }
   set_tex_name(str);

}


////////////////////////////////////
// put_transparent()
/////////////////////////////////////
void
XToonTexture::put_transparent(TAGformat &d) const
{
   // XXX - Deprecated
}

/////////////////////////////////////
// get_transparent()
/////////////////////////////////////
void
XToonTexture::get_transparent(TAGformat &d)
{
   err_mesg(ERR_LEV_WARN, "XToonTexture::get_transparent() - ***NOTE: Loading OLD format file!***");

   *d >> _transparent;
}

////////////////////////////////////
// put_annotate()
/////////////////////////////////////
void
XToonTexture::put_annotate(TAGformat &d) const
{
   // XXX - Deprecated
}

/////////////////////////////////////
// get_annotate()
/////////////////////////////////////
void
XToonTexture::get_annotate(TAGformat &d){
   err_mesg(ERR_LEV_WARN, "XToonTexture::get_annotate() - ***NOTE: Loading OLD format file!***");
   
   *d >> _annotate;
}



void
XToonTexture::update_smoothing(bool b){ 
  _update_smoothing = true; 
  _normals_smoothed = b;
  ntt_smooth_enabled = b;
  cerr << "update_smoothing" << endl;
}

void
XToonTexture::update_elliptic (bool b){ 
  _update_elliptic = true; 
  _normals_elliptic = b;
  ntt_elliptic_enabled = b;
  cerr << "update_elliptic" << endl;
}

void
XToonTexture::update_spheric(bool b){ 
  _update_spheric = true; 
  _normals_spheric = b;
  ntt_spheric_enabled = b;
  cerr << "update_spheric" << endl;
}

void
XToonTexture::update_cylindric(bool b){ 
  _update_cylindric = true; 
  _normals_cylindric = b;
  ntt_cylindric_enabled = b;
  cerr << "update_cylindric" << endl;
}


void
XToonTexture::update_curvatures(bool b){ 
  _update_curvatures = true; 
  ntt_curv_enabled = b;
}


void 
XToonTexture::set_inv_detail(bool b){
  ntt_inv_detail = b;
}


void                 
XToonTexture::print_frame_rate(){
  if (_nb_stat_frames==0) return;

  _frame_rate = _nb_stat_frames/_frame_rate;
  cerr << "Detail map = "  << _detail_map << endl;
  cerr << "_frame_rate = "  << _frame_rate << endl;
  cerr << "resolution = "  << VIEW::peek()->width() << " ; " << VIEW::peek()->height() << endl;

  _frame_rate = 0.0;
  _nb_stat_frames = 0;
}

// end of file xtoon_texture.C
