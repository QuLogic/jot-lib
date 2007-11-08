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
#ifndef _HATCHING_COLLECTION_H_IS_INCLUDED_
#define _HATCHING_COLLECTION_H_IS_INCLUDED_

////////////////////////////////////////////
// HatchingCollection
////////////////////////////////////////////
//
// -Holds a collection of hatching groups
// -HatchingPen talks to this object
// -Is held by the texture
//
////////////////////////////////////////////

#include "npr/hatching_group.H"
#include "manip/manip.H"


/*****************************************************************
 * HatchingCollection
 *****************************************************************/

#define CHatchingCollection const HatchingCollection

class HatchingCollection : 
	private ARRAY<HatchingGroup *>, public UPobs, public DATA_ITEM, protected BMESHobs {
 private:
	/******** STATIC MEMBER VARIABLES ********/
	static TAGlist		*_hc_tags;

	/******** MEMBERS VARIABLES ********/
	Patch *				_patch;
   bool              _xform_flag;

 public:
	 /******** CONSTRUCTOR/DECONSTRUCTOR *******/

	HatchingCollection(Patch *p=0);
	~HatchingCollection();

	/******** MEMBERS METHODS ********/


   /******** HATCHING GROUP MANAGEMENT ********/
        bool add_group(HatchingGroup *g);
        void remove_group(HatchingGroup *g);
      
	HatchingGroup *	add_group(int t);
	HatchingGroup *	next_group(mlib::CNDCpt &pt, HatchingGroup *hg);
	bool					delete_group(HatchingGroup *hg);

   int					draw(CVIEWptr& v);

	void					set_patch(Patch *p);

	Patch *				patch() { return _patch; }	

   /******** BMESHobs METHODS *******/
   virtual void    notify_change       (BMESH*, BMESH::change_t);
   virtual void    notify_xform        (BMESH*, mlib::CWtransf&, CMOD&);
   //virtual void  notify_merge        (BMESH*, BMESH*)           {}
   //virtual void  notify_split        (BMESH*, CARRAY<BMESH*>&)  {}
   //virtual void  notify_subdiv_gen   (BMESH*)                   {}
   //virtual void  notify_delete       (BMESH*)                   {}
   //virtual void  notify_sub_delete   (BMESH*)                   {}

   /******** UPobs METHODS ********/
	virtual void		reset(int is_reset);

	/******** DATA_ITEM METHODS ********/
	DEFINE_RTTI_METHODS_BASE("HatchingCollection", CHatchingCollection*);
   virtual DATA_ITEM	*dup() const { return new HatchingCollection; }
   virtual CTAGlist	&tags() const;

   /******** I/O functions ********/
   virtual void		get_hatching_group (TAGformat &d);
   virtual void		put_hatching_groups (TAGformat &d) const;
	
};

#endif
