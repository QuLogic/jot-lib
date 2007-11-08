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

#include "base_jotapp/base_jotapp.H"
#include "sps.H"

class TestSPSapp : public BaseJOTapp {
 public:

   //******** MANAGERS *******/
   TestSPSapp(int argc, char **argv) :
      BaseJOTapp(argc, argv) {
      assert(_instance == 0);
      _instance = this;
      _show_sample = false;
      _show_grid = false;
   }

   ~TestSPSapp() {
      _pts.clear();
      _balls.clear();
      _boxes.clear();
      _nodes.clear();
   }

   //******** KEYBOARD CALLBACKS ********
   static int toggle_sample_cb(const Event&, State *&);
   static int toggle_grid_cb(const Event&, State *&);

   //******** BaseJOTapp METHODS ********

 protected:
   virtual void load_scene();
   virtual void init_kbd(WINDOW &base_window);
   void create_grid(Wtransf ff);
   void visit(OctreeNode* node);

   //******** MEMBER DATA ********

   static TestSPSapp* _instance;
   ARRAY<Bvert_list> _pts;
   LIST<GEOMptr> _balls;
   LIST<GEOMptr> _boxes;
   ARRAY<OctreeNode*> _nodes;
   bool _show_sample;
   bool _show_grid;
};

// end of file test_app.H
