// dots_tx.fp
//
//   does halftoning using procedural or texture-based
//   halftone screen

varying   float opacity;       // interpolated per-vertex opacity
uniform   float op_remap;

// dynamic 2D patterns stuff:
uniform vec2 origin;
uniform vec2 u_vec;
uniform vec2 v_vec;

uniform float st;       // interpolation parameter from large to small dots
uniform float starting_timed_lod_scale;
uniform float target_scale;
uniform float timed_lod_hi;
uniform float timed_lod_lo;

// tone image uniforms:
uniform ivec2     dims;     // size of window (width, height)
uniform sampler2D tone_tex; // same dimensions as window
uniform sampler2D halo_tex; 

uniform bool use_alpha_transitions;

// style info:
struct layer_t 
{
   // base functionality
   int          mode;
   bool         is_highlight;
   vec3         ink_color;
   float        pattern_scale;
   sampler2D    pattern;
   int          channel;

   // derived in Halftone_TX
   bool         use_tone_correction;
   bool         use_lod_only;
   sampler2D    tone_correction_tex;
   sampler2D    r_function_tex;
};

uniform layer_t layer[4];

float get_halo(in vec2 pix)
{
   // halos
   float halo_factor = texture2D(halo_tex, pix/vec2(dims)).r;
   halo_factor = min( (1.0 - halo_factor) * 10.0, 1.0); 
       
   return halo_factor;
}

float get_tone(in vec2 pix,in int layer_num)
{
   // get tone at given pixel location via tone reference image 
    
   vec4 t = texture2D(tone_tex, pix/vec2(dims));
   float ret = (layer[layer_num].channel == 0) ? t.r 
             : (layer[layer_num].channel == 1) ? t.g
             : (layer[layer_num].channel == 2) ? t.b
             : (layer[layer_num].channel == 3) ? t.a : t.r;
   
   float op = pow(opacity, op_remap);
   return mix(1.0, ret, op );
}

vec2 pix_to_uv(in vec2 p)
{
   // p is a pixel vector relative to the origin.
   // return its components relative to u and v vectors,
   // divided by tex_scale
   return vec2(dot(p,u_vec)/dot(u_vec,u_vec),
               dot(p,v_vec)/dot(v_vec,v_vec));
}

float 
f(in float x)
{
   // procedural dots function

   const float     PI = 3.1415926535;
   const float TWO_PI = 6.2831853071;
   const float      A = 0.4;
   return A*cos(x*TWO_PI)+0.5;
}

float 
get_tex_h(sampler2D halftone_tex,in vec2 uv, in float lod )
{
   float h_lo = texture2D(halftone_tex, uv*timed_lod_lo).r ;
   float h_hi = texture2D(halftone_tex, uv*timed_lod_hi).r;

   return  mix(h_lo, h_hi, lod);
}

float
halftone(in layer_t  layer, in vec2 u, in float tone, in float lod)
{
   float t = tone; // tone to be input to halftone process (may use correction)
  
   if (layer.use_tone_correction) {
      // using the error correction texture
      tone = (layer.use_lod_only ?
              texture2D(layer.r_function_tex, vec2(tone, 0.0)).r :
              tone);
      t =  texture2D(layer.tone_correction_tex, vec2(tone, lod)).r;
   }

   // "height" value in halftone screen:
   float h = 0.0;
   
   if (layer.mode==1) {
      // texture-based halftone screen
      h = get_tex_h(layer.pattern, u, lod);
   } else {
      // procedural halftone screen
      const float     PI = 3.1415926535;
      const float TWO_PI = 6.2831853071;
      const float      A = 0.4;
      const vec2    offs = vec2(0.5);
      
      vec2 xy_pi = u * TWO_PI;
      
      vec2 h_xy = mix(A*cos(xy_pi)+offs, A*cos(xy_pi*2.0)+ offs, lod);
   
      h = (h_xy.x + h_xy.y) * 0.5;
   }

   // Move the highlights into the "grooves" of the halftone screen
//    if (layer.is_highlight) {
//       h = 1.0 - h;
//    }

   const float e = 0.2; // for antialiasing
   return smoothstep(-e, e, h - t);
}

void add_layer_contribution(
   in vec2       uv,
   in layer_t    layer,
   in int        channel,
   inout vec3    col,
   inout float   alpha,
   in float      halo_factor
   )
{
   if (layer.mode != 0) {
      float tone = get_tone(gl_FragCoord.xy, channel);
      uv = uv / layer.pattern_scale;
      
      if (layer.is_highlight)
         tone = 1.0 - tone;
         
      tone = mix(tone, 1.0 , halo_factor);
      
      //if (use_alpha_transitions) {
      //   vec2  t = halftone_alpha(uv, tone);
      //   vec3 c0 = mix(col, layer.ink_color, t.x);
      //   vec3 c1 = mix(col, layer.ink_color, t.y);
      //   col = mix(c0,c1,st);
      //} else {
         float a_i = halftone(layer, uv, tone, st);
         col = mix(col, layer.ink_color, a_i);
         alpha = alpha * (1.0 - a_i);
      //}
   }
}

void main()
{
   vec2 uv = pix_to_uv(gl_FragCoord.xy - origin);

   vec3 col = vec3(0.0); //gl_Color.rgb;
   
   float a=1.0;
   float h=get_halo(gl_FragCoord.xy);
//   h = 0.0;

   // for loops don't work on ATI cards...
   add_layer_contribution(uv, layer[0], 0, col, a, h);
   add_layer_contribution(uv, layer[1], 1, col, a, h);
   add_layer_contribution(uv, layer[2], 2, col, a, h);
   add_layer_contribution(uv, layer[3], 3, col, a, h);
  
   gl_FragColor = vec4(col, a);

//   gl_FragColor = texture2D(tone_tex, gl_FragCoord.xy/vec2(dims));
//   gl_FragColor = vec4(gl_FragCoord.xy/vec2(dims), 0.0, 1.0);
// gl_FragColor = vec4(1.0, 1.0, 0.0, 0.0);
}

//-----------------------------------------------------------------

//alpha blended transitions
/*
vec2 halftone_alpha(in vec2 u, in float tone)
{
   // returns two output tones for both scales

   vec2 output;
   vec2 tn = vec2(tone,tone);

   if (halftone_valid) {
      output = vec2(
         texture2D(halftone_tex,u    ).r,
         texture2D(halftone_tex,u*2.0).r
         );
   } else {
      output = vec2((f(u.x)+f(u.y)), (f(u.x*2.0)+f(u.y*2.0)))/2.0;
   }

   const float e = 0.1; // for antialiasing
   return smoothstep(-e, e, output - tn);
}
*/

// halftone-dots.fp
