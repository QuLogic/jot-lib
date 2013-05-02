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

/************************************************************/
/*                                                          */
/*                   Slider class                           */
/*                                                          */
/************************************************************/

class GLUI_Slider : public GLUI_Control
{
public:
  bool pressed;
  int		data_type;
  float	float_low, float_high;
  int		int_low, int_high;
  int		graduations;

  int  mouse_down_handler( int local_x, int local_y );
  int  mouse_up_handler( int local_x, int local_y, bool inside );
  int  mouse_held_down_handler( int local_x, int local_y, bool inside );
  int  special_handler( int key, int modifiers );

  void draw( int x, int y );
  void draw_active_area( void );
  void draw_translated_active_area( void );

  void draw_val( void );
  void draw_slider( void );
  void draw_knob( int x, int off_l, int off_r, int off_t, int off_b,
                  unsigned char r, unsigned char g, unsigned char b );
  void update_val(int x, int y);

  virtual void   set_int_val( int new_int );
  virtual void   set_float_val( float new_float );

  void set_float_limits( float low,float high);
  void set_int_limits( int low, int high);
  void set_num_graduations( int grads );

  void update_size( void );

  /*  int mouse_over( int state, int x, int y ); */

  GLUI_Slider(GLUI_Node *parent, const char *name,
              int id=-1, GLUI_CB callback=GLUI_CB(),
              int data_type=GLUI_SLIDER_INT,
              float limit_lo=GLUI_SLIDER_LO,
              float limit_hi=GLUI_SLIDER_HI,
              void *data=NULL);
  GLUI_Slider(void) { common_init(); }

protected:
  void common_init(void) {
      glui_format_str( name, "Slider: %p", this );

      w            = GLUI_SLIDER_WIDTH;
      h            = GLUI_SLIDER_HEIGHT;
      can_activate = true;
      live_type    = GLUI_LIVE_NONE;
      alignment    = GLUI_ALIGN_CENTER;
      int_low      = GLUI_SLIDER_LO;
      int_high     = GLUI_SLIDER_HI;
      float_low    = GLUI_SLIDER_LO;
      float_high   = GLUI_SLIDER_HI;

      pressed      = false;

      graduations  = 0;
  }
};


/************************************************************/
/*                                                          */
/*               ActiveText class                           */
/*                                                          */
/************************************************************/

class GLUI_ActiveText : public GLUI_Control
{
public:
  bool currently_pressed;
  bool currently_inside;
  bool currently_highlighted;

  int  mouse_down_handler( int local_x, int local_y );
  int  mouse_up_handler( int local_x, int local_y, bool same );
  int  mouse_held_down_handler( int local_x, int local_y, bool inside );
  int  key_handler( unsigned char key,int modifiers );

  void draw( int x, int y );
  void draw_active_area( void );
  void draw_translated_active_area( void );

  void set_text( const char *text );

  bool get_highlighted();
  void set_highlighted(bool h);

  void update_size( void );
  int check_fit(const char *s);

  GLUI_ActiveText(GLUI_Node *parent, const char *name,
                  int id=-1, GLUI_CB callback=GLUI_CB());
  GLUI_ActiveText( void ) { common_init(); }

protected:
  void common_init(void) {
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
  bool            currently_pressed;
  bool            currently_inside;

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

  int		get_event( void );
  int		get_event_key( void );
  int		get_event_mod( void );
  int		get_event_in( void );
  int		get_event_x( void );
  int		get_event_y( void );

  int		get_image_w( void );
  int		get_image_h( void );

  void	copy_img(unsigned char *data,int w, int h, int bpp);
  void	set_img_size(int w, int h);

  int		get_border( void );
  void	set_border( int b );

  int		get_margin( void );
  void	set_margin( int m );

  int		get_depressable( void );
  void	set_depressable( int d );

  int		general_mouse_down_handler( int but, int local_x, int local_y );
  int		general_mouse_up_handler( int but,  int local_x, int local_y, bool inside );
  int		general_mouse_held_down_handler( int but,  int local_x, int local_y, bool inside );

  int		mouse_down_handler( int local_x, int local_y );
  int		mouse_up_handler( int local_x, int local_y, bool inside );
  int		mouse_held_down_handler( int local_x, int local_y, bool inside );

  int		special_handler( int key, int modifiers );

  void	draw( int x, int y );
  void	draw_active_area( void );
  void	draw_translated_active_area( int front );

  void	update_size( void );

  void	execute_callback( void );

  GLUI_BitmapBox(GLUI_Node *parent, const char *name,
                 int id=-1, GLUI_CB callback=GLUI_CB(), bool active=true);
  GLUI_BitmapBox(void) { common_init(); }
  virtual ~GLUI_BitmapBox(void);

protected:
  void common_init(void) {
      glui_format_str( name, "BitmapBox: %p", this );

      w                  = GLUI_BITMAPBOX_WIDTH;
      h                  = GLUI_BITMAPBOX_HEIGHT;
      can_activate       = true;
      live_type          = GLUI_LIVE_NONE;
      alignment          = GLUI_ALIGN_CENTER;

      currently_pressed  = false;
      currently_inside   = false;

      event              = GLUI_BITMAPBOX_EVENT_NONE;
      event_key          = -1;
      event_mod          = -1;
      event_in           = -1;
      event_x            = -1;
      event_y            = -1;

      image              = NULL;
      image_disabled     = NULL;
      image_w            = 0;
      image_h            = 0;

      border             = 1;
      margin             = 2;

      depressable        = false;
  }
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

   GLUI_GraphSeries(void);

   ~GLUI_GraphSeries(void);

   void  clear_data(void);

   void  set_name(const char *n);

   void  set_color(const double *c);

   void  set_data (int n, const double *x, const double *y);
};

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

   bool  pressed;

   int   graph_w;
   int   graph_h;

   int   event;
   int   event_key;
   int   event_mod;
   int   event_x;
   int   event_y;

   int   get_event( void );
   int   get_event_key( void );
   int   get_event_mod( void );
   int   get_event_x( void );
   int   get_event_y( void );

   void  set_graph_size(int w, int h);

   int   get_graph_w( void );
   int   get_graph_h( void );

   int            get_num_series( void );
   void           set_num_series(int n);

   int            get_series_num (int i);
   const char *   get_series_name(int i);
   int            get_series_type(int i);
   double         get_series_size(int i);
   const double * get_series_color(int i);
   const double * get_series_data_x(int i);
   const double * get_series_data_y(int i);

   void           set_series_name(int i, const char *n);
   void           set_series_type(int i, int t);
   void           set_series_size(int i, double s);
   void           set_series_color(int i, const double *c);
   void           set_series_data(int i, int n, const double *x, const double *y);

   void           set_background(const double *c);
   const double * get_background();

   void           set_min_x(bool autom, double m=0.0);
   void           set_min_y(bool autom, double m=0.0);
   void           set_max_x(bool autom, double m=0.0);
   void           set_max_y(bool autom, double m=0.0);

   double         get_min_x();
   double         get_min_y();
   double         get_max_x();
   double         get_max_y();

   void  clear_series( void );

   int   mouse_down_handler( int local_x, int local_y );
   int   mouse_up_handler( int local_x, int local_y, bool inside );
   int   mouse_held_down_handler( int local_x, int local_y, bool inside );
   int   special_handler( int key, int modifiers );

   void  draw( int x, int y );
   void  draw_active_area( void );
   void  draw_translated_active_area( int front );

   void  update_size( void );

   void  execute_callback( void );

   void  redraw();

   GLUI_Graph(GLUI_Node *parent, const char *name,
              int id=-1, GLUI_CB callback=GLUI_CB());
   GLUI_Graph(void) { common_init(); }
   virtual ~GLUI_Graph(void);

protected:
  void common_init(void) {
      glui_format_str( name, "Graph: %p", this );

      w              = GLUI_GRAPH_WIDTH;
      h              = GLUI_GRAPH_HEIGHT;
      can_activate   = true;
      live_type      = GLUI_LIVE_NONE;
      alignment      = GLUI_ALIGN_CENTER;

      pressed        = false;

      event          = GLUI_GRAPH_EVENT_NONE;
      event_key      = -1;
      event_mod      = -1;
      event_x        = -1;
      event_y        = -1;

      graph_data     = NULL;
      graph_data_num = 0;

      graph_w        = GLUI_GRAPH_WIDTH;
      graph_h        = GLUI_GRAPH_HEIGHT;

      min_x          = 0.0;
      min_y          = 0.0;
      max_x          = 1.0;
      max_y          = 1.0;

      background[0]  = 0.8;
      background[1]  = 0.8;
      background[2]  = 0.8;
      background[3]  = 1.0;
  }
};

#endif // GLUI_CUSTOM_CONTROLS_H

