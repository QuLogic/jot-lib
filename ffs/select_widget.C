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
 * select_widget.C
 *****************************************************************/

/*!
 *  \file select_widget.C
 *  \brief Contains the definition of the SELECT_WIDGET widget.
 *
 *  \ingroup group_FFS
 *  \sa select_widget.H
 *
 */

#include "geom/gl_view.H"     // for GL_VIEW::draw_pts()
#include "gtex/ref_image.H"
#include "mesh/lmesh.H"
#include "mesh/mi.H"
#include "mesh/mesh_select_cmd.H"
#include "tess/tex_body.H"

#include "ffs/floor.H"
#include "ffs/select_widget.H"

using namespace mlib;

/*****************************************************************
 * STATIC VARIABLES
 *****************************************************************/
static bool debug = Config::get_var_bool("DEBUG_SELECT_WIDGET_ALL",false);

static const double SEARCH_RAD =
Config::get_var_dbl("SELECT_WIDGET_SEARCH_RAD",8.0);

static const double MIN_PIX_AREA = 
Config::get_var_dbl("SELECT_WIDGET_MIN_PIX_AREA",20);

/*****************************************************************
 * UTILITIES
 *****************************************************************/

//! Return true if the barycentric coordinate bc corresponds
//! to a location near the center of the face.
inline bool
is_near_center(Bface* f, CWvec& bc, double margin=0.25)
{

   if (!f)
      return false;
   if (f->is_quad()) {
      UVpt uv = f->quad_bc_to_uv(bc);
      return (uv[0] > margin && uv[0] < 1 - margin &&
              uv[1] > margin && uv[1] < 1 - margin);
   } else {
      return (bc[0] > margin && bc[1] > margin && bc[2] > margin);
   }
}

inline double
pix_area(Bface* f)
{
   if (!f) return 0;

   // get ndc area; if quad, take half of quad ndc area:
   double a = f->is_quad() ? (f->quad_ndc_area()/2) : f->ndc_area();

   // multiply by scale factor to change to PIXEL units:
   return a * sqr(VIEW::peek()->ndc2pix_scale());
}

/*!
	Find the nearest Bface at the given screen location pix,
	(within the search radius rad), then re-map it to the
	corresponding edit-level face and barycentric
	coordinates bc.
 */
inline Bface*
find_face(CPIXEL& pix, double rad, Wvec& bc)
{

   Bface* f = VisRefImage::get_edit_face(bc, pix, rad);

   // Only select faces belonging to a TEXBODY
   if (!(f && f->mesh() && TEXBODY::isa(f->mesh()->geom())))
      return 0;

   return f;
}

/*!
	Same as above, but ensures the pix location corresponds
	to a barycentric coordinate separated from the face
	edges by the given margin, and that the face area (in
	PIXELs) exceeds the given minimun.
 */
inline Bface*
find_face(CPIXEL& pix, double margin, double min_area)
{

   Wvec bc;
   Bface* f = find_face(pix, 1, bc);
   if (!is_near_center(f, bc, margin))
      return 0;
   if (pix_area(f) < min_area)
      return 0;
   return f;
}

/*!
	Based on the barycentric coord, return the
	nearest edge unless the location is too near
	a vertex or the face center.
 */
inline Bedge*
near_edge(Bface* f, CWvec& bc)
{

   if (!f)
      return 0;
   if (max(max(bc[0],bc[1]),bc[2]) > 0.75)
      return false;     // too near a vertex
   double m = min(min(bc[0],bc[1]),bc[2]);
   if (m > 0.25)
      return 0;         // too near the center

   // If min barycentric coord is bc[0] ( == v1), then
   // nearest edge is the one across, i.e. e2, etc.
   if (m == bc[0])
      return f->e2();
   if (m == bc[1])
      return f->e3();
   return f->e1();
}

inline Bedge*
find_edge(CPIXEL& pix)
{
   Wvec bc;
   Bedge* e = near_edge(find_face(pix, 1, bc), bc);
   if (e && e->is_weak())
      return 0;
   return e;
}

//! Based on the barycentric coord, return the nearest vert
//! unless the location is too far from any.
inline Bvert*
near_vert(Bface* f, CWvec& bc)
{

   if (!f)
      return 0;
   double m = max(max(bc[0],bc[1]),bc[2]);
   if (m < 0.75)
      return false;     // too far from any vertex

   // If max barycentric coord is bc[0] ( == v1), then
   // nearest vert is v1, etc.
   return (m == bc[0]) ? f->v1() : (m == bc[1]) ? f->v2() : f->v3();
}

inline Bvert*
find_vert(CPIXEL& pix)
{
   Wvec bc;
   return near_vert(find_face(pix, 1, bc), bc);
}

//! Find vertex of f whose cur subdiv vert is closest to
//! the given screen-space point p.
inline Bvert*
vert_min_subdiv_dist(Bface* f, CPIXEL& p)
{

   if (!f) return 0;

   if (!LMESH::isa(f->mesh()))
      return closest_vert(f, p);

   Lvert* v1 = ((Lvert*)f->v1())->cur_subdiv_vert();
   Lvert* v2 = ((Lvert*)f->v2())->cur_subdiv_vert();
   Lvert* v3 = ((Lvert*)f->v3())->cur_subdiv_vert();

   if (!(v1 && v2 && v3))
      return 0;

   double d1 = p.dist(PIXEL(v1->wloc()));
   double d2 = p.dist(PIXEL(v2->wloc()));
   double d3 = p.dist(PIXEL(v3->wloc()));

   if (d1 < d2)
      return (d3 < d1) ? f->v3() : f->v1();
   else
      return (d3 < d2) ? f->v3() : f->v2();
}

/*!
	Given edge e containing endpoint v, return the subdiv
	verts at the current level, in order starting at v and
	running to the other endpoint.
 */
inline Bvert_list
get_cur_level_verts(Bvert* v, Bedge* e)
{

   assert(v && e && e->contains(v));

   Bvert_list ret;

   LMESH* mesh = LMESH::upcast(e->mesh());
   if (mesh) {
      ((Ledge*)e)->get_subdiv_verts(mesh->rel_cur_level(), ret);
   } else {
      ret += e->v1();
      ret += e->v2();
   }
   if (e->v1() != v)
      ret.reverse();
   return ret;
}

/*!
	Find the next index (after k) for a vertex of the
	given pixel trail that is within the given threshold
	distance (in PIXELs) of the given Bvert v.

	Note: this only checks vertices of the given pixel
	trail.  Should work fine if the segments are all
	short, but if some are long it could miss the given
	vertex and report failure unnecessarily.
 */
inline int
next_match(Bvert* v, CPIXEL_list& trail, int k, double thresh)
{

   // Ensure various required conditions are true:
   if (!(v && trail.valid_index(k)))
      return -1;

   PIXEL p = v->pix();

   // Run forward over the pixel trail to the 1st point
   // within the threshold distance of v:
   int i = k+1;
   for ( ; i < trail.num() && p.dist(trail[i]) > thresh; i++)
      ;

   // If nothing found return failure status:
   if (!trail.valid_index(i))
      return -1;

   // Index i is good enough. But look a little farther for
   // something closer to v:
   int ret = i;
   double min_dist = p.dist(trail[i]);
   for (i++ ; i < trail.num() && p.dist(trail[i]) < min_dist; i++) {
      min_dist = p.dist(trail[i]);
      ret = i;
   }
   return ret;
}

//! Project Wpt_list to PIXELs and compute length:
//! If projection fails return -1.
inline double
pix_len(CWpt_list& pts)
{

   PIXEL_list pix;
   return pts.project(pix) ? pix.length() : -1;
}

//! Return length of part of a PIXEL_list from index j to
//! index k:
inline double
length(CPIXEL_list& pix, int j, int k)
{

   // XXX - requires that partial lengths have been updated.

   if (pix.valid_index(j) && pix.valid_index(k))
      return pix.partial_length(k) - pix.partial_length(j);
   return -1;
}

inline int
match_span(Bvert* v, Bedge* e, CPIXEL_list& trail, int k, double thresh)
{
   // Ensure various required conditions are true:
   if (!(v && e && e->contains(v) && trail.valid_index(k))) {
      err_adv(debug, "match_span: invalid vert/edge/index");
      return -1;
   }

   // Get the chain of vertices at the "current" mesh level:
   Bvert_list verts = get_cur_level_verts(v, e);
   assert(verts.num() > 1);

   // Ensure the vertex chain starts near the current
   // position in the pixel trail:
   if (trail[k].dist(verts.first()->wloc()) > thresh) {
      err_adv(debug, "match_span: vert chain too far from pixel trail: %f > %f",
              trail[k].dist(verts.first()->wloc()), thresh);
      return -1;
   }

   // Test that the length of the refined edge is
   // close to the length of the chosen span.
   //
   // This is a cheap way of seeing that the projected edge
   // lies reasonably along the given portion of the pixel trail.
   int ret = next_match(verts.last(), trail, k, thresh);
   if (ret < 0) {
      err_adv(debug, "match_span: can't match next vert");
      return -1;
   }
   double vlen = pix_len(verts.wpts());
   if (vlen < 0)
      return -1;
   // Measure length of the trail, including distance to
   // beginning and end of the projected edge:
   double tlen = length(trail, k, ret);
   double e1 = verts.first()->pix().dist(trail[  k]);
   double e2 = verts.last ()->pix().dist(trail[ret]);;
   err_adv(debug, "adding %3.0f, %3.0f", e1, e2);
   err_adv(debug, "edge length: %3.0f, span length: %3.0f, ratio: %1.2f: %s",
           vlen, tlen, tlen/vlen, (tlen > 1.2*vlen) ? "rejected" : "accepted");
   if (tlen > 1.2*vlen)
      return -1;
   return ret;
}

/*****************************************************************
 * SELECT_WIDGET
 *****************************************************************/
SELECT_WIDGETptr SELECT_WIDGET::_instance;

SELECT_WIDGET::SELECT_WIDGET() :
   DrawWidget(),
   _mode(SEL_NONE)
{
   // we should fix TEXT2D so you can set a relative location, e.g.
   // start or stop within a given distance of the window boundary.

   _draw_start += DrawArc(new TapGuard,      drawCB(&SELECT_WIDGET::tap_cb));
   _draw_start +=
      DrawArc(new SmallCircleGuard,drawCB(&SELECT_WIDGET::sm_circle_cb));
   _draw_start += DrawArc(new StrokeGuard,   drawCB(&SELECT_WIDGET::stroke_cb));
   _draw_start += DrawArc(new SlashGuard, drawCB(&SELECT_WIDGET::slash_cb));

   // Set up the clean up routine
   atexit(clean_on_exit);

   assert(!_instance);
   _instance = this;

   // DrawWidget constructor signs up for DISPobs notifications.
   // We use that to deselect mesh elements of any mesh that is
   // undisplayed.
}

void
SELECT_WIDGET::clean_on_exit()
{
   _instance = 0;
}

SELECT_WIDGETptr
SELECT_WIDGET::get_instance()
{
   if (!_instance)
      new SELECT_WIDGET();
   return _instance;
}

//! Returns true if the gesture is valid for beginning a
//! select_widget session.
bool
SELECT_WIDGET::init(CGESTUREptr&  gest)
{

   if (get_instance()->init_select_face(gest) ||
       get_instance()->init_select_edge(gest)) {
      return true;
   }
   return false;
}

bool 
SELECT_WIDGET::init_select_face(CGESTUREptr& g)
{
   if (!g->is_small_circle())
      return false;
   if (!try_select_face(g->center(), 0.25))
      return false;
   _mode = SEL_FACE;
   reset_timeout();
   activate();
   return true;
}

/*!
	Select an edit-level face if one is found at screen
	location 'pix' corresponding to a barycentric location
	outside of the given margin of the edges of the face.
	(I.e., pix is sufficiently near the center of the face.)
 */
bool
SELECT_WIDGET::try_select_face(CPIXEL &pix, double margin)
{

   Bface* f = find_face(pix, margin, MIN_PIX_AREA);
   if (!f)
      return false;
   err_adv(0, "  selected face: pix area %f", pix_area(f));
   WORLD::add_command(new MESH_SELECT_CMD(f));
   return true;
}

//! Same as try_select_face(), but deselects.
//! Also requires the found face is currently selected.
bool
SELECT_WIDGET::try_deselect_face(CPIXEL &pix, double margin)
{

   Bface* f = find_face(pix, margin, MIN_PIX_AREA);
   if (!(f && f->is_selected()))
      return false;
   WORLD::add_command(new MESH_DESELECT_CMD(f));
   return true;
}

//! Find the edge.
//!
//! add an select command if successful.
bool
SELECT_WIDGET::try_select_edge(CPIXEL& p)
{
   Bedge* e = find_edge(p);
   if (!e)
      return false;
   WORLD::add_command(new MESH_SELECT_CMD(e));
   return true;
}

//! Same as try_select_edge(), but deselects.
//! Also requires the found edge is currently selected.
bool
SELECT_WIDGET::try_deselect_edge(CPIXEL &pix)
{

   Bedge* e = find_edge(pix);
   if (!(e && e->is_selected()))
      return false;
   WORLD::add_command(new MESH_DESELECT_CMD(e));
   return true;
}

//! Currently checks to see that the gesture was a small circle.
//! If so, this may be an indication that the user wants to go 
//! into edge selection mode. It then executes try_select_edge() 
//! to try to find the edge. When successful, the widget enters 
//! edge selection mode.
bool 
SELECT_WIDGET::init_select_edge(CGESTUREptr& g)
{
   if (!g->is_small_circle())
      return false;
   if (!try_select_edge(g->center()))
      return false;
   _mode = SEL_EDGE;
   reset_timeout();
   activate();
   return true;
}

int
SELECT_WIDGET::tap_cb(CGESTUREptr& g, DrawState*& s)
{
   err_adv(debug, "SELECT_WIDGET::tap_cb()");

   assert(g && g->is_tap());


   if (_mode==SLASH_SEL)
   {
	   //pattern editing
		Bface* f = find_face(g->start(),0.25,MIN_PIX_AREA);
		if (f)
			if (select_list.contains(f)||select_list.contains(f->quad_partner()))
			{
	
				//get whichever part of the quad is in the selection list
				int temp = select_list.contains(f) ? select_list.get_index(f)+1 : select_list.get_index(f->quad_partner())+1 ;


				if (temp>end_face) //user selected the end face
				{
					end_face=temp;
				}
				else //user is selecting a pattern
				{
					if (pattern<temp)
						pattern=temp;

					//select/deselect face
					if (temp < MAX_PATTERN_SIZE)
					pattern_array[temp]=!pattern_array[temp];

				}
				return 1;
			}
			else
				cerr << "tap found a NULL face !" << endl;
		
		   
		return cancel_cb(g,s);


   }
   else
   {
   // Tap a selected face near the middle to deselect it:
	if (try_deselect_face(g->center(), 0.25))
		  return 1;

   // Tap edge to deselect
	if (try_deselect_edge(g->center()))
		  return 1;

   }
   // Otherwise, turn off SELECT_WIDGET
   
   return cancel_cb(g,s);
}

int
SELECT_WIDGET::cancel_cb(CGESTUREptr& g, DrawState*& s)
{
   err_adv(debug, "SELECT_WIDGET::cancel_cb");

   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;

   return 1; // This means we did use up the gesture
}

int
SELECT_WIDGET::sm_circle_cb(CGESTUREptr& g, DrawState*& s)
{
   err_adv(debug, "SELECT_WIDGET::sm_circle_cb()");

   try_select_face(g->center(), 0.25) || try_select_edge(g->center());

   return 1;
}

int
SELECT_WIDGET::stroke_cb(CGESTUREptr& g, DrawState*&)
{
   assert(g);

   select_faces(g->pts()) || select_edges(g->pts());

   return 1;
}

int 
SELECT_WIDGET:: slash_cb(CGESTUREptr& gest, DrawState*& s)
{

	
	if (_mode==SEL_FACE) //widget is in face selection mode 
	{
		select_list.clear();

		Bface* f = find_face(gest->start(),0.25,MIN_PIX_AREA);
		if (f)//f should be the currently selected face
		if (f->is_selected())
		if (f->is_quad())
		{
	
			f=f->quad_rep();

		//line in screen space coresponding to the slash
		PIXELline slash(gest->start(),gest->end());

		Bedge *e1,*e2,*e3,*e4;
		Bedge* edge = 0;
		
		//get and test the quad edges against the stroke line 
		f->get_quad_edges(e1,e2,e3,e4);

		if( e1->pix_line().intersect_segs(slash) )
		{
			edge=e1;
		}
		else
		if( e2->pix_line().intersect_segs(slash) )
		{
			edge=e2;
		}
		else
		if( e3->pix_line().intersect_segs(slash) )
		{
			edge=e3;
		}
		else
		if( e4->pix_line().intersect_segs(slash) )
		{
			edge=e4;
		}
		else
		{
			//error
			cerr << "ERROR no intersection" << endl;
			return 1;
		}
		
			//walk the geometry and select faces

			Bface* fn = f;
			do
			{			
				if (!fn->is_selected())	
					select_list +=fn;
		
				assert(edge);			//I'm paranoid too
				assert(edge->f1()!=edge->f2());

				//grabs the face on the other side of the edge
				//even if we are not directly adjacent to this edge
				fn=fn->other_quad_face(edge);
				
				if (fn) //if a valid face than advance the edge pointer
				{
					assert(edge!=fn->opposite_quad_edge(edge));
					edge = fn->opposite_quad_edge(edge);
	
					fn = fn->quad_rep(); //all faces on the selection list are rep faces

				}
				else
					cerr << "No face on the other side of the edge" << endl;

			} //quit if not a valid face or not a quad
			while(fn&&(fn->is_quad())&&edge&&(fn!=f));


			_mode=SLASH_SEL;  //go into pattern editing mode
			end_face=0;		//prepare the 2nd step data
			pattern=0;
			for (int i=0; i<MAX_PATTERN_SIZE; i++)
				pattern_array[i]=1; //fill the default pattern with ones
		}
		else
			cerr << "This is not a quad" << endl;

	}
	else
		if( _mode==SLASH_SEL)//pattern editing mode
		{ //activates upon second slash motion


			//adds the entire list to the selected group
			//undo deselects the entire group

			Bface_list final_list;

			//copy the face pointers to the final list 
			//using the pattern as a repeating template
			//and stop at the end face
			for (int i = 0; i<(end_face ? (end_face) : select_list.num()) ; i++)
			{
				if (pattern ? pattern_array[(i%pattern)+1] : 1)
				final_list+=select_list[i];
			}

		
		   WORLD::add_command(new MESH_SELECT_CMD(final_list));
		
		   return cancel_cb(gest,s);

		}
		else
			cerr << "wrong mode " << endl;
	return 1;
}

void
SELECT_WIDGET::reset()
{
   err_adv(debug, "SELECT_WIDGET::reset()");
   _mode = SEL_NONE;
}

void
SELECT_WIDGET::notify(CGELptr &g, int is_displayed)
{
   err_adv(debug, "SELECT_WIDGET::notify");

   TEXBODYptr tex = TEXBODY::upcast(g);
   if (tex && !is_displayed) {

      // A TEXBODY was undisplayed, so we need to deselect any
      // mesh elements that are currently selected

      err_adv(debug, "  undisplaying a texbody");

      MeshGlobal::deselect_all(tex->meshes());
   }

   // The base class observes when this widget is displayed
   // or undisplayed, so let it do that:
   DrawWidget::notify(g, is_displayed);
}

int
SELECT_WIDGET::draw(CVIEWptr& v)
{
	if(_mode==SLASH_SEL)
	{
		//draw the pre selection list
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

	  // no face culling
		glDisable(GL_CULL_FACE);

      // no lighting
		glDisable(GL_LIGHTING);  

		
		glBegin(GL_TRIANGLES);

		
			  for (int i = 0; i<select_list.num(); i++)
	  {
		  if( (i<(MAX_PATTERN_SIZE-1) ? (pattern ? pattern_array[i+1] : 1) : 1))
		{
		
		//color the pattern blue
		  if(i< (pattern))
			GL_COL(Color::blue3, 0.7*alpha()); 

			//color the last selected face red
		  if ((end_face!=0)&&(i==(end_face-1)))
			  
		  {
			GL_COL(Color::red3, 0.7*alpha()); 
		  }

		
		//triangle 1
		glVertex3dv(select_list[i]->v1()->wloc().data());
		glVertex3dv(select_list[i]->v2()->wloc().data());
		glVertex3dv(select_list[i]->v3()->wloc().data());

		glVertex3dv(select_list[i]->quad_partner()->v1()->wloc().data());
		glVertex3dv(select_list[i]->quad_partner()->v2()->wloc().data());
		glVertex3dv(select_list[i]->quad_partner()->v3()->wloc().data());
	
		//restore the green color
		  if(i< (pattern))
				GL_COL(Color::s_green, 0.7*alpha()); 

		  if ((end_face!=0)&&(i==(end_face-1)))
		  {
			GL_COL(Color::s_green, 0.7*alpha()); 
		  }
		}
	  }

      glEnd();
	// restore matrix
	 glPopMatrix();
	
		

    
///---DRAW OUTLINES		
			
		
		GL_VIEW::init_line_smooth(10.0);

		    
     // no face culling
		glDisable(GL_CULL_FACE);

      // no lighting
		glDisable(GL_LIGHTING);           // GL_ENABLE_BIT


		GL_COL(Color::s_green, 0.7*alpha()); 
		

      //                                         
      //     (v)-----------(v)                  
      //       |            |                    
      //       |            |                    
      //       |            |                    
      //       |            |                    
      //       |            |                    
      //     (v)-----------(v)                 
      //    

	  for (int i = 0; i<select_list.num(); i++)
	  {
	
		
		//color the pattern blue
		  if(i< (pattern))
			GL_COL(Color::blue3, 0.7*alpha()); 

			//color the last selected face red
		  if ((end_face!=0)&&(i==(end_face-1)))
			  
		  {
			GL_COL(Color::red3, 0.7*alpha()); 
		  }


		  GLStripCB cb;
		  Bface_list quad(select_list[i]);
		  quad.get_boundary().draw(&cb);


		//restore the green color
		  if(i< (pattern))
				GL_COL(Color::s_green, 0.7*alpha()); 

		  if ((end_face!=0)&&(i==(end_face-1)))
		  {
			GL_COL(Color::s_green, 0.7*alpha()); 
		  }
		}
	  

	  GL_VIEW::end_line_smooth();

	}

   return 1;
}

/*!
	Given a pixel trail with current location in it
	indexed by k,

	Find the edge emanating from vertex v that, when
	refined to the current subdivision level, matches
	closely a portion of the given pixel trail starting at
	the given index k.
 */
Bedge*
match_span(Bvert* v, CPIXEL_list& trail, int& k)
{

   static double PT_THRESH =
      Config::get_var_dbl("SELECT_EDGE_CHAIN_PT_THRESH", 7);

   if (!(v && trail.valid_index(k))) {
      err_adv(debug, "match_span: bad setup");
      return 0;
   }

   
   // Find next edge within front-facing surface regions;
   // unless all edges are back-facing
   Bedge_list edges = v->get_manifold_edges().filter(StrongEdgeFilter());
   if (edges.any_satisfy(FrontFacingEdgeFilter()))
      edges = edges.filter(FrontFacingEdgeFilter());

   if (edges.empty())
      err_adv(debug, "match_span: 0 edges to check");

   for (int i=0; i<edges.num(); i++) {
      int next_k = match_span(v, edges[i], trail, k, PT_THRESH);
      if (trail.valid_index(next_k)) {
         k = next_k;
         return edges[i];
      }
   }

   k = -1;
   return 0;
}

bool
SELECT_WIDGET::select_faces(CPIXEL_list& pts)
{
   err_adv(debug, "SELECT_WIDGET::select_faces:");

   if (pts.num() < 2) {
      err_adv(debug, "  too few points: %d", pts.num());
      return false;
   }

   Bface* f = find_face(pts[0], 0.25, MIN_PIX_AREA);
   if (!(f && f->is_selected())) {
      err_adv(debug, "  bad starter face");
      return false;
   }
   
//    // XXX - Old code (to be deleted).  Selects faces individually instead of as
//    // a group:
//    for (int i=0; i<pts.num(); i++)
//       try_select_face(pts[i], 0.1);

   Bface_list flist;

   for(int i = 0; i < pts.num(); ++i){
      
      f = find_face(pts[i], 0.1, MIN_PIX_AREA);
      if (!f || f->is_selected())
         continue;
      flist += f;
      
   }
   
   if(flist.num() > 0){
      
      WORLD::add_command(new MESH_SELECT_CMD(flist));
      err_adv(debug, "  succeeded");
      return true;
      
   }
   
   err_adv(debug, "  no faces selected");
   return false;
   
}

bool
SELECT_WIDGET::select_edges(CPIXEL_list& pts)
{
   err_adv(debug, "SELECT_WIDGET::select_edges:");

   if (pts.num() < 2) {
      err_adv(debug, "  bad gesture: %d points", pts.num());
      return false;
   }

   // Find edit-level vert near start of pixel trail:
   Bvert* v = find_vert(pts[0]);
   if (!v) {
      err_adv(debug, "  can't get starter vertex");
      return false;
   }

   // 2. Extract edge sequence within tolerance of gest

   Bedge_list chain;

   int k = 0;                           // index of cur position in gesture
   Bvert* cur = v;                      // current vertex
   Bedge* e = 0;
   while ((e = match_span(cur, pts, k))) {
      if(!e->is_selected()) chain += e;
      cur = e->other_vertex(cur);
   }

   err_adv(debug, "  got %d edges", chain.num());

   // Confirm gest is sufficiently close to edge chain

   // 3. Select the edges

   WORLD::add_command(new MESH_SELECT_CMD(chain));

   return true;
}

// end of file select_widget.C
