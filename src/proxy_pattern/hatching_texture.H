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
#ifndef HATCHING_TEXTURE_H
#define HATCHING_TEXTURE_H

#include "std/config.H"
#include "mesh/patch.H"
#include "proxy_surface.H"
#include "gtex/basic_texture.H"

#include <vector>

class ProxyUVStroke;
class ProxyUVStrokeInf;
class ProxyTexture;

#define CHatchingTexture HatchingTexture const
class HatchingTexture : public BasicTexture  {
 public: 
   //******** MANAGERS ********
   HatchingTexture(Patch* patch = 0);      
   virtual ~HatchingTexture();

   //******** RUN-TIME TYPE ID ********   
   DEFINE_RTTI_METHODS3("HatchingTexture", HatchingTexture*,
                        HatchingTexture, CDATA_ITEM*);

   //******** DATA_ITEM METHODS ********
   virtual DATA_ITEM  *dup() const { return new HatchingTexture;}
    
   //******** GTexture VIRTUAL METHODS ********
   virtual void set_patch(Patch* p); 
   int  draw(CVIEWptr& v);
   virtual int  draw_final(CVIEWptr& v);   
   void         set_draw_proxy_mesh(bool b) {_draw_proxy_mesh = b; }
   bool         get_draw_proxy_mesh() const { return _draw_proxy_mesh; }

   //***** Helper funcions *****
   void draw_start();
   void draw_end();

   //********Interface to proxy ********
   void             set_proxy_texture(ProxyTexture* pt);  
   ProxyTexture*    get_proxy_texture() const { return _proxy_texture;}   

   bool add_stroke(CNDCpt_list& pl, const ARRAY<double>& prl, BaseStroke * proto);
   
  
 private:    
   ProxyTexture*                 _proxy_texture;
   vector<ProxyUVStroke*>        _strokes;        //Global strokes live in UV space infinity to infinity
   bool                          _draw_proxy_mesh;
   // These strokes live on the faces..
   //void create_strokes();
   //void draw_starter_strokes(CBface_list& faces);
   //ProxyUVStroke* create_one_stroke(CUVpt& start, CUVpt& end, int pts, BaseStroke* proto, Bface* f);
   //void connect_draw(ProxyUVStroke* srk, int k, Bface* face);
   //void build_stroke(ProxyStroke* rendered_stroke, ProxyUVStroke* stroke_part, int k, Bface* face);
};



#endif  // HATCHING_TEXTURE_H

// end of file hatching_texture.H
