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
 * along with jot-lib.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************/

#ifndef GLUI_CUSTOM_CONTROLS_H
#define GLUI_CUSTOM_CONTROLS_H

#define GLUI_SLIDER_WIDTH				100
#define GLUI_SLIDER_HEIGHT				80
#define GLUI_SLIDER_LO					0
#define GLUI_SLIDER_HI					100

#define GLUI_BITMAPBOX_WIDTH			120
#define GLUI_BITMAPBOX_HEIGHT			80

#define GLUI_GRAPH_WIDTH			   250
#define GLUI_GRAPH_HEIGHT			   80

#define GLUI_ACTIVETEXT_SIZE        20

/********* Pre-declare classes as needed *********/
class GLUI_Slider;
class GLUI_BitmapBox;
class GLUI_Graph;
class GLUI_ActiveText;


#define GLUI_SLIDER_INT			1
#define GLUI_SLIDER_FLOAT		2

#define GLUI_BITMAPBOX_EVENT_NONE				0
#define GLUI_BITMAPBOX_EVENT_MOUSE_DOWN		1
#define GLUI_BITMAPBOX_EVENT_MOUSE_MOVE		2
#define GLUI_BITMAPBOX_EVENT_MOUSE_UP			3
#define GLUI_BITMAPBOX_EVENT_KEY					4
#define GLUI_BITMAPBOX_EVENT_MIDDLE_DOWN		5
#define GLUI_BITMAPBOX_EVENT_MIDDLE_MOVE		6
#define GLUI_BITMAPBOX_EVENT_MIDDLE_UP			7
#define GLUI_BITMAPBOX_EVENT_RIGHT_DOWN		8
#define GLUI_BITMAPBOX_EVENT_RIGHT_MOVE		9
#define GLUI_BITMAPBOX_EVENT_RIGHT_UP			10


#define GLUI_GRAPH_EVENT_NONE				   0
#define GLUI_GRAPH_EVENT_MOUSE_DOWN		   1
#define GLUI_GRAPH_EVENT_MOUSE_MOVE		   2
#define GLUI_GRAPH_EVENT_MOUSE_UP			3
#define GLUI_GRAPH_EVENT_KEY					4

#define GLUI_GRAPH_SERIES_DOT					0
#define GLUI_GRAPH_SERIES_LINE				1

#define GLUI_GRAPH_SERIES_NAME_LENGTH     64

#define GLUI_ACTIVETEXT_OPTION_X          10

#define GLUI_COLORBUTTON_R          0
#define GLUI_COLORBUTTON_G          0
#define GLUI_COLORBUTTON_B          1

#if 0
class GLUI_Main {
  friend class GLUI_Slider;
  friend class GLUI_BitmapBox;
  friend class GLUI_Graph;
}

class GLUI {
  GLUI_Slider *add_slider(
     const char *name,
     int data_type=GLUI_SLIDER_INT,
     float limit_lo = GLUI_SLIDER_LO,
     float limit_hi = GLUI_SLIDER_HI,
     void *data=NULL,
     int id=-1,
     GLUI_Update_CB callback=NULL
     );

  GLUI_Slider *add_slider_to_panel( GLUI_Panel *panel, const char *name,
					int data_type=GLUI_SLIDER_INT,
					float limit_lo = GLUI_SLIDER_LO, float limit_hi = GLUI_SLIDER_HI,
					void *data=NULL,
					int id=-1, GLUI_Update_CB callback=NULL);

  GLUI_BitmapBox *add_bitmapbox( const char *name,
					int id=-1, GLUI_Update_CB callback=NULL, bool active=true);
  GLUI_BitmapBox *add_bitmapbox_to_panel( GLUI_Panel *panel, const char *name,
					int id=-1, GLUI_Update_CB callback=NULL, bool active=true);

  GLUI_Graph *add_graph( const char *name,
					int id=-1, GLUI_Update_CB callback=NULL);
  GLUI_Graph *add_graph_to_panel( GLUI_Panel *panel, const char *name,
					int id=-1, GLUI_Update_CB callback=NULL);

  GLUI_ActiveText  *add_activetext( const char *name, int id=-1,
			    GLUI_Update_CB callback=NULL);
  GLUI_ActiveText  *add_activetext_to_panel( GLUI_Panel *panel, const char *name,
				     int id=-1, GLUI_Update_CB callback=NULL );

}
#endif

/************************************************************/
/*                                                          */
/*                   Slider class                           */
/*                                                          */
/************************************************************/

class GLUI_Slider : public GLUI_Control
{
public:
  int	pressed;
  int		data_type;
  float	float_low, float_high;
  int		int_low, int_high;
  int		graduations;

  int  mouse_down_handler( int local_x, int local_y );
  int  mouse_up_handler( int local_x, int local_y, int inside );
  int  mouse_held_down_handler( int local_x, int local_y, int inside );
  int  special_handler( int key, int modifiers );

  void draw( int x, int y );
  void draw_active_area( void );
  void draw_translated_active_area( void );

  void draw_val( void );
  void draw_slider( void );
  void draw_knob( int x, int off_l, int off_r, int off_t, int off_b,
							unsigned char r, unsigned char g, unsigned char b );
  void update_val(int x, int y);

  virtual void   set_int_val( int new_int )
  { CLAMP(new_int,   int_low,   int_high);   int_val =   new_int;
   output_live(true); if (can_draw()) draw_translated_active_area();};
  virtual void   set_float_val( float new_float )
  { CLAMP(new_float, float_low, float_high); float_val = new_float;
   output_live(true); if (can_draw()) draw_translated_active_area();};

  void set_float_limits( float low,float high);
  void set_int_limits( int low, int high);
  void set_num_graduations( int grads );

  void update_size( void );

  /*  int mouse_over( int state, int x, int y ); */

  GLUI_Slider(void);
};


/************************************************************/
/*                                                          */
/*               ActiveText class                           */
/*                                                          */
/************************************************************/

class GLUI_ActiveText : public GLUI_Control
{
public:
  int  currently_pressed;
  int  currently_inside;
  int  currently_highlighted;

  int  mouse_down_handler( int local_x, int local_y );
  int  mouse_up_handler( int local_x, int local_y, int same );
  int  mouse_held_down_handler( int local_x, int local_y, int inside );
  int  key_handler( unsigned char key,int modifiers );

  void draw( int x, int y );
  void draw_active_area( void );
  void draw_translated_active_area( void );

  void set_text( const char *text );

  int  get_highlighted()      { return currently_highlighted; }
  void set_highlighted(int h)
  {
     int oh;
     oh = currently_highlighted;
     currently_highlighted = h;
     if (h!=oh) draw_translated_active_area();
  }

  void update_size( void );
  int check_fit(const char *s);

  GLUI_ActiveText( void ) {
    glui_format_str( name, "ActiveText: %p", this );
    h            = GLUI_ACTIVETEXT_SIZE;
    w            = 100;
    alignment    = GLUI_ALIGN_LEFT;
    can_activate = true;
    currently_pressed = false;
    currently_inside  = false;
    currently_highlighted = false;
  }

};

/************************************************************/
/*                                                          */
/*                   BitmapBox class                        */
/*                                                          */
/************************************************************/

class GLUI_BitmapBox : public GLUI_Control
{
public:
  int	            currently_pressed;
  int             currently_inside;

  unsigned char *	image;
  unsigned char *	image_disabled;
  int					image_w;
  int					image_h;
  int             border;
  int             margin;
  int             depressable;


  int		event;
  int		event_key;
  int		event_mod;
  int		event_in;
  int		event_x;
  int		event_y;

  int		get_event( void )			{ return event; }
  int		get_event_key( void )		{ return event_key; }
  int		get_event_mod( void )		{ return event_mod; }
  int		get_event_in( void )		{ return event_in; }
  int		get_event_x( void )		{ return event_x; }
  int		get_event_y( void )		{ return event_y; }

  int		get_image_w( void )		{ return image_w; }
  int		get_image_h( void )		{ return image_h; }

  void	copy_img(unsigned char *data,int w, int h, int bpp);
  void	set_img_size(int w, int h);

  int		get_border( void )		{ return border; }
  void	set_border( int b )		{ border = b; update_size(); if ( glui ) glui->refresh();}

  int		get_margin( void )		{ return margin; }
  void	set_margin( int m )		{ margin = m; update_size(); if ( glui ) glui->refresh();}

  int		get_depressable( void )	   { return depressable;   }
  void	set_depressable( int d )	{ depressable = d;      }

  int		general_mouse_down_handler( int but, int local_x, int local_y );
  int		general_mouse_up_handler( int but,  int local_x, int local_y, int inside );
  int		general_mouse_held_down_handler( int but,  int local_x, int local_y, int inside );

  int		mouse_down_handler( int local_x, int local_y );
  int		mouse_up_handler( int local_x, int local_y, int inside );
  int		mouse_held_down_handler( int local_x, int local_y, int inside );

  int		special_handler( int key, int modifiers );

  void	draw( int x, int y );
  void	draw_active_area( void );
  void	draw_translated_active_area( int front );

  void	update_size( void );

  void	execute_callback( void );

  GLUI_BitmapBox(void);
  virtual ~GLUI_BitmapBox(void);
};

/************************************************************/
/*                                                          */
/*                   Graph class                            */
/*                                                          */
/************************************************************/

class GLUI_GraphSeries
{
 public:
   char        name[64];
   int         type;
   double      size;
   double      color[4];
   int         data_num;
   double *    data_x;
   double *    data_y;

   GLUI_GraphSeries(void)
   {
      name[0]     = 0;
      type        = GLUI_GRAPH_SERIES_DOT;
      size        = 1.5;
      color[0]    = 0.0; color[1] = 0.0; color[2] = 0.0; color[3] = 1.0;
      data_num    = 0;
      data_x      = NULL;      data_y      = NULL;
   }

   ~GLUI_GraphSeries(void)  {  clear_data();  }

   void  clear_data(void)
   {
      if (data_num != 0)
      {
         delete[] data_x; delete[] data_y;
         data_num = 0; data_x = NULL; data_y = NULL;
      }
   }

   void  set_name(const char *n)
   {
      int i = 0;
      for ( ; i<GLUI_GRAPH_SERIES_NAME_LENGTH-1; i++)
         if (n[i] != 0)
            name[i] = n[i];
         else
            break;
      name[i] = 0;
   }

   void  set_color(const double *c) { for (int i=0; i<4; i++) color[i]=c[i]; }

   void  set_data (int n, const double *x, const double *y)
   {
      if (n > 0)
      {
         if (data_num != n)
         {
                  clear_data(); data_num = n;
                  data_x = new double[n]; data_y = new double[n];
                  assert(data_y); assert(data_x);
         }
         for (int i=0; i<data_num; i++) { data_x[i] = x[i]; data_y[i] = y[i]; }
      }
   }
};

#define VALID_I(A)   { assert((A)>=0); assert((A)<graph_data_num); }

class GLUI_Graph : public GLUI_Control
{
public:

   GLUI_GraphSeries*	graph_data;

   int   graph_data_num;

   double   background[4];

   double   min_x;
   double   min_y;
   double   max_x;
   double   max_y;

   int   pressed;

   int   graph_w;
   int   graph_h;

   int   event;
   int   event_key;
   int   event_mod;
   int   event_x;
   int   event_y;

   int   get_event( void )          { return event; }
   int   get_event_key( void )      { return event_key; }
   int   get_event_mod( void )      { return event_mod; }
   int   get_event_x( void )        { return event_x; }
   int   get_event_y( void )        { return event_y; }



   void  set_graph_size(int w, int h);

   int   get_graph_w( void )        { return graph_w; }
   int   get_graph_h( void )        { return graph_h; }



   int            get_num_series( void )  { return graph_data_num; }
   void           set_num_series(int n);


   int            get_series_num (int i)  { VALID_I(i); return graph_data[i].data_num; }
   const char *   get_series_name(int i)  { VALID_I(i); return graph_data[i].name; }
   int            get_series_type(int i)  { VALID_I(i); return graph_data[i].type; }
   double         get_series_size(int i)  { VALID_I(i); return graph_data[i].size; }
   const double * get_series_color(int i) { VALID_I(i); return graph_data[i].color; }
   const double * get_series_data_x(int i){ VALID_I(i); return graph_data[i].data_x; }
   const double * get_series_data_y(int i){ VALID_I(i); return graph_data[i].data_y; }

   void           set_series_name(int i, const char *n);
   void           set_series_type(int i, int t);
   void           set_series_size(int i, double s);
   void           set_series_color(int i, const double *c);
   void           set_series_data(int i, int n, const double *x, const double *y);

   void           set_background(const double *c) { for (int i=0; i<4; i++) background[i]=c[i]; }
   const double * get_background() { return background; }

   void           set_min_x(bool autom, double m=0.0);
   void           set_min_y(bool autom, double m=0.0);
   void           set_max_x(bool autom, double m=0.0);
   void           set_max_y(bool autom, double m=0.0);

   double         get_min_x()    { return min_x; }
   double         get_min_y()    { return min_y; }
   double         get_max_x()    { return max_x; }
   double         get_max_y()    { return max_y; }


   void  clear_series( void );

   int   mouse_down_handler( int local_x, int local_y );
   int   mouse_up_handler( int local_x, int local_y, int inside );
   int   mouse_held_down_handler( int local_x, int local_y, int inside );
   int   special_handler( int key, int modifiers );

   void  draw( int x, int y );
   void  draw_active_area( void );
   void  draw_translated_active_area( int front );

   void  update_size( void );

   void  execute_callback( void );

   void  redraw();

   GLUI_Graph(void);
   virtual ~GLUI_Graph(void);

};

#endif // GLUI_CUSTOM_CONTROLS_H

