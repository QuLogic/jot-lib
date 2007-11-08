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
#ifndef COLLIDE_H_IS_INCLUDED
#define COLLIDE_H_IS_INCLUDED

#include "sps/sps.H"
#include "disp/view.H"
#include "disp/gel.H"
#include "disp/base_collide.H"
#include "std/stop_watch.H"

#define GRAVITY_TYPES = 3;

/*****************************************************************
 * Collide:
 *
 *      Animates a camera to Cruise around a object
 *****************************************************************/

class Collide : public BaseCollide {
 public :
   virtual ~Collide() {}
                         
   Collide(double s,int h,double r,double m) : BaseCollide() {
      assert(_instance == 0);
      _instance = this;
      _size = s;
      _height = h;
      _regularity = r;
      _min_dist = m; 
      _objs = 0;
	  _acc = .00001;
	  _V = Wvec(0,0,0);
	  _pV = Wvec(0,0,0);
	  _prevForce = Wvec(0,0,0);

   }


//static ARRAY<OctreeNode*>* _spsTree;
   //static ARRAY<OctreeNode*>* get_sps() {return _spsTree; }
   static mlib::CWvec get_move(mlib::CWpt& p, mlib::CWvec& v); 
   bool set_land(GEOMptr g) {_land = g;
							_update_scene();
							return true;} 
  //static ARRAY<OctreeNode*>* get_scene() {return &_RootNodes;}
  
   static void set_size(double); 


   bool buildCollisionList(OctreeNode*);
   bool getNearPoints(OctreeNode*);
   double intersect(mlib::CWpt&, mlib::CWvec&, mlib::CWpt&, mlib::CWvec&);
   double intersectSphere(mlib::CWpt& rO, mlib::CWvec& rV, mlib::CWpt& sO, double sR);

 protected:

	
	//static ARRAY<OctreeNode*>* _spsTree;


   stop_watch   _clock;
   Bface_list   _hitFaces;                              //list of potential colliders
   ARRAY<Wvec>	_smplPoints;							//list of sample points
   GEOMptr		_land;
   BMESHptr             _BSphere;                               //bounding sphere around the camera
   BMESHptr     _DestSphere;                    //bounding sphere around the destination point
   double               _size;                                  //size of the camera's bounding sphere
   int                  _height;                                
   double               _regularity;
   double               _min_dist;
   double				_acc;
   Wvec					_V;
   Wvec					_pV;
   Wvec					_prevForce;
   int                  _objs;
   BBOX         _camBox;
   BBOX         _polyBox;
   OctreeNode* _RootNode;

   virtual mlib::CWvec _get_move(mlib::CWpt& p, mlib::CWvec& v);
   virtual bool _update_scene(); 
};


/***a**************************************************************
 * Gravity:
 *
 *      Creates a artificial gravity vector to "ground" the
 *              camera on a surface
 *****************************************************************/

class Gravity : public BaseGravity {
 public :
                         
   Gravity() : BaseGravity() {
      assert(_instance == 0);
      _instance = this;

      _currentGrav = 0;
      _ground = NULL;
      _strength = .05;
      _globe = false;
      _landscape = false;
      _world = false;
   }         
//CAMptr &p, 
   virtual ~Gravity() {}
    

	static mlib::Wvec	get_dir(mlib::CWpt& p);
	
	//static bool			set_grav(GEOMptr g, mlib::CWvec& d, int t)
	//	{
	//	return _inst ? _inst->_set_grav(g,d,t) : false;
	//	}

	void toggle_type()	{
		if(_currentGrav == 2)
			_currentGrav = 0;
		else
		  _currentGrav++;
		}


    bool set_grav(GEOMptr g, mlib::CWvec& d, int t);
	void set_globe(GEOMptr);
	void set_landscape(GEOMptr);
	void set_world(mlib::Wvec);



 protected:


   virtual mlib::CWvec      _get_dir(mlib::CWpt& p);

   enum _gtypes {
      GLOBE = 0,
      LANDSCAPE,
      WORLD
   };

   GEOMptr      _ground;                //object the camera will "walk" on
   mlib::Wvec   _world_dir;         //direction of world gravity            
   double       _strength;              //strength of the gravity
   int          _currentGrav;   //
   bool         _globe;                 
   bool         _landscape;
   bool         _world;
};

#endif // COLLIDE_H_IS_INCLUDED

/* end of file collide.H */
