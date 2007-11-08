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
#include "mesh/uv_data.H"
#include "npr_solid_texture.H"
#include "std/config.H"

using namespace mlib;

/**********************************************************************
 * Globals
 **********************************************************************/

bool     nst_paper_flag;       //If paper's being used
bool     nst_tex_flag;         //If texture's being used
bool     nst_is_init_vertex_program = false;
bool     nst_use_vertex_program;
bool     nst_use_vertex_program_arb;
bool     nst_use_vertex_program_nv;

GLuint   nst_lit_solid_prog_arb;
GLuint   nst_solid_prog_arb;
GLuint   nst_lit_solid_prog_nv;
GLuint   nst_solid_prog_nv;

LIST<str_ptr>*    NPRSolidTexture::_solid_texture_names = 0;
LIST<TEXTUREptr>* NPRSolidTexture::_solid_texture_ptrs = 0;
LIST<str_ptr>*    NPRSolidTexture::_solid_texture_remap_orig_names = 0;
LIST<str_ptr>*    NPRSolidTexture::_solid_texture_remap_new_names = 0;

/*****************************************************************
 * Texture Remapping
 *****************************************************************/

char *solid_remap_base = "nprdata/other_textures/";
char *solid_remap_fnames[][2] = 
{
   {"apple4.png",       "o-still-apple.png"},
   {"ban2.png",         "o-still-banana.png"},
   {"bluepaint3.png",   "o-still-pitcher.png"},
   {"lemon2.png",       "o-still-lemon.png"},
   {"pear5.png",        "o-still-pear.png"},
   {"plate.png",        "o-still-bowl.png"},
   {"table4.png",       "o-still-table.png"},
   {NULL,            NULL}
};

/////////////////////////////////////
// The Lighting Shader
/////////////////////////////////////

const unsigned char NPRLitSolidShaderNV[]=
{

// Material Support:
//    -Emis, Amb, Diff, Spec colors
// Lighting Support:
//    -3 Lights (GL_LIGHT0-2)
//    -Global ambient color
//    -Per light amb, diff, spec color
//    -Position or Directional (no spots)
//    -Attenuation (1,d,d^2)
//
// NOTE: Right now jot forces:
//    -amb, diff material track active color
//    -emis is black
//    -spec is white when engaged 
//

//c[0]-c[3]    modelview
//c[4]-c[7]    projection
//c[8]-c[11]   inv_trans_modelview
//c[12]-c[15]  texture0
//c[16]-c[19]  texture1
//c[20]        ndc window size [xy]

//c[21]        eye position [xyzw]
//c[22]        global illum (mat.emiss + mat.amb * global amb) [rgba]
//c[23]        light mask - three lights - (1=on 0=off) [l0,l1,l2]

//c[30]        light0 - position (w=1) or direction (w=0) [xyzw]
//c[31]        light0 - ambient (light * mat.amb) [rgba]   
//c[32]        light0 - diffuse (light * mat.diff) [rgba]
//c[33]        light0 - specular (light * mat.spec) [rgba] (a=shininess)
//c[34]        light0 - attenuation [k0,k1,k2]

//c[40]        light1 - position (w=1) or direction (w=0) [xyzw]
//c[41]        light1 - ambient (light * mat.amb) [rgba]
//c[42]        light1 - diffuse (light * mat.diff) [rgba]
//c[43]        light1 - specular (light * mat.spec) [rgba] (a=shininess)
//c[44]        light1 - attenuation [k0,k1,k2]

//c[50]        light2 - position (w=1) or direction (w=0) [xyzw]
//c[51]        light2 - ambient (light * mat.amb) [rgba]
//c[52]        light2 - diffuse (light * mat.diff) [rgba]
//c[53]        light2 - specular (light * mat.spec) [rgba] (a=shininess)
//c[54]        light2 - attenuation [k0,k1,k2]

//R0           scratch
//R1           scratch
//R2(RQ)       lit results
//R3 (Rd)      light dist
//R4 (Rl)      light vec
//R5 (Rv)      eye vector
//R6 (Rx)      specular color
//R7 (Rc)      diffuse color
//R8 (RH)      half angle vector
//R9 (Ro)      (1,1,1,1)
//R10(Re)      eye position non-homogeneous
//R11(Rn)      eye normal


"!!VP1.0\
SGE R9,R9,R9;\
DP4 R10.x,v[OPOS],c[0];\
DP4 R10.y,v[OPOS],c[1];\
DP4 R10.z,v[OPOS],c[2];\
DP4 R10.w,v[OPOS],c[3];\
DP4 R1.x,R10,c[4];\
DP4 R1.y,R10,c[5];\
DP4 R1.z,R10,c[6];\
DP4 R1.w,R10,c[7];\
MOV o[HPOS],R1;\
MUL R1,R1,c[20];\
RCP R0.w,R10.w;\
MUL R10,R10,R0.w;\
DP4 o[TEX0].x,v[TEX0],c[12];\
DP4 o[TEX0].y,v[TEX0],c[13];\
DP4 o[TEX0].z,v[TEX0],c[14];\
DP4 o[TEX0].w,v[TEX0],c[15];\
DP4 o[TEX1].x,R1,c[16];\
DP4 o[TEX1].y,R1,c[17];\
DP4 o[TEX1].z,R1,c[18];\
DP4 o[TEX1].w,R1,c[19];\
DP3 R11.x,v[NRML],c[8];\
DP3 R11.y,v[NRML],c[9];\
DP3 R11.z,v[NRML],c[10];\
DP3 R11.w,R11,R11;\
RSQ R11.w,R11.w;\
MUL R11,R11,R11.w;\
ADD R0,-R10,c[21];\
DP3 R0.w,R0,R0;\
RSQ R1.w,R0.w;\
MUL R5,R0,R1.w;\
MOV R7,c[22];\
SLT R6,R6,R6;"

"ADD R8,R5,c[30];\
DP3 R8.w,R8,R8;\
RSQ R8.w,R8.w;\
MUL R8,R8,R8.w;\
DP3 R0.x,R11,c[30];\
DP3 R0.y,R11,R8;\
MOV R0.w,c[33].w;\
LIT R2,R0;\
ADD R0,-R10,c[30];\
DP3 R0.w,R0,R0;\
RSQ R1.w,R0.w;\
MUL R4,R0,R1.w;"
//"DST R3,R0.w,R1.w;\"
//"DP3 R3.w,R3,c[34];\"
//"RCP R3,R3.w;\"
"ADD R8,R5,R4;\
DP3 R8.w,R8,R8;\
RSQ R8.w,R8.w;\
MUL R8,R8,R8.w;\
DP3 R0.x,R11,R4;\
DP3 R0.y,R11,R8;\
MOV R0.w,c[33].w;\
LIT R0,R0;"
//"MUL R0,R0,R3.w;\"
"SLT R1,c[30].wwww,R9;\
MUL R2,R1,R2;\
MAD R2,c[30].w,R0,R2;\
MUL R2,c[23].x,R2;\
MAD R7.xyz,R2.x,c[31],R7;\
MAD R7.xyz,R2.y,c[32],R7;\
MAD R6.xyz,R2.z,c[33],R6;"

"ADD R8,R5,c[40];\
DP3 R8.w,R8,R8;\
RSQ R8.w,R8.w;\
MUL R8,R8,R8.w;\
DP3 R0.x,R11,c[40];\
DP3 R0.y,R11,R8;\
MOV R0.w,c[43].w;\
LIT R2,R0;\
ADD R0,-R10,c[40];\
DP3 R0.w,R0,R0;\
RSQ R1.w,R0.w;\
MUL R4,R0,R1.w;\
DST R3,R0.w,R1.w;\
DP3 R3.w,R3,c[44];\
RCP R3,R3.w;\
ADD R8,R5,R4;\
DP3 R8.w,R8,R8;\
RSQ R8.w,R8.w;\
MUL R8,R8,R8.w;\
DP3 R0.x,R11,R4;\
DP3 R0.y,R11,R8;\
MOV R0.w,c[43].w;\
LIT R0,R0;\
MUL R0,R0,R3.w;\
SLT R1,c[40].wwww,R9;\
MUL R2,R1,R2;\
MAD R2,c[40].w,R0,R2;\
MUL R2,c[23].y,R2;\
MAD R7.xyz,R2.x,c[41],R7;\
MAD R7.xyz,R2.y,c[42],R7;\
MAD R6.xyz,R2.z,c[43],R6;"

"ADD R8,R5,c[50];\
DP3 R8.w,R8,R8;\
RSQ R8.w,R8.w;\
MUL R8,R8,R8.w;\
DP3 R0.x,R11,c[50];\
DP3 R0.y,R11,R8;\
MOV R0.w,c[53].w;\
LIT R2,R0;\
ADD R0,-R10,c[50];\
DP3 R0.w,R0,R0;\
RSQ R1.w,R0.w;\
MUL R4,R0,R1.w;\
DST R3,R0.w,R1.w;\
DP3 R3.w,R3,c[54];\
RCP R3,R3.w;\
ADD R8,R5,R4;\
DP3 R8.w,R8,R8;\
RSQ R8.w,R8.w;\
MUL R8,R8,R8.w;\
DP3 R0.x,R11,R4;\
DP3 R0.y,R11,R8;\
MOV R0.w,c[53].w;\
LIT R0,R0;\
MUL R0,R0,R3.w;\
SLT R1,c[50].wwww,R9;\
MUL R2,R1,R2;\
MAD R2,c[50].w,R0,R2;\
MUL R2,c[23].z,R2;\
MAD R7.xyz,R2.x,c[51],R7;\
MAD R7.xyz,R2.y,c[52],R7;\
MAD R6.xyz,R2.z,c[53],R6;"

"ADD o[COL0].xyz,R6,R7;\
MOV o[COL0].w,v[COL0].w;\
END"
};

const unsigned char NPRLitSolidShaderARB[]=
//XXX - ATI wants newlines, otherwise the
//error string thinks every error is
//at the start of the program, as
//that's the start of the only line...!
//Sad, huh?
{
"!!ARBvp1.0\n\
ATTRIB iPos  = vertex.position;\n\
ATTRIB iNorm = vertex.normal;\n\
ATTRIB iCol  = vertex.color;\n\
ATTRIB iTex0 = vertex.texcoord[0];\n\
\
OUTPUT oPos  = result.position;\n\
OUTPUT oCol  = result.color;\n\
OUTPUT oTex0 = result.texcoord[0];\n\
OUTPUT oTex1 = result.texcoord[1];\n\
\
PARAM  cOne  = {1.0,1.0,1.0,1.0};\n\
PARAM  cZero = {0.0,0.0,0.0,0.0};\n\
\
PARAM  cNDC       = program.env[20];\n\
PARAM  cLightMask = program.env[23];\n\
\
"
"\
\
PARAM  cMatShin       = program.env[21];\n\
PARAM  cGlobCol       = program.env[22];\n\
\
PARAM  cLightPos0     = program.env[30];\n\
PARAM  cLightColAmb0  = program.env[31];\n\
PARAM  cLightColDiff0 = program.env[32];\n\
PARAM  cLightColSpec0 = program.env[33];\n\
PARAM  cLightAtten0   = program.env[34];\n\
\
PARAM  cLightPos1     = program.env[40];\n\
PARAM  cLightColAmb1  = program.env[41];\n\
PARAM  cLightColDiff1 = program.env[42];\n\
PARAM  cLightColSpec1 = program.env[43];\n\
PARAM  cLightAtten1   = program.env[44];\n\
\
PARAM  cLightPos2     = program.env[50];\n\
PARAM  cLightColAmb2  = program.env[51];\n\
PARAM  cLightColDiff2 = program.env[52];\n\
PARAM  cLightColSpec2 = program.env[53];\n\
PARAM  cLightAtten2   = program.env[54];\n\
\
"
/*
"\
\
PARAM  cGlobCol   = state.lightmodel.scenecolor;\
PARAM  cMatShin   = state.material.shininess;\
\
PARAM  cLightPos0     = state.light[0].position;\
PARAM  cLightAtten0   = state.light[0].attenuation;\
PARAM  cLightColAmb0  = state.lightprod[0].ambient;\
PARAM  cLightColDiff0 = state.lightprod[0].diffuse;\
PARAM  cLightColSpec0 = state.lightprod[0].specular;\
\
PARAM  cLightPos1     = state.light[1].position;\
PARAM  cLightAtten1   = state.light[1].attenuation;\
PARAM  cLightColAmb1  = state.lightprod[1].ambient;\
PARAM  cLightColDiff1 = state.lightprod[1].diffuse;\
PARAM  cLightColSpec1 = state.lightprod[1].specular;\
\
PARAM  cLightPos2     = state.light[2].position;\
PARAM  cLightAtten2   = state.light[2].attenuation;\
PARAM  cLightColAmb2  = state.lightprod[2].ambient;\
PARAM  cLightColDiff2 = state.lightprod[2].diffuse;\
PARAM  cLightColSpec2 = state.lightprod[2].specular;\
\
"
*/
"\
\
PARAM  mMVP[4]    =  { state.matrix.mvp };\n\
PARAM  mMV0[4]    =  { state.matrix.modelview[0] };\n\
PARAM  mMV0_it[4] =  { state.matrix.modelview[0].invtrans };\n\
PARAM  mTex0[4]   =  { state.matrix.texture[0] };\n\
PARAM  mTex1[4]   =  { state.matrix.texture[1] };\n\
\
TEMP   tScratch1;\n\
TEMP   tScratch2;\n\
TEMP   tEyePos;\n\
TEMP   tEyeNorm;\n\
TEMP   tEyeVec;\n\
TEMP   tHalfVec;\n\
TEMP   tLightVec;\n\
TEMP   tDiffCol;\n\
TEMP   tSpecCol;\n\
TEMP   tAtten;\n\
TEMP   tLit;\n\
\
ALIAS  aXY     = tScratch1;\n\
ALIAS  aNDC    = tScratch1;\n\
ALIAS  aLitDir = tScratch1;\n\
ALIAS  aLitPos = tScratch1;\n\
ALIAS  aFoo    = tScratch1;\n\
\
ALIAS  aDir    = tScratch2;\n\
ALIAS  aBar    = tScratch2;\n\
"
"\
DP4 aXY.x, mMVP[0], iPos;\n\
DP4 aXY.y, mMVP[1], iPos;\n\
DP4 aXY.z, mMVP[2], iPos;\n\
DP4 aXY.w, mMVP[3], iPos;\n\
\
MOV oPos, aXY;\n\
\
MUL aNDC, aXY,cNDC;\n\
\
DP4 oTex0.x, mTex0[0], iTex0;\n\
DP4 oTex0.y, mTex0[1], iTex0;\n\
DP4 oTex0.z, mTex0[2], iTex0;\n\
DP4 oTex0.w, mTex0[3], iTex0;\n\
\
DP4 oTex1.x, mTex1[0], aNDC;\n\
DP4 oTex1.y, mTex1[1], aNDC;\n\
DP4 oTex1.z, mTex1[2], aNDC;\n\
DP4 oTex1.w, mTex1[3], aNDC;\n\
\
DP4 tEyePos.x, mMV0[0], iPos;\n\
DP4 tEyePos.y, mMV0[1], iPos;\n\
DP4 tEyePos.z, mMV0[2], iPos;\n\
DP4 tEyePos.w, mMV0[3], iPos;\n\
\
RCP aFoo.w, tEyePos.w;\n\
MUL tEyePos,tEyePos, aFoo.w;\n\
\
DP3 tEyeNorm.x, mMV0_it[0], iNorm;\n\
DP3 tEyeNorm.y, mMV0_it[1], iNorm;\n\
DP3 tEyeNorm.z, mMV0_it[2], iNorm;\n\
DP3 tEyeNorm.w, tEyeNorm,  tEyeNorm;\n\
RSQ tEyeNorm.w, tEyeNorm.w;\n\
MUL tEyeNorm,   tEyeNorm,  tEyeNorm.w;\n\
\
DP3 aFoo.w,   tEyePos,  tEyePos;\n\
RSQ aFoo.w,   aFoo.w;\n\
MUL tEyeVec, -tEyePos,  aFoo.w;\n\
\
MOV tDiffCol, cGlobCol;\n\
\
"
"\
\
ADD tHalfVec,   tEyeVec,      cLightPos0;\
DP3 tHalfVec.w, tHalfVec,     tHalfVec;\
RSQ tHalfVec.w, tHalfVec.w;\
MUL tHalfVec,   tHalfVec,     tHalfVec.w;\
DP3 aLitDir.x,  tEyeNorm,     cLightPos0;\
DP3 aLitDir.y,  tEyeNorm,     tHalfVec;\
MOV aLitDir.w,  cMatShin.x;\
LIT aLitDir,    aLitDir;\
ADD aDir,       cOne,        -cLightPos0.wwww;\
MUL tLit,       aLitDir,      aDir;\
\
\
ADD aFoo,       cLightPos0,  -tEyePos;\n\
DP3 aFoo.w,     aFoo,         aFoo;\n\
RSQ aBar.w,     aFoo.w;\n\
MUL tLightVec,  aFoo,         aBar.w;\n\
DP3 aLitPos.x,  tEyeNorm,     tLightVec;\n\
\
DST tAtten,     aFoo.w,       aBar.w;\n\
DP3 tAtten.w,   tAtten,       cLightAtten0;\n\
RCP tAtten,     tAtten.w;\n\
\
ADD tHalfVec,   tEyeVec,      tLightVec;\n\
DP3 tHalfVec.w, tHalfVec,     tHalfVec;\n\
RSQ tHalfVec.w, tHalfVec.w;\n\
MUL tHalfVec,   tHalfVec,     tHalfVec.w;\n\
DP3 aLitPos.y,  tEyeNorm,     tHalfVec;\n\
MOV aLitPos.w,  cMatShin.x;\n\
\
LIT aLitPos,    aLitPos;\n\
MUL aLitPos,    aLitPos,      tAtten.w;\n\
\
MAD tLit,       aLitPos,      cLightPos0.w,     tLit;\n\
\
\
MUL tLit,       cLightMask.x, tLit;\n\
\
MAD tDiffCol,   tLit.x,       cLightColAmb0,    tDiffCol;\n\
MAD tDiffCol,   tLit.y,       cLightColDiff0,   tDiffCol;\n\
MUL tSpecCol,   tLit.z,       cLightColSpec0;\n\
\
"
"\
\
ADD tHalfVec,   tEyeVec,      cLightPos1;\n\
DP3 tHalfVec.w, tHalfVec,     tHalfVec;\n\
RSQ tHalfVec.w, tHalfVec.w;\n\
MUL tHalfVec,   tHalfVec,     tHalfVec.w;\n\
DP3 aLitDir.x,  tEyeNorm,     cLightPos1;\n\
DP3 aLitDir.y,  tEyeNorm,     tHalfVec;\n\
MOV aLitDir.w,  cMatShin.x;\n\
LIT aLitDir,    aLitDir;\n\
ADD aDir,       cOne,        -cLightPos1.wwww;\n\
MUL tLit,       aLitDir,      aDir;\n\
\
\
ADD aFoo,       cLightPos1,  -tEyePos;\n\
DP3 aFoo.w,     aFoo,         aFoo;\n\
RSQ aBar.w,     aFoo.w;\n\
MUL tLightVec,  aFoo,         aBar.w;\n\
DP3 aLitPos.x,  tEyeNorm,     tLightVec;\n\
\
DST tAtten,     aFoo.w,       aBar.w;\n\
DP3 tAtten.w,   tAtten,       cLightAtten1;\n\
RCP tAtten,     tAtten.w;\n\
\
ADD tHalfVec,   tEyeVec,      tLightVec;\n\
DP3 tHalfVec.w, tHalfVec,     tHalfVec;\n\
RSQ tHalfVec.w, tHalfVec.w;\n\
MUL tHalfVec,   tHalfVec,     tHalfVec.w;\n\
DP3 aLitPos.y,  tEyeNorm,     tHalfVec;\n\
MOV aLitPos.w,  cMatShin.x;\n\
\
LIT aLitPos,    aLitPos;\n\
MUL aLitPos,    aLitPos,      tAtten.w;\n\
\
MAD tLit,       aLitPos,      cLightPos1.w,     tLit;\n\
MUL tLit,       cLightMask.y, tLit;\n\
\
MAD tDiffCol,   tLit.x,       cLightColAmb1,    tDiffCol;\n\
MAD tDiffCol,   tLit.y,       cLightColDiff1,   tDiffCol;\n\
MAD tSpecCol,   tLit.z,       cLightColSpec1,   tSpecCol;\n\
\
"
"\
\
ADD tHalfVec,   tEyeVec,      cLightPos2;\n\
DP3 tHalfVec.w, tHalfVec,     tHalfVec;\n\
RSQ tHalfVec.w, tHalfVec.w;\n\
MUL tHalfVec,   tHalfVec,     tHalfVec.w;\n\
DP3 aLitDir.x,  tEyeNorm,     cLightPos2;\n\
DP3 aLitDir.y,  tEyeNorm,     tHalfVec;\n\
MOV aLitDir.w,  cMatShin.x;\n\
LIT aLitDir,    aLitDir;\n\
ADD aDir,       cOne,        -cLightPos2.wwww;\n\
MUL tLit,       aLitDir,      aDir;\n\
\
ADD aFoo,       cLightPos2,  -tEyePos;\n\
DP3 aFoo.w,     aFoo,         aFoo;\n\
RSQ aBar.w,     aFoo.w;\n\
MUL tLightVec,  aFoo,         aBar.w;\n\
DP3 aLitPos.x,  tEyeNorm,     tLightVec;\n\
\
DST tAtten,     aFoo.w,       aBar.w;\n\
DP3 tAtten.w,   tAtten,       cLightAtten2;\n\
RCP tAtten,     tAtten.w;\n\
\
ADD tHalfVec,   tEyeVec,      tLightVec;\n\
DP3 tHalfVec.w, tHalfVec,     tHalfVec;\n\
RSQ tHalfVec.w, tHalfVec.w;\n\
MUL tHalfVec,   tHalfVec,     tHalfVec.w;\n\
\
DP3 aLitPos.y,  tEyeNorm,     tHalfVec;\n\
MOV aLitPos.w,  cMatShin.x;\n\
\
LIT aLitPos,    aLitPos;\n\
MUL aLitPos,    aLitPos,      tAtten.w;\n\
\
MAD tLit,       aLitPos,      cLightPos2.w,     tLit;\n\
MUL tLit,       cLightMask.z, tLit;\n\
\
MAD tDiffCol,   tLit.x,       cLightColAmb2,    tDiffCol;\n\
MAD tDiffCol,   tLit.y,       cLightColDiff2,   tDiffCol;\n\
MAD tSpecCol,   tLit.z,       cLightColSpec2,   tSpecCol;\n\
\
ADD oCol.xyz,   tDiffCol,     tSpecCol;\n\
MOV oCol.w,     iCol.w;\n\
\
END\n"
};

/////////////////////////////////////
// The Non-Lighting Shader
/////////////////////////////////////

const unsigned char NPRSolidShaderNV[]=
{

//c[0]-c[3]    modelview
//c[4]-c[7]    projection
//c[12]-c[15]  texture0
//c[16]-c[19]  texture1
//c[20]        ndc window size

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
DP4 o[TEX0].x,v[TEX0],c[12];\
DP4 o[TEX0].y,v[TEX0],c[13];\
DP4 o[TEX0].z,v[TEX0],c[14];\
DP4 o[TEX0].w,v[TEX0],c[15];\
MOV o[COL0],v[COL0];\
END"
};


const unsigned char NPRSolidShaderARB[]=
{
"!!ARBvp1.0\n\
\
ATTRIB iPos  = vertex.position;\
ATTRIB iCol  = vertex.color;\
ATTRIB iTex0 = vertex.texcoord[0];\
\
OUTPUT oPos  = result.position;\
OUTPUT oCol  = result.color;\
OUTPUT oTex0 = result.texcoord[0];\
OUTPUT oTex1 = result.texcoord[1];\
\
PARAM  cNDC       = program.env[20];\
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
DP4 oTex0.x, mTex0[0], iTex0;\
DP4 oTex0.y, mTex0[1], iTex0;\
DP4 oTex0.z, mTex0[2], iTex0;\
DP4 oTex0.w, mTex0[3], iTex0;\
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

bool nst_init_vertex_program_nv()
{
   if (GLExtensions::gl_nv_vertex_program_supported())
   {
      err_mesg(ERR_LEV_INFO, "NPRSolidTexture::init() - Can use NV vertex programs");
#if defined(GL_NV_vertex_program) && !defined(NON_NVIDIA_GFX)
      glGenProgramsNV(1, &nst_solid_prog_nv); 
      glBindProgramNV(GL_VERTEX_PROGRAM_NV, nst_solid_prog_nv); 
      glLoadProgramNV(GL_VERTEX_PROGRAM_NV, nst_solid_prog_nv, strlen((char * ) NPRSolidShaderNV), NPRSolidShaderNV);
      glGenProgramsNV(1, &nst_lit_solid_prog_nv); 
      glBindProgramNV(GL_VERTEX_PROGRAM_NV, nst_lit_solid_prog_nv); 
      glLoadProgramNV(GL_VERTEX_PROGRAM_NV, nst_lit_solid_prog_nv, strlen((char * ) NPRLitSolidShaderNV), NPRLitSolidShaderNV);
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

bool nst_init_vertex_program_arb()
{
   assert(!nst_is_init_vertex_program);

   if (GLExtensions::gl_arb_vertex_program_supported())
   {
      bool success = false, native = false;
      err_mesg(ERR_LEV_INFO, "NPRSolidTexture::init() - Can use ARB vertex programs!");
#ifdef GL_ARB_vertex_program
      
      GL_VIEW::print_gl_errors("NPRSolidTexture::init [Start] - ");
      
      glGenProgramsARB(1, &nst_solid_prog_arb); 
      glBindProgramARB(GL_VERTEX_PROGRAM_ARB, nst_solid_prog_arb); 
	   glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                           strlen((char * ) NPRSolidShaderARB), NPRSolidShaderARB);
      success = GLExtensions::gl_arb_vertex_program_loaded("NPRSolidTexture::init (unlit) - ", native, NPRSolidShaderARB);

      GL_VIEW::print_gl_errors("NPRSolidTexture::init [Middle] - ");
      
      glGenProgramsARB(1, &nst_lit_solid_prog_arb); 
      glBindProgramARB(GL_VERTEX_PROGRAM_ARB, nst_lit_solid_prog_arb); 
	   glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                           strlen((char * ) NPRLitSolidShaderARB), NPRLitSolidShaderARB);

      success = success && GLExtensions::gl_arb_vertex_program_loaded("NPRSolidTexture::init (lit) - ", native, NPRLitSolidShaderARB);

      GL_VIEW::print_gl_errors("NPRSolidTexture::init [End] - ");
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

void nst_init_vertex_program()
{
   assert(!nst_is_init_vertex_program);

   nst_use_vertex_program_arb = nst_init_vertex_program_arb();
   nst_use_vertex_program_nv =  nst_init_vertex_program_nv();

   if (nst_use_vertex_program_nv || nst_use_vertex_program_arb)
   {
      err_mesg(ERR_LEV_INFO, "NPRSolidTexture::init() - Will use vertex programs!");
      nst_use_vertex_program = true;
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "NPRSolidTexture::init() - Won't be using vertex programs!");
      nst_use_vertex_program = false;
   }

   nst_is_init_vertex_program = true;
}



/////////////////////////////////////
// setup_vertex_program_nv()
/////////////////////////////////////
void nst_setup_vertex_program_nv(bool lit)
{
#if defined(GL_NV_vertex_program) && !defined(NON_NVIDIA_GFX)
   //Bind the *long* lit version only when needed
   if (!lit)
      glBindProgramNV(GL_VERTEX_PROGRAM_NV, nst_solid_prog_nv); 
   else
      glBindProgramNV(GL_VERTEX_PROGRAM_NV, nst_lit_solid_prog_nv); 

   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 0, GL_MODELVIEW, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 4, GL_PROJECTION, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 8, GL_MODELVIEW, GL_INVERSE_TRANSPOSE_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 12, GL_TEXTURE, GL_IDENTITY_NV);

   if (GLExtensions::gl_arb_multitexture_supported())
   {
#ifdef GL_ARB_multitexture
      glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 16, GL_TEXTURE1_ARB, GL_IDENTITY_NV);
#endif
   }
   NDCpt n(XYpt(1,1));
   glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 20, (float)n[0], (float)n[1], 1, 1);
   
   //Load the lighting state if needed
   if (lit)
   {
      GLfloat me[4],ma[4],md[4],ms[4],msh;
      GLfloat ga[4],la[4],ld[4],ls[4],lp[4],lx[4];

      glGetMaterialfv(GL_FRONT,GL_EMISSION,me);
      glGetMaterialfv(GL_FRONT,GL_AMBIENT,ma);
      glGetMaterialfv(GL_FRONT,GL_DIFFUSE,md);
      glGetMaterialfv(GL_FRONT,GL_SPECULAR,ms);
      glGetMaterialfv(GL_FRONT,GL_SHININESS,&msh);

      //c[21]        eye position [xyzw]
      glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 21, 0.0f, 0.0f, 0.0f, 1.0f);

      //c[22]        global illum (mat.emiss + mat.amb * global amb) [rgba]
      glGetFloatv(GL_LIGHT_MODEL_AMBIENT,ga);
      glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 22, 
            me[0]+ma[0]*ga[0], me[1]+ma[1]*ga[1], me[2]+ma[2]*ga[2], me[3]+ma[3]*ga[3]);

      //c[23]        light mask - three lights - (1=on 0=off) [l0,l1,l2]
      glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 23, (glIsEnabled(GL_LIGHT0))?1.0f:0.0f, 
            (glIsEnabled(GL_LIGHT1))?1.0f:0.0f, (glIsEnabled(GL_LIGHT2))?1.0f:0.0f,0.0f);

      for (int i=0; i<3; i++)
      {
         //c[30+i10]    lighti - position (w=1) or direction (w=0) [xyzw]
         //c[31+i10]    lighti - ambient (light * mat.amb) [rgba]   
         //c[32+i10]    lighti - diffuse (light * mat.diff) [rgba]
         //c[33+i10]    lighti - specular (light * mat.spec) [rgba] (a=shininess)
         //c[34+i10]    lighti - attenuation [k0,k1,k2]

         glGetLightfv(GL_LIGHT0+i, GL_POSITION,lp);
         glGetLightfv(GL_LIGHT0+i, GL_AMBIENT,la);
         glGetLightfv(GL_LIGHT0+i, GL_DIFFUSE,ld);
         glGetLightfv(GL_LIGHT0+i, GL_SPECULAR,ls);
         glGetLightfv(GL_LIGHT0+i, GL_CONSTANT_ATTENUATION,&lx[0]);
         glGetLightfv(GL_LIGHT0+i, GL_LINEAR_ATTENUATION,&lx[1]);
         glGetLightfv(GL_LIGHT0+i, GL_QUADRATIC_ATTENUATION,&lx[2]);      

         glProgramParameter4fvNV(GL_VERTEX_PROGRAM_NV, 30 + i*10, lp);
         glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV,  31 + i*10, 
                     ma[0]*la[0], ma[1]*la[1], ma[2]*la[2], ma[3]*la[3]);
         glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV,  32 + i*10, 
                     md[0]*ld[0], md[1]*ld[1], md[2]*ld[2], md[3]*ld[3]);
         glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV,  33 + i*10, 
                     ms[0]*ls[0], ms[1]*ls[1], ms[2]*ls[2], msh);
         glProgramParameter4fvNV(GL_VERTEX_PROGRAM_NV, 34 + i*10, lx);
      }

   }

   glEnable(GL_VERTEX_PROGRAM_NV);  //GL_ENABLE_BIT

#endif
}

/////////////////////////////////////
// done_vertex_program()
/////////////////////////////////////
void nst_done_vertex_program_nv()
{
#if defined(GL_NV_vertex_program) && !defined(NON_NVIDIA_GFX)
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 0,  GL_NONE, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 4,  GL_NONE, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 8,  GL_NONE, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 12, GL_NONE, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 16, GL_NONE, GL_IDENTITY_NV);
   glDisable(GL_VERTEX_PROGRAM_NV);  //GL_ENABLE_BIT
#endif
}


/////////////////////////////////////
// setup_vertex_program()
/////////////////////////////////////
void nst_setup_vertex_program_arb(bool lit)
{
#ifdef GL_ARB_vertex_program
   NDCpt n(XYpt(1,1));

   if (!lit)
      glBindProgramARB(GL_VERTEX_PROGRAM_ARB, nst_solid_prog_arb); 
   else
      glBindProgramARB(GL_VERTEX_PROGRAM_ARB, nst_lit_solid_prog_arb); 
  
   glProgramEnvParameter4dARB(GL_VERTEX_PROGRAM_ARB, 20, n[0], n[1], 1.0, 1.0);

   //Load the lighting state if needed
   if (lit)
   {
      GLboolean   glbool;
      GLint       glint;

      GLfloat me[4],ma[4],md[4],ms[4],msh;
      GLfloat ga[4],la[4],ld[4],ls[4],lp[4],lx[4];


      // XXX - ATI barfs on this.  It will light things 
      // correctly using the fixed pipeline, but manual
      // queries of the material state (here, or in
      // vertex programs) fail to return the right 
      // values when GL_COLOR_MATERIAL is enabled.
      // So, we'll just manually figure things out...

      // Used to be AMBIENT_AND_DIFFUSE tracking... right?
      glGetBooleanv(GL_COLOR_MATERIAL,&glbool);
      assert(glbool == GL_TRUE);

      glGetIntegerv(GL_COLOR_MATERIAL_FACE,&glint);
      assert(glint == GL_FRONT_AND_BACK);        

      glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER,&glint);
      assert(glint == GL_AMBIENT_AND_DIFFUSE);

      glGetMaterialfv(GL_FRONT,GL_EMISSION,  me);
//    glGetMaterialfv(GL_FRONT,GL_AMBIENT,   ma);
      glGetFloatv(    GL_CURRENT_COLOR,      ma);
//    glGetMaterialfv(GL_FRONT,GL_DIFFUSE,   md);
      glGetFloatv(    GL_CURRENT_COLOR,      md);
      glGetMaterialfv(GL_FRONT,GL_SPECULAR,  ms);
      glGetMaterialfv(GL_FRONT,GL_SHININESS,&msh);

      //c[21]        material shininess [s,s,s,s]
      glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 21, msh, msh, msh, msh);

      //c[22]        global illum (mat.emiss + mat.amb * global amb) [rgba]
      glGetFloatv(GL_LIGHT_MODEL_AMBIENT,ga);
      glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 22, 
            me[0]+ma[0]*ga[0], me[1]+ma[1]*ga[1], me[2]+ma[2]*ga[2], me[3]+ma[3]*ga[3]);

      //c[23]        light mask - three lights - (1=on 0=off) [l0,l1,l2]
      glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 23, (glIsEnabled(GL_LIGHT0))?1.0f:0.0f, 
            (glIsEnabled(GL_LIGHT1))?1.0f:0.0f, (glIsEnabled(GL_LIGHT2))?1.0f:0.0f,0.0f);

      for (int i=0; i<3; i++)
      {
         //c[30+i10]    lighti - position (w=1) or direction (w=0) [xyzw]
         //c[31+i10]    lighti - ambient (light * mat.amb) [rgba]   
         //c[32+i10]    lighti - diffuse (light * mat.diff) [rgba]
         //c[33+i10]    lighti - specular (light * mat.spec) [rgba] (a=shininess)
         //c[34+i10]    lighti - attenuation [k0,k1,k2]

         glGetLightfv(GL_LIGHT0+i, GL_POSITION,lp);
         glGetLightfv(GL_LIGHT0+i, GL_AMBIENT,la);
         glGetLightfv(GL_LIGHT0+i, GL_DIFFUSE,ld);
         glGetLightfv(GL_LIGHT0+i, GL_SPECULAR,ls);
         glGetLightfv(GL_LIGHT0+i, GL_CONSTANT_ATTENUATION,&lx[0]);
         glGetLightfv(GL_LIGHT0+i, GL_LINEAR_ATTENUATION,&lx[1]);
         glGetLightfv(GL_LIGHT0+i, GL_QUADRATIC_ATTENUATION,&lx[2]);      

         glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 30 + i*10, lp);
         glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 31 + i*10, 
                     ma[0]*la[0], ma[1]*la[1], ma[2]*la[2], ma[3]*la[3]);
         glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 32 + i*10, 
                     md[0]*ld[0], md[1]*ld[1], md[2]*ld[2], md[3]*ld[3]);
         glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 33 + i*10, 
                     ms[0]*ls[0], ms[1]*ls[1], ms[2]*ls[2], ms[3]*ls[3]);
         glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 34 + i*10, lx);
      }

   }

   glEnable(GL_VERTEX_PROGRAM_ARB);  //GL_ENABLE_BIT
#endif
}

/////////////////////////////////////
// done_vertex_program()
/////////////////////////////////////
void nst_done_vertex_program_arb()
{
#ifdef GL_ARB_vertex_program
   glDisable(GL_VERTEX_PROGRAM_ARB);  //GL_ENABLE_BIT
#endif
}

/////////////////////////////////////
// setup_vertex_program()
/////////////////////////////////////
void nst_setup_vertex_program(bool lit)
{
   assert(nst_use_vertex_program);

   if (nst_use_vertex_program_arb)
   {
      nst_setup_vertex_program_arb(lit);
   }
   else if (nst_use_vertex_program_nv)
   {
      nst_setup_vertex_program_nv(lit);
   }
   else
   {
      assert(0);
   }
}

/////////////////////////////////////
// done_vertex_program()
/////////////////////////////////////
void nst_done_vertex_program()
{
   assert(nst_use_vertex_program);

   if (nst_use_vertex_program_arb)
   {
      nst_done_vertex_program_arb();
   }
   else if (nst_use_vertex_program_nv)
   {
      nst_done_vertex_program_nv();
   }
   else
   {
      assert(0);
   }

}

/**********************************************************************
 * NPRSolidTexCB:
 **********************************************************************/
void 
NPRSolidTexCB::faceCB(CBvert* v, CBface*f) 
{
   Wvec n;
   f->vert_normal(v,n);

   if (!nst_use_vertex_program)
   {
      if (nst_tex_flag) {
         TexCoordGen* tg = f->patch()->tex_coord_gen();
         if (tg) 
            glTexCoord2dv(tg->uv_from_vert(v,f).data());
         else if (UVdata::lookup(f))
            glTexCoord2dv(UVdata::get_uv(v,f).data());
      }
   
      if (nst_paper_flag)
         PaperEffect::paper_coord(NDCZpt(v->wloc()).data());
      
      glNormal3dv(n.data());
      glVertex3dv(v->loc().data());
   }
   else
   {
      if (nst_tex_flag)
      {
         TexCoordGen* tg = f->patch()->tex_coord_gen();
         if (tg) 
            glTexCoord2dv(tg->uv_from_vert(v,f).data());
         else if (UVdata::lookup(f))
            glTexCoord2dv(UVdata::get_uv(v,f).data());
      }


      glNormal3dv(n.data());
      glVertex3dv(v->loc().data());
   }

}

/**********************************************************************
 * NPRSolidTexture:
 **********************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       NPRSolidTexture::_nst_tags = 0;


/////////////////////////////////////
// draw()
/////////////////////////////////////
int
NPRSolidTexture::draw(CVIEWptr& v) 
{
   GL_VIEW::print_gl_errors("NPRSolidTexture::draw - Begin");
   
   int dl;

   if (_ctrl)
      return _ctrl->draw(v);

   nst_paper_flag = (_use_paper==1)?(true):(false);

   if (!nst_is_init_vertex_program) 
      nst_init_vertex_program();

   if (nst_paper_flag)
   {
      // The enclosing NPRTexture will have already
      // rendered the mesh in 'background' mode.
      // We should enable blending and draw with
      // z <= mode...
   }

   update_tex();
   

   glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT |
                     GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   GL_COL(_color, _alpha*alpha());                       //GL_CURRENT_BIT

/*
   if (_use_lighting)
   {
      GLboolean   glbool;
      GLint       glint;

      glEnable(GL_LIGHTING);                             //GL_ENABLE_BIT
      glShadeModel(GL_SMOOTH);                           //GL_LIGHTING_BIT

      GLfloat me[4], mad[4], ms[4], msh;

      // XXX - ATI barfs on this.  It will light things 
      // correctly using the fixed pipeline, but manual
      // queries of the material state (here, or in
      // vertex programs) fail to return the right 
      // values when GL_COLOR_MATERIAL is enabled.
      // So, we'll just manually kil that mode, and
      // set the materials ourselves...

      // Used to be AMBIENT_AND_DIFFUSE tracking... right?
      glGetBooleanv(GL_COLOR_MATERIAL,&glbool);
      assert(glbool == GL_TRUE);

      glGetIntegerv(GL_COLOR_MATERIAL_FACE,&glint);
      assert(glint == GL_FRONT_AND_BACK);        

      glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER,&glint);
      assert(glint == GL_AMBIENT_AND_DIFFUSE);

      glDisable(GL_COLOR_MATERIAL);                         //GL_ENABLE_BIT

      glGetMaterialfv(GL_FRONT,  GL_EMISSION,        me);
      glGetFloatv(               GL_CURRENT_COLOR,  mad);
      glGetMaterialfv(GL_FRONT,  GL_SHININESS,     &msh);

      if (_light_specular) { ms[0] = ms[1] = ms[2] = ms[3] = 1.0f; }
      else                 { ms[0] = ms[1] = ms[2] = ms[3] = 0.0f; }   

      glMaterialfv(GL_FRONT_AND_BACK,  GL_EMISSION,   me);  //GL_LIGHTING_BIT
      glMaterialfv(GL_FRONT_AND_BACK,  GL_AMBIENT,    mad); //GL_LIGHTING_BIT
      glMaterialfv(GL_FRONT_AND_BACK,  GL_DIFFUSE,    mad); //GL_LIGHTING_BIT
      glMaterialfv(GL_FRONT_AND_BACK,  GL_SPECULAR,   ms);  //GL_LIGHTING_BIT
      glMaterialf(GL_FRONT_AND_BACK,   GL_SHININESS,  msh); //GL_LIGHTING_BIT
   }
*/

   if (_use_lighting)
   {
      glEnable(GL_LIGHTING);                             //GL_ENABLE_BIT
      glShadeModel(GL_SMOOTH);                           //GL_LIGHTING_BIT

      GLfloat ms[4];

      if (_light_specular) { ms[0] = ms[1] = ms[2] = ms[3] = 1.0f; }
      else                 { ms[0] = ms[1] = ms[2] = ms[3] = 0.0f; }   

      glMaterialfv(GL_FRONT_AND_BACK,  GL_SPECULAR,   ms);  //GL_LIGHTING_BIT
   }
   else
   {
      glDisable(GL_LIGHTING);                            //GL_ENABLE_BIT
   }
   
   if (_tex) 
   {
      _tex->apply_texture();                             //GL_TEXTURE_BIT
   }

   //XXX - Just always blend, there may be alpha
   //in the texture...
   static bool OPAQUE_COMPOSITE = Config::get_var_bool("OPAQUE_COMPOSITE",false,true);

   if ((v->get_render_mode() == VIEW::TRANSPARENT_MODE) && (OPAQUE_COMPOSITE))
   {
      glDisable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ZERO);
   }
   else
   {
      glEnable(GL_BLEND);                                   //GL_ENABLE_BIT
      if (PaperEffect::is_alpha_premult())
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);			//GL_COLOR_BUFFER_BIT
		else
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //GL_COLOR_BUFFER_BIT
   }

   //Don't write depth if not using the background texture
   if (!_transparent)
   {
      glDepthMask(GL_FALSE);                             //GL_DEPTH_BUFFER_BIT
   }

   NDCZpt origin(0,0,0);

   if (nst_paper_flag && _travel_paper)
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

   PaperEffect::begin_paper_effect(nst_paper_flag, origin[0], origin[1]);

   if (nst_use_vertex_program) 
   {
      

      // Try it with the display list
      if ((_uv_in_dl == nst_tex_flag) && BasicTexture::dl_valid(v))
      {
         nst_setup_vertex_program(_use_lighting==1);    // GL_ENABLE_BIT, ???
         BasicTexture::draw(v);
         nst_done_vertex_program();

         PaperEffect::end_paper_effect(nst_paper_flag);

         glPopAttrib();

         return _patch->num_faces();
      }

      // Failed. Create it.
      dl = _dl.get_dl(v, 1, _patch->stamp());
      if (dl)
      {
         glNewList(dl, GL_COMPILE);
         _uv_in_dl = nst_tex_flag;
      }
   }

   set_face_culling();                                   // GL_ENABLE_BIT
   _patch->draw_tri_strips(_cb);

   if (nst_use_vertex_program) 
   {
      // End the display list here
      if (_dl.dl(v)) 
      {
         _dl.close_dl(v);

         // Built it, now execute it
         nst_setup_vertex_program(_use_lighting==1);        // GL_ENABLE_BIT, ???
         BasicTexture::draw(v);
         nst_done_vertex_program();
      }
   }

   PaperEffect::end_paper_effect(nst_paper_flag);

   glPopAttrib();

   GL_VIEW::print_gl_errors("NPRSolidTexture::draw - End");

   return _patch->num_faces();

}

/////////////////////////////////////
// update_tex()
/////////////////////////////////////

void
NPRSolidTexture::update_tex(void)
{

   int ind;

   if (!_solid_texture_names)
   {
      _solid_texture_names = new LIST<str_ptr>; assert(_solid_texture_names);
      _solid_texture_ptrs = new LIST<TEXTUREptr>; assert(_solid_texture_ptrs);

      _solid_texture_remap_orig_names = new LIST<str_ptr>; assert(_solid_texture_remap_orig_names);
      _solid_texture_remap_new_names = new LIST<str_ptr>; assert(_solid_texture_remap_new_names);

      int i = 0;
      while (solid_remap_fnames[i][0] != NULL)
      {
         _solid_texture_remap_orig_names->add(str_ptr(solid_remap_base) + solid_remap_fnames[i][0]);
         _solid_texture_remap_new_names->add(str_ptr(solid_remap_base) + solid_remap_fnames[i][1]);
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
      if ((ind = _solid_texture_names->get_index(tf)) != BAD_IND)
      {
         //Finding original name in cache...

         //If its a failed texture...
         if ((*_solid_texture_ptrs)[ind] == NULL)
         {
            //...see if it was remapped...
            int ii = _solid_texture_remap_orig_names->get_index(tf);
            //...and change to looking up the remapped name            
            if (ii != BAD_IND)
            {
               str_ptr old_tf = tf;
               tf = (*_solid_texture_remap_new_names)[ii];

               ind = _solid_texture_names->get_index(tf);

               err_mesg(ERR_LEV_SPAM, 
                  "NPRSolidTexture::set_texture() - Previously remapped --===<<[[{{ (%s) ---> (%s) }}]]>>===--", 
                     **(Config::JOT_ROOT()+old_tf), **(Config::JOT_ROOT()+tf) );
            }
         }

         //Now see if the final name yields a good texture...
         if ((*_solid_texture_ptrs)[ind] != NULL)
         {
            _tex = (*_solid_texture_ptrs)[ind];
            _tex_name = tf;
            err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::set_texture() - Using cached copy of texture.");
         }
         else
         {
            err_mesg(ERR_LEV_INFO, "NPRSolidTexture::set_texture() - **ERROR** Previous caching failure: '%s'...", **tf);
            _tex = NULL;
            _tex_name = NULL_STR;
         }
      }
      //Haven't seen this name before...
      else
      {
         err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::set_texture() - Not in cache...");
      
         Image i(**(Config::JOT_ROOT()+tf));

         //Can't load the texture?
         if (i.empty())
         {
            //...check for a remapped file...
            int ii = _solid_texture_remap_orig_names->get_index(tf);

            //...and use that name instead....
            if (ii != BAD_IND)
            {
               //...but also indicate that the original name is bad...

               _solid_texture_names->add(tf);
               _solid_texture_ptrs->add(NULL);

               str_ptr old_tf = tf;
               tf = (*_solid_texture_remap_new_names)[ii];

               err_mesg(ERR_LEV_ERROR, 
                  "NPRSolidTexture::set_texture() - Remapping --===<<[[{{ (%s) ---> (%s) }}]]>>===--", 
                     **(Config::JOT_ROOT()+old_tf), **(Config::JOT_ROOT()+tf) );

               i.load_file(**(Config::JOT_ROOT()+tf));
            }
         }

         //If the final name loads, store the cached texture...
         if (!i.empty())
	      {
            TEXTUREglptr t = new TEXTUREgl();

            t->set_save_img(true);
            t->set_image(i.copy(),i.width(),i.height(),i.bpp());
         

            _solid_texture_names->add(tf);
            _solid_texture_ptrs->add(t);

            err_mesg(ERR_LEV_INFO, "NPRSolidTexture::set_texture() - Cached: (w=%d h=%d bpp=%u) %s",
               i.width(), i.height(), i.bpp(), **(Config::JOT_ROOT()+tf));;

            _tex = t;
            _tex_name = tf;
	      }
         //Otherwise insert a failed NULL
	      else
	      {
            err_mesg(ERR_LEV_ERROR, "NPRSolidTexture::set_texture() - *****ERROR***** Failed loading to cache: '%s'...", **(Config::JOT_ROOT()+tf));
         
            _solid_texture_names->add(tf);
            _solid_texture_ptrs->add(NULL);

            _tex = NULL;
            _tex_name = NULL_STR;
	      }
      }   
   }


   if (_tex)
      nst_tex_flag = true;
   else
      nst_tex_flag = false;

}

/*
void
NPRSolidTexture::update_tex(void)
{

   int ind;

   if (_tex_name == NULL_STR)
   {
      assert(_tex == NULL);
   }
   else if ((_tex_name != NULL_STR) && (_tex == NULL))
   {
      if (!_solid_texture_names)
      {
         _solid_texture_names = new LIST<str_ptr>;     assert(_solid_texture_names);
         _solid_texture_ptrs = new LIST<TEXTUREptr>;   assert(_solid_texture_ptrs);
      }

      if ((ind = _solid_texture_names->get_index(_tex_name)) != BAD_IND)
      {
         if ((*_solid_texture_ptrs)[ind] != NULL)
         {
            _tex = (*_solid_texture_ptrs)[ind];
            //_tex_name = _tex_name;
         }
         else
         {
            err_mesg(ERR_LEV_INFO, "NPRSolidTexture::update_tex() - *****ERROR***** Previous caching failure: '%s'",
                          **(Config::JOT_ROOT() + _tex_name));


            _tex = NULL;
            _tex_name = NULL_STR;
         }
      }
      else
      {
         err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::update_tex() - Not in cache...");
      
         Image i(Config::JOT_ROOT() + _tex_name);
         if (!i.empty())
	      {
		      TEXTUREglptr t = new TEXTUREgl("");

            t->set_save_img(true);
            t->set_image(i.copy(),i.width(),i.height(),i.bpp());

            _solid_texture_names->add(_tex_name);
            _solid_texture_ptrs->add(t);

            err_mesg(ERR_LEV_INFO, "NPRSolidTexture::update_tex() - Cached: (WIDTH=%d HEIGHT=%d BPP=%u) %s", 
                           i.width(), i.height(), i.bpp(), **(Config::JOT_ROOT() + _tex_name));

            _tex = t;
            //_tex_name = _tex_name;
	      }
	      else
	      {
            err_mesg(ERR_LEV_ERROR, "NPRSolidTexture::update_tex() - *****ERROR***** Failed loading to cache: '%s'",
                          **(Config::JOT_ROOT() + _tex_name));

            _solid_texture_names->add(_tex_name);
            _solid_texture_ptrs->add(NULL);

            _tex = NULL;
            _tex_name = NULL_STR;
	      }
      }   
   }

   if (_tex)
      nst_tex_flag = true;
   else
      nst_tex_flag = false;

}
*/






/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
NPRSolidTexture::tags() const
{
   if (!_nst_tags) {
      _nst_tags = new TAGlist;
      *_nst_tags += OGLTexture::tags();

      *_nst_tags += new TAG_val<NPRSolidTexture,int>(
         "use_paper",
         &NPRSolidTexture::use_paper_);
      *_nst_tags += new TAG_val<NPRSolidTexture,int>(
         "travel_paper",
         &NPRSolidTexture::travel_paper_);
      *_nst_tags += new TAG_val<NPRSolidTexture,int>(
         "use_lighting",
         &NPRSolidTexture::use_lighting_);
      *_nst_tags += new TAG_val<NPRSolidTexture,int>(
         "light_specular",
         &NPRSolidTexture::light_specular_);
      *_nst_tags += new TAG_val<NPRSolidTexture,COLOR>(
         "COLOR",
         &NPRSolidTexture::color_);
      *_nst_tags += new TAG_val<NPRSolidTexture,double>(
         "alpha",
         &NPRSolidTexture::alpha_);

      *_nst_tags += new TAG_meth<NPRSolidTexture>(
         "texture",
         &NPRSolidTexture::put_tex_name,
         &NPRSolidTexture::get_tex_name,
         1);
      *_nst_tags += new TAG_meth<NPRSolidTexture>(
         "layer_name",
         &NPRSolidTexture::put_layer_name,
         &NPRSolidTexture::get_layer_name,
         1);


      *_nst_tags += new TAG_meth<NPRSolidTexture>(
         "transparent",
         &NPRSolidTexture::put_transparent,
         &NPRSolidTexture::get_transparent,
         0);
      *_nst_tags += new TAG_meth<NPRSolidTexture>(
         "annotate",
         &NPRSolidTexture::put_annotate,
         &NPRSolidTexture::get_annotate,
         0);
   }
   return *_nst_tags;
}

////////////////////////////////////
// put_layer_name()
/////////////////////////////////////
void
NPRSolidTexture::put_layer_name(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::put_layer_name()");
      
   d.id();
   if (get_layer_name() == NULL_STR)
   {
      err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::put_layer_name() - Wrote NULL string.");
      *d << "NULL_STR";
      *d << " ";
   }
   else
   {
      *d << **(get_layer_name());
      *d << " ";
      err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::put_layer_name() - Wrote string: '%s'", **get_tex_name());
   }
   d.end_id();
}

/////////////////////////////////////
// get_layer_name()
/////////////////////////////////////

void
NPRSolidTexture::get_layer_name(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::get_layer_name()");

   //XXX - May need something to handle filenames with spaces

   str_ptr str, lay, space;
   *d >> str;     
   if (!(*d).ascii()) *d >> space; 

   if (str == "NULL_STR") 
   {
      lay = NULL_STR;
      err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::get_layer_name() - Loaded NULL string.");
   }
   else
   {
      lay = str;
      err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::get_layer_name() - Loaded string: '%s'", **lay);
   }
   set_layer_name(lay);

}

////////////////////////////////////
// put_texname()
/////////////////////////////////////
void
NPRSolidTexture::put_tex_name(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::put_tex_name()");
        
   //XXX - May need something to handle filenames with spaces

   d.id();
   if (get_tex_name() == NULL_STR)
   {
      err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::put_tex_name() - Wrote NULL string.");
      *d << "NULL_STR";
      *d << " ";
   }
   else
   {
      *d << **(get_tex_name());
      *d << " ";
      err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::put_tex_name() - Wrote string: '%s'", **get_tex_name());
   }
   d.end_id();
}

/////////////////////////////////////
// get_tex_name()
/////////////////////////////////////

void
NPRSolidTexture::get_tex_name(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::get_tex_name()");

   //XXX - May need something to handle filenames with spaces

   str_ptr str, tex, space;
   *d >> str;     
   if (!(*d).ascii()) *d >> space; 

   if (str == "NULL_STR") 
   {
      tex = NULL_STR;
      err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::get_tex_name() - Loaded NULL string.");
   }
   else
   {
      tex = str;
      err_mesg(ERR_LEV_SPAM, "NPRSolidTexture::get_tex_name() - Loaded string: '%s'", **tex);
   }
   set_tex_name(tex);

}

////////////////////////////////////
// put_transparent()
/////////////////////////////////////
void
NPRSolidTexture::put_transparent(TAGformat &d) const
{
   // XXX - Deprecated
}

/////////////////////////////////////
// get_transparent()
/////////////////////////////////////
void
NPRSolidTexture::get_transparent(TAGformat &d)
{
   err_mesg(ERR_LEV_WARN, "NPRSolidTexture::get_transparent() - ***NOTE: Loading OLD format file!***");

   *d >> _transparent;
}

////////////////////////////////////
// put_annotate()
/////////////////////////////////////
void
NPRSolidTexture::put_annotate(TAGformat &d) const
{
   // XXX - Deprecated
}

/////////////////////////////////////
// get_annotate()
/////////////////////////////////////
void
NPRSolidTexture::get_annotate(TAGformat &d)
{
   err_mesg(ERR_LEV_WARN, "NPRSolidTexture::get_annotate() - ***NOTE: Loading OLD format file!***");

   *d >> _annotate;
}


/* end of file npr_solid_texture.C */
