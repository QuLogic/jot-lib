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
#include "collide.H"

#define e 2.71828183
/********************************************************
Given a velocity vector and a position, it will test all
objects found with the sps octree for collisions and
return and new velocity that doesn't run through objects
********************************************************/
CWvec
Collide::_get_move(CWpt& s, CWvec& vel)
{
   if (_land == NULL)
      return vel;

   //transform source/velocty to object space
   Wpt source = _land->inv_xform() * s;
   Wvec velocity = _land->inv_xform() * vel;

   Wpt dest = source + velocity; //destination to travel to (obj space)
   double speed = velocity.length();

   _hitFaces.clear();

   double boxsize = _size * 5;
   Wvec d = Wvec(1,1,1)*boxsize;
   _camBox = BBOX(source - d, source + d);

	_hitFaces.clear();

	//build collision list from the land
	 buildCollisionList(_RootNode);
		
	//if(_hitFaces.num() != 0)
	//	cout << "Faces Found: " << _hitFaces.num() << endl;

   //if there are no near by nodes then bring camera closer to the object
   if (_hitFaces.empty())
		{
		Wvec force = _land->bbox().center() - dest;
		return velocity+(_size * .1 * log(force.length()) * force);
		}

   ARRAY<Wvec> norms;
   ARRAY<double> weights;
   double totalWeight = 0;

	//spring forces

   //weight all near by nodes
   for (int i = 0; i < _hitFaces.num(); i++) {
      Wpt p;
      _hitFaces[i]->bc2pos(_smplPoints[i],p);
      Wvec n = _hitFaces[i]->bc2norm(_smplPoints[i]);

      //get the projected distance of the camera and the surface point
      //against the normal of the surface point
      Wvec v  = (dest - p).projected(n);
      double dist = n*v;

      //calculate the weight of given point
      weights.add(pow(e,sqr(dist)));
      totalWeight+=weights[i];

      //calculate normal
      if (dist <= _size)       //if its closer than it should be
         norms += speed * (_size - dist) * n;
      else                            //if its further than should be             
         norms += speed * (_size - dist) * -n;
   }

   //calculate combination of all weighted norms
   Wvec force = Wvec(0,0,0);
   for (int i = 0; i < _hitFaces.num(); i++)
      force += (weights[i]/totalWeight) * norms[i];

	//smooth forces so its not jerky
	double a = .1;
   _prevForce = force;
	force = ((1 - a) * (force - _prevForce)) +_pV;
   _pV = force;

/*   
   for (int i = 0; i < _hitFaces.num(); i++)
		{
		Wpt p;
		_hitFaces[i]->bc2pos(_smplPoints[i],p);
		Wvec n = _hitFaces[i]->bc2norm(_smplPoints[i]);

		Wvec v  = ((source + (velocity + force)) - p).projected(n);
		double dist = n*v;
		if(dist < _size)
			velocity = velocity + (n *(_size - dist));
		}
	*/

   return _land->xform() * (velocity + force);
}

bool
Collide::_update_scene()
{
   CGELlist objects = VIEW::peek()->active();

   //if the geom has been scaled or skewed non-uniformly we can't use it
   if (!_land->xform().is_equal_scaling_orthogonal()) {
      cout << "COLLIDE::Warning - Land model not scaled uniformely" << endl;
      return false;
   }
   //create a octree from the "land" model of the scene
   ARRAY<OctreeNode*> nodes;
   if (_land && BMESH::isa(_land->body())) {
      Bvert_list list;
      Bface_list fs;
      ARRAY<Wvec> bcs;
      double spacing=0;
      nodes += sps(BMESH::upcast(_land->body()),
                   _height, _regularity, _min_dist, fs, bcs, spacing);
      _RootNode = nodes[0];
      _objs += 1;
   }

   //_spsTree = &_RootNodes;
   return true;
}


double
Collide::intersect(
   CWpt& pOrigin,
   CWvec& pNormal,
   CWpt& rOrigin,
   CWvec& rVector)
{
   double d = -(Wvec(pOrigin) * pNormal);
   return -((pNormal * Wvec(rOrigin) + d) / (pNormal * rVector));
}

double
Collide::intersectSphere(CWpt& rO, CWvec& rV, CWpt& sO, double sR)
{
   Wvec Q = sO - rO;
   double c = Q.length();
   double v = Q * rV;
   double d = sR*sR - (c*c - v*v);

   // If there was no intersection, return -1

   if (d < 0.0) return -1.0;

   // Return the distance to the [first] intersecting point

   return v - sqrt(d);
}

bool
Collide::buildCollisionList(OctreeNode* node)
{
   //if the bounding box doesn't intersect the node
   //then ignore it and its children
   if (node->get_leaf() != 1 && node->get_leaf() != 0)
      return false;

   //if the node doesn't overlap then no need to add children
   if (!node->overlaps(_camBox))
      return false;

   //if the node has no children(its a leaf), add its triangles
   if (node->get_leaf()) {
      _hitFaces += node->get_face();
      _smplPoints.add(node->get_point());
   } else { //else, check its children
      OctreeNode** children =  node->get_children();
      for (int i = 0; i<8; i++)
         buildCollisionList(children[i]);
   }
   return true;
}

bool
Collide::getNearPoints(OctreeNode* node)
{
   //if the node doesn't overlap then no need to add children
   if (!node->overlaps(_camBox))
      return false;

   //if the node has no children(its a leaf), add its triangles
   if (node->get_leaf()) {
      //Wpt p = node->centroid();
      _hitFaces = _hitFaces + node->intersects();
   } else { //else, check its children
      OctreeNode** children =  node->get_children();
      for (int i = 0; i<8; i++)
         buildCollisionList(children[i]);
   }
   return true;
}

/*
  void Collide::set_size(double s)
  {
  _size = s;

  _BSphere = new BMESH;
  _BSphere->Sphere(_BSphere->new_patch());
  _BSphere->transform(Wtransf::scaling(_size,_size,_size));


  _DestSphere = new BMESH;
  _DestSphere->Sphere(_DestSphere->new_patch());
  _DestSphere->transform(Wtransf::scaling(_size,_size,_size));
  }
*/

////////////////////////////////
//Gravity
//
////////////////////////////



/////////////////////
//get_dir()
//given position, find the current gravity for that point
/////////////////////
CWvec
Gravity::_get_dir(CWpt& p)
{

//If Gravity hasn't been set
   if (_ground == NULL)
      return Wvec(0,0,0);
   Wpt cent = _ground->bbox().center();
   Wpt max  = _ground->bbox().max();

   /*
     if(_currentGrav == WORLD)
     return _strength * _world_dir;

     Wpt cent = _ground->bbox().center();
     Wpt max  = _ground->bbox().max();

     if(_currentGrav == GLOBE)
     return _strength * (cent-p).normalized();

     if(_currentGrav == LANDSCAPE) */
   //^ commented out because only landscape works/is useful atm
   return _strength * (Wvec(0,cent[1],0) - Wvec(0,max[1],0)).normalized();

   return Wvec(0,0,0);
}


bool
Gravity::set_grav(GEOMptr g, mlib::CWvec& d, int t)
{
   _ground         = g;
   _world_dir      = d;
   _currentGrav    = t;
   return true;
}


void
Gravity::set_globe(GEOMptr g)
{
   _ground         = g;
   _globe          = true;
   _landscape      = false;
   _world          = false;

}
void
Gravity::set_landscape(GEOMptr g)
{
   _ground         = g;
   _globe          = false;
   _landscape      = true;
   _world          = false;
}
void Gravity::set_world(Wvec d)
{
   _world_dir      = d;
   _globe          = false;
   _landscape      = false;
   _world          = true;
}

/* end of file collide.C */
