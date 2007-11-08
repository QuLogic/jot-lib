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
#ifndef DISK_MAP_H_IS_INCLUDED
#define DISK_MAP_H_IS_INCLUDED

#include "map3d/map0d3d.H"
#include "subdiv_updater.H"

/*****************************************************************
 * DiskMap:
 *
 *   Defines a coordinate frame WRT a "disk", i.e. a region
 *   of a mesh (represented by a Bface_list) that has the
 *   topology of a disk. The frame consists of:
 *     
 *     o: origin, associated with disk (e.g. center)
 *     t: vector along disk normal at o
 *     n: vector provided by Map0D3D::set_norm()
 *     b: n x t
 *
 *   Also stores a local coordinate WRT the coordinate
 *   frame. The map() function returns the local coordinate
 *   transformed by the coordinate frame.
 *
 *   For an example, see tess/primitive.C, where a DiskMap
 *   is used to define the location of a skeleton Bpoint so
 *   that it tracks the base surface to which the primitive
 *   is attached.
 *
 *****************************************************************/
class DiskMap : public Map0D3D {
 public:

   //******** MANAGERS ********

   // No public constructor. The following returns a DiskMap
   // if the given set of faces topologically forms a disk
   // (see Bface_list::is_disk()), and if the maximum normal
   // deviation (in radians) is below the given threshold:
   static DiskMap* create(CBface_list& disk, bool flip_tan=false, Bvert* v=NULL);

   virtual ~DiskMap();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("DiskMap", DiskMap*, Map0D3D, CBnode*);

   //******** ACCESSORS ********

   CBface_list& faces() const { return _disk; }

   //******** Map0D3D VIRTUAL METHODS ********

   // Output location is the local coord transformed by xf:
   virtual Wpt  map() const    { return _xf * _loc; }

   // Redefine the local coords so map() will return p:
   virtual bool set_pt(CWpt& p);

   // Stores n minus its projection along t:
   virtual void set_norm(CWvec& n);

   //******** MAP VIRTUAL METHODS ********

   // You can transform the map...
   virtual bool can_transform()         const    { return true; }

   // ...but it just ignores it. The reason is that the transform
   // is presumably applied to the mesh, and that takes care of it,
   // because this depends on the mesh:
   virtual void transform(CWtransf&, CMOD&) {}

   //******** Bnode VIRTUAL METHODS ********

   // Single input: the SubdivUpdater:
   virtual Bnode_list inputs()  const { return Bnode_list(_updater); }

   // Recompute cached values from a changed disk:
   virtual void recompute();

 protected:

   //******** MEMBER DATA ********

   Bface_list           _disk;          // the region
   Bvert*               _v;            // vert serving as o
   SubdivUpdater*       _updater;       // updater for base region
   Wpt            _loc;           // location in local coords
   bool                 _flip_tan;      // if true, flip the tan direction

   // computed from _disk and cached here:
   Wpt            _o;             // coordinate frame origin
   Wtransf        _xf;            // local frame: [o t n b]

   //******** MANAGERS ********

   // Created in create(), set up in recompute():
   DiskMap() : _updater(0), _flip_tan(false) {}

   //******** UTILITIES ********

   // This map maintains a coordinate frame, plus a local
   // coordinate WRT that frame. The following returns the
   // coordinate frame, which is different from Map0D3D::frame()
   // only in the origin. (Here it is the disk origin, in
   // Map0D3D::frame() it is the location returned by map()).
   CWtransf& xf() const { return _xf; }
};

#endif // DISK_MAP_H_IS_INCLUDED

// end of file disk_map.H
