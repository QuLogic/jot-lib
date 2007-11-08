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
 * zcross_path.H:
 **********************************************************************/
#ifndef ZCROSS_PATH_H_IS_INCLUDED
#define ZCROSS_PATH_H_IS_INCLUDED

#include "bvert.H"
#include "bsimplex.H"

/*!
 *  \brief A segment of a zero crossing line on a surface.
 *
 *  This is meant to be part of a sequence (like an ARRAY or vector) of ZXseg's
 *  that form one of more zero crossing lines.  Each ZXseg doesn't actually hold
 *  a segment, it just has a point that should be connected (to form segments) to
 *  the points in the ZXseg's before and after it in the sequence.  Some ZXseg's
 *  can also be marked with an end flag.  The point in a ZXseg marked with the
 *  end flag is the last point in a zero crossing line.  A ZXseg after one marked
 *  with the end flag in a sequence is the beginning of a new zero crossing line.
 *
 *  \note This class is mainly geared towards representing zero crossing lines of
 *  N dot V (silhouettes) on a mesh (and has some things specific to that purpose
 *  built in).  However it has also been used (in a somewhat hackish fashion) to
 *  represent other types of zero crossing lines.
 *
 */
class ZXseg {
   
   protected:
   
      //! \brief Starting point(location) of the segment.
      //! 
      //! This will be connected to the point in the next ZXseg in the sequence
      //! to form a segment.  Likewise, the point in the previous ZXseg in the
      //! the sequence (if there is one) will be connect to this to form a segment.
      Wpt  _p;
      
      //! \brief Ether a segment or a face a segment lies on.
      //!
      //! In general this is the face that the segment starting at \p _p will
      //! pass through.  Sometimes it might be an edge instead (though it unclear
      //! at the time of this writing if or when this occurs).
      Bsimplex*  _s;
      
      //! \brief Starting vertex of the segment
      //!
      //! \question What is this used for?
      //! \question Is this a hold over from the older edge representation of
      //! silhouettes that this was derived from?
      Bvert*     _v;
      
      //! \brief Confidence of point
      //!
      //! The confidence of the point on the zero crossing line between 0.0 and
      //! 1.0.  0.0 indicates the point isn't very important (which usually means
      //! it shouldn't be drawn).  1.0 indicates the point is very important
      //! which usually means to always draw it.
      //!
      //! \note Used to be "Direction of gradiant"
      double     _g;
      
      //! \brief Barycentric coordinates
      //!
      //! Barycentric coordinates of the point on the Bsimplex \p _s.
      Wvec _bc;
      
      //! \brief Stroke Type
      //!
      //! The type of the stroke.  Used at some point in the rendering process
      //! to determine the style of the stroke to draw for the zero crossing
      //! line containing this ZXseg.
      int        _type;
      
      //! \brief Termination Deliminter
      //!
      //! Flags this ZXseg as the end of a contiguous zero crossing line.
      bool       _end;
      
   public:
   
      //! \name Constructors
      //@{
      
      ZXseg(Bsimplex* s, CWpt &pt, Bvert * v, bool grad, mlib::CWvec& bc,
            int type = 0, bool end = 0)
         : _p(pt),
           _s(s),
           _v(v),
           _g(grad ? 1.0 : 0.0),
           _bc(bc),
           _type(type),
           _end(end)
      {}
   
      ZXseg(Bsimplex* s, CWpt &pt, Bvert * v, double conf, mlib::CWvec& bc,
            int type = 0, bool end = 0)
         : _p(pt),
           _s(s),
           _v(v),
           _g(conf),
           _bc(bc),
           _type(type),
           _end(end)
      {}

      //! \brief Constructor for case of null face.
      ZXseg(Bsimplex* s, CWpt &pt, Bvert * v, bool grad, Bsimplex* bary_s,
            int type = 0, bool end = 0)
         : _p(pt),
           _s(s),
           _v(v),
           _g(grad ? 1.0 : 0.0),
           _type(type),
           _end(end)
      {
         // this is a slow calc for now, but needed when we
         // don't know what face we're on
   
         // Project_barycentric is a vertual function so the
         // correct definition will be called depending if bary_s
         // is edge of face
         if (bary_s)
            bary_s->project_barycentric ( _p, _bc );
      }   

      ZXseg ()
      {
         _s = NULL;
         _v = NULL;
         _g = false;
   
      }
      
      //@}
      
      //! \name Accessor Functions
      //@{
      
      bool       end()    const { return _end; }
      Wpt& p()            { return _p; }  
      Bsimplex*  s()      const { return _s; }
      //! if we really really want the face we can get it
      //! if no face is there you will get a NULL
      Bface*     f()      const { return get_bface(_s); }
      Bvert*     v()      const { return _v; }
      bool       g()      const { return _g > 0.0; }
      double     conf()   const { return _g; }
      int        type()   const { return _type; }
      CWvec& bc()   const { return _bc; }
   
      
      void set_end()          { _end = true; }
      void setf(Bsimplex* s)  { _s = s; }                             
      void setv(Bvert* v)     { _v = v; }
      void setg(bool g)       { _g = g ? 1.0 : 0.0; }
      void setg(double g)     { _g = g; }
      void settype(int t)     { _type = t;}
      void set_bary( Bsimplex* s )
         { if(s) s->project_barycentric ( _p, _bc); }
      
      //@}
      
      //! \name Overloaded Operators
      //@{
         
      bool operator ==( const ZXseg& z) const
         {return _p==z._p && _s==z._s && _v==z._v && _g==z._g;}
      
      //@}

};

#define CZXseg const ZXseg

class ZcrossCB;
/**********************************************************************
 * class to define zero crossing silhouettes on a mesh.
 **********************************************************************/
#define CZcrossPath const ZcrossPath
class ZcrossPath
{
public:
   //******** MANAGERS ********

   // Create an empty strip:
   ZcrossPath() : _patch(0), _index(-1) {}

   // Given a list of edges to search, build a strip of
   // edges that satisfy a given property.
   ZcrossPath(CBface_list& list) :
         _patch(0),
         _index(-1)
   {

      build(list);
   }

   virtual ~ZcrossPath() {}

   //******** BUILDING ********

   void build(CBface_list& list);

   bool has_sil(Bface* f); // is it a silhouette face?

   void start_sil(Bface * f); //find legal face, build silpaths
   void get_g(Bface*f , double g[3], int& ex1, int& ex2);

   Bface* sil_walk_search ( Bface * f , Bvert* vert[3], double g[3] ); //better search algorithm
   Bface* sil_search (Bface * last , Bface *f); //continue on a good build
   Bface* sil_finish (Bface * last , Bface *f); //find the far point (at breaks)

   // bool has_sil(Bface* f, double& g1, double& g2, double& g3, Wpt& x1, mlib::Wpt&x2);

   // Continue building the strip from a given edge type,
   // starting at the given edge and one of its vertices.
   // If the vertex is nil a vertex is chosen from the edge
   // arbitrarily.


   // Add another segment to the strip:
   //void add(Bface* f, CWpt& pt, bool grad) { _faces += f; _points += pt; _grads += grad; }
   //void add_vert(Bvert* v ) { _verts += v; }

   void add_seg ( Bface* f , CWpt& pt, Bvert* v, bool grad, mlib::CWvec& bc)
   {
      bool end = (f) ? false : true;      
      _segs += ZXseg ( f, pt, v, grad, bc, 0, end );
   }
   void add_seg ( const ZXseg& seg) { _segs += seg;  }

   void add_seg ( Bface* f , CWpt& pt, Bvert* v, bool grad, Bface * bary_f)
   {
      bool end = (f) ? false : true;       
      _segs += ZXseg ( f, pt, v, grad, bary_f, 0 ,end);
   } 

   // Clear the strip:
   virtual void reset() { _segs.clear(); }

   //******** ACCESSORS ********
   Patch*         patch()        const { return _patch; }
   CARRAY<ZXseg>& segs()         const { return _segs; }
   ARRAY<ZXseg>&  segs()         { return _segs; }



   Bface* face(int i)            const { return _segs[i].f(); }
   CWpt& point(int i)            const { return _segs[i].p(); }
   Bvert* vert(int i )           const { return _segs[i].v(); }
   bool grad(int i)              const { return _segs[i].g(); }
   CWvec& bc(int i)              const { return _segs[i].bc();}
   int type(int i)               const { return _segs[i].type();}

   ZXseg&  seg(int i )                 { return _segs[i]; }
   void set_eye(CWpt& eye )     { _eye = eye; }
   CWpt& eye()                  const   { return _eye; }
   CWpt& first()                const { return _segs[0].p(); }
   CWpt& last()                 const { return _segs[num()-1].p(); }

   //bool   empty()               const { return _points.empty(); }
   bool   empty()               const { return _segs.empty(); }
   //int    num()                 const { return _points.num(); }
   int    num()               const { return _segs.num(); }

   BMESH* mesh() const
   {
      return empty() ? (BMESH*)0 : _segs[0].f()->mesh();
   }

   //******** THE MAIN JOB ********


protected:
   friend class Patch;
   //******** DATA ******** 
   ARRAY <ZXseg>        _segs;  
   Patch*               _patch;       // patch this is assigned to
   int                  _index;       // index in patch's list
   Wpt                  _eye;         // eye_local ( set by bmesh )
   //******** PROTECTED METHODS ********

   // used internally when generating
   // strips of edges of a given type:

   // setting the Patch -- nobody's bizness but the Patch's:
   // (to set it call Patch::add(ZcrossPath*))
   void   set_patch(Patch* p)           { _patch = p; }
   void   set_patch_index(int k)        { _index = k; }
   int    patch_index() const           { return _index; }
};

// XXX - transplanted from npr/zxedge_stroke_texture.H 7/10/2005:
// TYPES of LINES  that zxedge_stroke ( and thus sil&crease) may encounter
enum { STYPE_SIL=0, STYPE_BF_SIL, STYPE_BORDER, STYPE_CREASE, STYPE_SUGLINE, STYPE_WPATH, STYPE_POLYLINE, STYPE_NUM } ;

#endif // ZCROSS_PATH_H_IS_INCLUDED

/* end of file zcross_path.H */
