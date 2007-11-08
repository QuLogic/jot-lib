// halftone-dots.fp
//
//   does halftoning based on dots laid out on a regular grid

uniform vec2 origin;
uniform vec2 u_vec;
uniform vec2 v_vec;

uniform int style; // style ID
uniform float st;  // interpolation parameter from large to small dots

// tone image uniforms
uniform sampler2D tone_map;
uniform int width;
uniform int height;

float get_tone(vec2 pix, int channel)
{
   // get tone at given pixel location via tone reference image
   return texture2D(tone_map, pix/vec2(width,height))[channel];
}

float f(float x)
{
   const float two_pi = 2.0*3.1415927;
   const float a = 0.4;
   return 0.5 + a * cos(x*two_pi);
}

vec2 pix_to_uv(vec2 p)
{
   // p is a vector relative to the origin
   // return its components relative to u and v vectors

   return vec2(dot(p,u_vec)/dot(u_vec,u_vec),
               dot(p,v_vec)/dot(v_vec,v_vec));
}

float get_lod_f(float val, float lod)
{
   return mix(f(val),f(val*2.0),lod);
}

float
halftone(vec2 u, float tone, float lod)
{
   // "height" value in halftone screen:
   float h = (get_lod_f(u.x,lod) + get_lod_f(u.y,lod))/2.0;

   const float e = 0.1;
   return smoothstep(-e, e, h - tone);
}

vec3
color_ub(int r, int g, int b)
{
   return vec3(float(r)/255.0,
               float(g)/255.0,
               float(b)/255.0);
}

struct layer_style
{
   bool  is_enabled;
   bool  is_highlight;
   vec3  ink_color;
};

struct dots_style
{
   float dot_size;
   layer_style layer0;
   layer_style layer1;
};

void add_layer_contribution(
   vec2 u,
   layer_style layer,
   int channel,
   inout vec3 col
   )
{
   if (layer.is_enabled) {
      float tone = get_tone(gl_FragCoord.xy, channel);
      if (layer.is_highlight)
         tone = 1.0 - tone;
      col = mix(col, layer.ink_color, halftone(u, tone, st));
   }
}

void main()
{
   dots_style my_style;
   if (style == 0) {
      my_style = dots_style(
         12.0,
         layer_style(true, false, color_ub( 49,  48,  18)),
         layer_style(true, true , color_ub(252, 235, 209))
         );
   } else if (style == 1) {
      my_style = dots_style(
         13.0,
         layer_style(true,  false, color_ub(100, 117, 135)),
         layer_style(false, false, color_ub(100, 117, 135))
         );
   } else if (style == 2) {
      my_style = dots_style(
         15.0,
         layer_style(true,  false, color_ub(93,  50,  23)),
         layer_style(false, false, color_ub(93,  50,  23))
         );
   } else {
      my_style = dots_style(
         12.0,
         layer_style(true,  false, color_ub(35, 45, 50)),
         layer_style(false, false, color_ub(35, 45, 50))
         );
   }

   // uv coords with scaling for dot size:
   vec2 u = pix_to_uv(gl_FragCoord.xy - origin) / my_style.dot_size;

   vec3 col = gl_Color.rgb;
   
   // add layers:
   add_layer_contribution(u, my_style.layer0, 0, col);
   add_layer_contribution(u, my_style.layer1, 1, col);

   gl_FragColor = vec4(col, 1.0);
}

// halftone-dots.fp
