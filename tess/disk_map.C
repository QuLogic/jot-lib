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
#include "disk_map.H"

static bool debug = Config::get_var_bool("DEBUG_DISK_MAP",false);

DiskMap* 
DiskMap::create(CBface_list& disk, bool flip_tan, Bvert* v)
{
   err_adv(debug, "DiskMap::create");

   // Check topology conditions:
   if (!disk.is_disk()) {
      err_adv(debug, " %d faces do not form disk", disk.num());
      return 0;
   }

   if (v && !disk.get_verts().contains(v)) {
      err_adv(debug, " input vert not on the input faces");
      return 0;
   }

   DiskMap* ret = new DiskMap;
   ret->_disk = disk;           // set disk
   ret->_v = v;              // default
   ret->_flip_tan = flip_tan;   // record whether to flip "tangent" dir
   ret->invalidate();           // force recompute

   SubdivUpdater* su = SubdivUpdater::create(disk);
   if (su->inputs().empty()) {
      // This will happen if the disk is at the control level and is
      // not procedurally controlled at all. Then it should never
      // change and we have no input. We are assuming changes to the
      // mesh only happen via Bnodes (Bsurfaces etc.).
      err_adv(debug, " not using SubdivUpdater");
      delete su;
      su = 0;
   } else {
      ret->_updater = su;
      ret->hookup();
   }

   if (debug)
      ret->print_dependencies();

   ret->update();

   err_adv(debug, " succeeded");

   return ret;
}

DiskMap::~DiskMap() 
{
   destructor();
   delete _updater;     // we created it, we delete it
   _updater = 0;
}

void
DiskMap::recompute()
{
   // Recompute cached values from a changed disk:

   assert(!_disk.empty());

   _o = _v ? _v->loc(): _disk.get_verts().center();
   _t = _disk.avg_normal();
   if (_flip_tan)
      _t = -_t;
   if (_v) // XXX - better policy needed
      _n = (_disk.get_verts().center()-_o).orthogonalized(_t).normalized();
   else 
      _n = _n.orthogonalized(_t).normalized(); 
   if (_n.is_null())
      _n = _t.perpend();
   _xf = Wtransf(_o, _t, cross(_n, _t));
   
   if (debug && _debug) {
      cerr << identifier() << " recomputing" << endl;
      cerr << "CoordFrame* " << (CoordFrame*)this << endl;
      cerr << "o: " << o() << endl;
      cerr << "t: " << t() << endl;
      cerr << "b: " << b() << endl;
      cerr << "n: " << n() << endl;
   }
}

void 
DiskMap::set_norm(CWvec& n) 
{
   _n = n;
   invalidate(); 
}

bool
DiskMap::set_pt(CWpt& pt)
{
   update();
   _loc = _xf.inverse() * pt;
   outputs().invalidate();
   return true;
}

// end of file disk_map.C
