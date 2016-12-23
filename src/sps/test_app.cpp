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
#include "test_app.hpp"
#include "gtex/smooth_shade.hpp"  // used in BALLwidget_anchor
#include "gtex/wireframe.hpp"  // used in GRIDwidget_anchor

const int HEIGHT = 6;
const double REGULARITY = 20;
const double MIN_DIST = 0.35;

/*****************************************************************
 * BALLwidget_anchor:
 *
 *      Class used for those sample point balls
 *****************************************************************/
MAKE_PTR_SUBC(BALLwidget_anchor, GEOM);
class BALLwidget_anchor : public GEOM {
 public:
   BALLwidget_anchor() : GEOM() {
      // Start with an empty LMESH:
      LMESHptr mesh = make_shared<LMESH>();

      // Ensure mesh knows its containing GEOM:
      mesh->set_geom(this);

      // Make a ball shape
      mesh->Icosahedron();

      // Regardless of current rendering mode,
      // always use Gouraud shading:
      mesh->set_render_style(SmoothShadeTexture::static_name());

      // Color the Patch blue:
      mesh->patch(0)->set_color(COLOR(0.1, 0.1, 0.9));
//      mesh->patch(0)->set_transp(0.9);

      // Store the mesh:
      _body = mesh;

      set_name("BALLwidget_anchor");
   }

   // We're not in the picking game:
   virtual int draw_vis_ref() { return 0; }

   virtual bool needs_blend() const { return false; }
};

/*****************************************************************
 * GRIDwidget_anchor:
 *
 *      Class used for those octree grid
 *****************************************************************/
MAKE_PTR_SUBC(GRIDwidget_anchor, GEOM);
class GRIDwidget_anchor : public GEOM {
 public:
   GRIDwidget_anchor(Wtransf ff) : GEOM() {
      // Start with an empty LMESH:
      LMESHptr mesh = make_shared<LMESH>();

      // Ensure mesh knows its containing GEOM:
      mesh->set_geom(this);

      // Make a cube shape
      mesh->Cube();
      set_xform(ff);

      // Regardless of current rendering mode,
      // always use wireframe shading:
      mesh->set_render_style(WireframeTexture::static_name());

      // Color the Patch black:
      mesh->patch(0)->set_color(COLOR::black);
      mesh->patch(0)->set_transp(0.5);

      // Store the mesh:
      _body = mesh;

      set_name("GRIDwidget_anchor");
   }

   // We're not in the picking game:
   virtual int draw_vis_ref() { return 0; }

   virtual bool needs_blend() const { return false; }
};

/*****************************************************************
 * TestSPSapp Methods
 *****************************************************************/
TestSPSapp* TestSPSapp::_instance = nullptr;

void
TestSPSapp::create_grid(Wtransf ff)
{
      _boxes.push_back(new GRIDwidget_anchor(ff));
      GEOMptr _anchor = _boxes.back();
      _anchor->set_pickable(0); // Don't allow picking of anchor
      NETWORK.set(_anchor, 0);// Don't network the anchor
      CONSTRAINT.set(_anchor, GEOM::SCREEN_WIDGET);
      WORLD::create   (_anchor, false); 
      WORLD::undisplay(_anchor, false);
}

void
TestSPSapp::visit(OctreeNode* node)
{
   if (node->get_leaf()) {
      if (node->get_disp()) {
         Wtransf ff = Wtransf(node->min()) * Wtransf::scaling(node->dim());
         create_grid(ff);
      }
   } else {
      for (int i = 0; i < 8; i++)
         visit(node->get_children()[i]);
   }
}

void
TestSPSapp::load_scene()
{
   BaseJOTapp::load_scene();

   for (int i=0; i<EXIST.num(); i++) {
      GEOMptr geom = GEOM::upcast(EXIST[i]);
      if (geom && dynamic_pointer_cast<BMESH>(geom->body())) {
         Bvert_list list;
         Bface_list fs;
         vector<Wvec> bcs;
         double spacing;
         //generate_samples(dynamic_pointer_cast<BMESH>(geom->body()), 1.0, fs, bcs);
         //generate_samples(dynamic_pointer_cast<BMESH>(geom->body()), fs, bcs, HEIGHT, MIN_DIST);
         _nodes.push_back(sps(dynamic_pointer_cast<BMESH>(geom->body()), HEIGHT, REGULARITY, MIN_DIST, fs, bcs, spacing));
         for (Bface_list::size_type i = 0; i < fs.size(); i++) {
            Wpt pt;
            fs[i]->bc2pos(bcs[i], pt);
            list.push_back(new Bvert(pt));
         }
         _pts.push_back(list);
      }
   }

   for (auto & node : _nodes)
      visit(node);

   for (auto & point : _pts) {
      for (Bvert_list::size_type j = 0; j < point.size(); j++) {
         _balls.push_back(new BALLwidget_anchor);
         GEOMptr _anchor = _balls.back();
         _anchor->set_pickable(0); // Don't allow picking of anchor
         NETWORK.set(_anchor, 0);// Don't network the anchor
         CONSTRAINT.set(_anchor, GEOM::SCREEN_WIDGET);
         WORLD::create   (_anchor, false);
         WORLD::undisplay(_anchor, false);
      }
   }
}

void
TestSPSapp::init_kbd(WINDOW &base_window)
{
   // Set up keyboard callbacks.

   BaseJOTapp::init_kbd(base_window);

   // Add key commands specific to test app:
   _key_menu->add_menu_item('s', "Toggle Sample",      &toggle_sample_cb);
   _key_menu->add_menu_item('g', "Toggle Grid",      &toggle_grid_cb);
}

int
TestSPSapp::toggle_sample_cb(const Event&, State *&)
{
   // Switch between showing and hiding samples

   assert(_instance);
   _instance->_show_sample = !_instance->_show_sample;
   if (_instance->_show_sample) {
      vector<Bvert_list>::size_type i;
      vector<GEOMptr>::iterator k;
      for (i = 0, k = _instance->_balls.begin(); i < _instance->_pts.size(); i++) {
         for (Bvert_list::size_type j = 0; j < _instance->_pts[i].size(); j++, ++k) {
            WORLD::display(*k, false);
            Wpt p = _instance->_pts[i][j]->loc();
            Wvec    delt(p-Wpt(XYpt(p)+XYvec(VEXEL(3,3)), p));
            Wtransf ff = Wtransf(p) * Wtransf::scaling(1.5*delt.length()); 
            (*k)->set_xform(ff);
         }
      }
      err_msg("showing sample");
   }
   else {
      vector<GEOMptr>::iterator i;
      for (i = _instance->_balls.begin(); i != _instance->_balls.end(); ++i)
         WORLD::undisplay(*i, false);
      err_msg("samples not shown");
   }

   return 0;
}

int
TestSPSapp::toggle_grid_cb(const Event&, State *&)
{
   vector<GEOMptr>::iterator i;
   // Switch between showing and hiding grid

   assert(_instance);
   _instance->_show_grid = !_instance->_show_grid;
   if (_instance->_show_grid) {
      for (i = _instance->_boxes.begin(); i != _instance->_boxes.end(); ++i)
         WORLD::display(*i, false);
      err_msg("showing grid");
   }
   else {
      for (i = _instance->_boxes.begin(); i != _instance->_boxes.end(); ++i)
         WORLD::undisplay(*i, false);
      err_msg("grid not shown");
   }

   return 0;
}

/**********************************************************************
 * main()
 **********************************************************************/
int
main(int argc, char **argv)
{
   TestSPSapp app(argc, argv);

   // Define the program name displayed in the window title bar:
   Config::set_var_str("JOT_WINDOW_NAME", "Stratified Point Sampling test app");

   app.init();
   app.Run();

   return 0;
}
