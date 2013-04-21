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
/*****************************************************************
 * panel.H
 *****************************************************************/
#ifndef PANEL_H_IS_INCLUDED
#define PANEL_H_IS_INCLUDED

#include "mesh/lmesh.H"
#include "map3d/bnode.H"
#include "bcurve.H"
#include "bsurface.H"
#include "blending_meme.H"
#include "geom/command.H"

/*******************************************************
 * PCellCornerVertFilter:
 *
 *  Accepts a vertex whose if it lies on a "corner" of
 *  a PCell. I.e., either a Bpoint controls the vertex
 *  or more than one PCell is adjacent to the vertex.
 *  (PCells are defined right below.)
 *******************************************************/
class PCellCornerVertFilter: public SimplexFilter {
 public:
   PCellCornerVertFilter() {}

   virtual bool accept(CBsimplex* s) const;
};

/*******************************************************
 * PCell:
 *
 *  Represents a connected set of faces inside a Panel;
 *  each cell corresponds to a skeleton vertex. Skeletons
 *  are used to inflate panels into "paper dolls."
 *******************************************************/
class Panel;
class PCell;
typedef ARRAY<PCell*> PCell_list;
class PCell {
 public:
   PCell(Panel* p, CBface_list& f) : _panel(p), _faces(f) {
      assert(p != NULL);
   }

   //******** ACCESSORS ********
   Bface_list faces() const { return _faces; }

   //******** TESTS ********
   bool is_empty()           const { return _faces.empty(); }
   bool contains(Bface* f)   const { return _faces.contains(f); }

   //******** GEOMETRY ********
   Wpt center() const;

   //******** CONNECTIVITY ********

   bool same_panel(const PCell* c) const {
      return (c && c->_panel == _panel);
   }

   // return list of neighboring cells that share a boundary
   // edge with this cell
   PCell_list nbrs() const;

   // neighbors from the same Panel:
   PCell_list internal_nbrs() const;

   // neighbors from another Panel:
   PCell_list external_nbrs() const;

   // boundary of the cell:
   EdgeStrip get_boundary() const { return faces().get_boundary(); }

   // boundary edges:
   Bedge_list boundary_edges() const {
      return faces().boundary_edges();
   }

   // edges common to this cell and the given one:
   Bedge_list shared_boundary(PCell* c) const {
      return boundary_edges().intersect(c->boundary_edges());
   }

   // list of edges in the common boundary between any 2 cells in the list:
   static Bedge_list shared_edges(const PCell_list& cells);

   // counts number of corners:
   int num_corners() const {
      return faces().get_boundary().edges().get_verts().
         filter(PCellCornerVertFilter()).num();
   }

 protected:
   Panel*            _panel;
   Bface_list        _faces;
};

/*******************************************************
 * Panel
 *
 *   Bsurface that fills in a given boundary.
 *******************************************************/
class Panel:  public Bsurface {
 public:

   //******** CREATION ********

   // Create a Panel to fill a simple closed boundary:
   static Panel* create(CBcurve_list& contour, CARRAY<int> ns = ARRAY<int>());
   static Panel* create(CBvert_list& verts);

   // Create a Panel to fill multiple boundaries:
   static Panel* create(CARRAY<Bcurve_list>& contours);

   //******** MANAGERS ********

   // The do-nothing constructor:
   Panel() {}

   // Creates a child Panel of the given parent:
   Panel(Panel* parent);

   virtual ~Panel();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Panel", Panel*, Bsurface, CBnode*);

   //******** STATICS ********

   static bool toggle_show_skel() { return (_show_skel = !_show_skel); }
   static bool is_showing_skel()  { return _show_skel; }

   // Returns the Panel that owns the boss meme on the simplex:
   static Panel* find_owner(CBsimplex* s) {
      return upcast(Bsurface::find_owner(s));
   }

   //******** CELLS ********

   static PCell* find_cell(Bface* f) {
      Panel* p = Panel::find_owner(f);
      if (!p) return 0;
      for (int i=0; i<p->_cells.num(); i++)
         if (p->_cells[i]->contains(f))
            return p->_cells[i];
      return 0;
   }

   static PCell_list find_cells(CBface_list& faces) {
      PCell_list ret;
      for (int i=0; i<faces.num(); i++) {
         PCell* c = find_cell(faces[i]);
         if (c)
            ret.add_uniquely(c);
      }
      return ret;
   }

   //********* re-tessellation ******
   bool re_tess               (PIXEL_list pts, bool scribble = false);
   bool re_tess               (CBcurve_list& cs, CARRAY<int>& ns);

   //********* oversketch *****

   // Oversketch of skeleton
   bool oversketch(CPIXEL_list& sketch_pixels);

   // The skeleton pts
   Wpt_list skel_pts();

   //******** Bbase VIRTUAL METHODS ********

   // Clone a little Panel:
   virtual void produce_child();

   //******** RefImageClient METHODS

   virtual int draw(CVIEWptr&);

   //******** Bnode METHODS ********

   virtual Bnode_list inputs() const;

   //******** BMESHobs NOTIFICATION METHODS ********

 protected:

   //******** INTERNAL DATA ********

   PCell_list   _cells;

   static bool _show_skel; // for testing cells/skeletons

   // Panel at control level:
   Panel* control() const { return upcast(Bbase::control()); }

   //******** CELLS ********

   void draw_skeleton() const;
   void add_cell(CBface_list& faces);
   void add_cell(FaceMeme* fm);

   //******* OVERSKETCH HELPER ****

   // input parameters:
   // new_centers - the new desired skeleton pts
   // visited - is this panel visited in the recursive process
   // map_locs - maps verts on the boundaries to new locations
   // cmd - for undo and redo
   // default_old_t, default_new_t - useful only when panel has a single cell
   bool apply_skel(CWpt_list& new_centers, ARRAY<Panel*>& visited, 
      map<Bvert*, Wpt>& map_locs, MULTI_CMDptr cmd, 
      Wvec default_old_t, Wvec default_new_t);

   // The neighboring panel of the current panel, which shares the same
   // bcurve: c
   Panel* nbr_panel(Bcurve* c);

   //******** TESSELLATION ********

   // Prepare for fresh tessellation (wipe old stuff):
   bool prepare_tessellation(CBcurve_list& contour);
   bool prepare_tessellation(LMESH* m);

   bool tessellate_disk       (CBvert_list&  verts, bool create_mode = true);
   bool tessellate_disk       (CBcurve_list& contour, CARRAY<int> ns = ARRAY<int>());
   bool tessellate_banded_disk(CBcurve_list& contour);
   bool tessellate_quad       (CBcurve_list& contour, CARRAY<int> ns = ARRAY<int>());
   bool tessellate_tri        ( Bcurve_list  contour, ARRAY<int> ns = ARRAY<int>());
   bool tessellate_multi      (CARRAY<Bcurve_list>& contours);

   bool inc_sec_tri           (Bcurve* bc1, Bcurve* bc2, CPIXEL_list& pts, bool scribble);
   bool inc_sec_quad          (Bcurve* bc1, Bcurve* bc2, CPIXEL_list& pts, bool scribble);
   bool inc_sec_disk          (Bcurve* bc1, Bcurve* bc2, CPIXEL_list& pts, bool scribble);
   bool split_tri             (Bpoint* bp,  Bcurve* bc,  CPIXEL_list& pts, bool scribble);
   bool try_re_tess_adj       (Bcurve* bc, int n);

   // Called after tessellation.
   // May fail if error conditions are detected.
   bool finish_tessellation();

   // Internal helper method for tessellating a quadrilateral
   // region.  Chooses the number of edges in each dimension,
   // aiming for good aspect ratios. Assumes the curves have been
   // screened so the average "horizontal" length is more than
   // the average "vertical" length:
   static bool choose_quad_resolution(
      Bcurve* h1,  // horizontal curve 1
      Bcurve* h2,  // horizontal curve 2
      Bcurve* v1,  // vertical curve 1
      Bcurve* v2   // vertical curve 2
      );

   //******** MEMES ********

   // XXX - should these be in Bbase?
   VertMeme* add_vert_meme (Lvert* v);
   void      add_vert_memes(CBvert_list& verts);
};

/*****************************************************************
 * PNL_RETESS_CMD
 *
 *****************************************************************/
MAKE_PTR_SUBC(PNL_RETESS_CMD,COMMAND);
class PNL_RETESS_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   PNL_RETESS_CMD(Panel* p, CBcurve_list& cs, CARRAY<int>& ns);

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("PNL_RETESS_CMD", PNL_RETESS_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Panel*   _p;
   Bcurve_list    _old_cs, _new_cs;
   ARRAY<int>     _old_ns, _new_ns;
};

#endif // PANEL_H_IS_INCLUDED

// end of file panel.H
