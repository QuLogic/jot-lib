// hatching.fp
//
//   does simple hatching with strokes like pencil
//
//   needs: LOD, extra layers (light, dark, cross hatch...)

// dynamic 2D pattern parameters:
uniform vec2 origin;
uniform vec2 u_vec;
uniform vec2 v_vec;

// paper texture:
uniform float tex_width;
uniform float tex_height;
uniform float contrast;

uniform sampler2D paper_tex; 
uniform sampler2D sampler2D_perlin;

uniform int visible[4];       //is this layer visible
uniform vec3 color[4];
uniform float line_spacing[4];
uniform float line_width[4];
uniform int do_paper[4];
uniform float angle[4];
uniform float perlin_amp[4];
uniform float perlin_freq[4];
uniform float min_tone[4];
uniform int highlight[4];
uniform int channel[4];

uniform float st;            // interpolation parameter from fat to thin lines

// tone image uniforms

uniform sampler2D tone_map;
uniform int width;
uniform int height;

varying float nv;    // n dot v:

vec2 pix_to_uv(vec2 p, vec2 u, vec2 v)
{
   // return components of vector p wrt u and v vectors

   return vec2(dot(p,u)/dot(u,u),dot(p,v)/dot(v,v));
}

float paper_effect(float pressure)
{
   vec2 u_vec_p = u_vec*tex_width;
   vec2 v_vec_p = v_vec*tex_height;
   vec2 u       = pix_to_uv(gl_FragCoord.xy - origin, u_vec_p, v_vec_p);

   float s1 = 1.0;
   float s0 = s1/2.0;
   vec4 tex = vec4(0.0);
   if (st <= 0.0) {
      tex = texture2D(paper_tex, u/s1);
   } else if (st >= 1.0) {
      tex = texture2D(paper_tex, u/s0);
   } else {
      // blend between the two results
      tex = mix(texture2D(paper_tex, u/s1),texture2D(paper_tex, u/s0),st);
   }

   // get height of paper at current pixel
   // (increase contrast)
   float h = clamp(contrast*(tex.a-0.5)+0.5,0.0,1.0); 

   float p = min(2.0 * pressure,      1.0);      // peak function
   float v = max(2.0 * pressure -1.0, 0.0);      // valley function

   return mix(v,p,h);                            // remapped alpha
}

float get_tone(float min_tone,int highlight,int mychannel)
{
   vec2  uv   = vec2(gl_FragCoord.x/float(width), gl_FragCoord.y/float(height));
   float tone = (mychannel==0) ? texture2D(tone_map, uv).r : (mychannel==1) ? texture2D(tone_map, uv).g : (mychannel==2) ? texture2D(tone_map, uv).b : texture2D(tone_map, uv).a;
   return (highlight==1) ? clamp(1.0-(tone + min_tone),0.0,1.0) : (tone + min_tone);
}

float interp(float x0, float y0, float x1, float y1, float x)
{
   // linearly interpolate from (x0,y0) to (x1,y1) for given x
   return (x-x0)*(y1-y0)/(x1-x0) + y0;
}

float sqr(float x)
{
   return x*x;
}

float dropoff(float x0, float y0, float x1, float y1, float x2, float x)
{
   // required: x0 < x1 < x2
   //
   // interpolates y in [x0,x1], then tapers y down to 0 in
   // the shape of a tilted ellipse in [x1,x2]:
   //
   // used for tapering width and alpha along a line, based on tone

   float y = interp(x0,y0,x1,y1,x);
   return ((x < x0) ? y0   :
           (x < x1) ? y    :
           (x > x2) ? 0.0  :
           y*sqrt(1.0 - sqr((x-x1)/(x2-x1))));
}

float line_alpha(float x, float w)
{
   // compute an alpha value based on distance to the nearest
   // canonical line: 0.0 away from the line, 1.0 on the line, 
   // with a smooth fall-off.

   // find distance to nearest canonical line
   x = fract(x);
   x = min(x,1.0-x); 
   return (x > w) ? 0.0 : 1.0 - sqr(x/w);
}

float sil_alpha(float e0, float e1)
{
   // do silhouette computation
   // return alpha == 1 on silhouette,
   //                 0 away from it

   // e0 is inner sil width,
   // e1 is outer width (required: 0 < e0 < e1).
   // alpha is 1.0 inside inner width, and 
   // fades to 0.0 at the outer width.

   float grad = length(vec2(dFdx(nv),dFdy(nv)));
   float dist = abs(nv)/grad;
   return (grad > 1e-5) ? smoothstep(-e1, -e0, -dist) : 0.0;
}

vec2 get_pix(float angle)
{
   // rotate fragment position and express it
   // relative to the dynamic pattern origin.

   // start off w/ 45 degree rotation for fun:
   float rad = radians(angle);
   float c = cos(rad);
   float s = sin(rad);
   //const float s2 = 1.41421356/2.0;
   mat2 R = mat2(vec2(c,-s),vec2(s,c));

   // rotated pixel vector (relative to origin):
   return R * (gl_FragCoord.xy - origin);
}

vec2 get_noise2(vec2 u, float amp, float freq)
{
   // perlin noise tends to be unsupported so we get it from texture
   // Yeah right, it works just fine 
   // karol's experiment : 
   // the noise is centered around 0.5  
   return amp * ((texture2D(sampler2D_perlin, freq * u)).xy - vec2(0.5,0.5));
}

vec3
color_ub(float r, float g, float b)
{
   // specify a color with 3 integers in [0,255]:
   return vec3(r/255.0,g/255.0,b/255.0);
}

float
stroke_ink(vec2 u, float width, float tone, int do_paper)
{
   // return the alpha value at the given uv coordinate
   // indicating how much ink is there.
   // take into account:
   //   - hatching strokes
   //   - silhouette suppression of hatching
   //   - silhouette pixels themselves
   //   - paper effect

   // find tone (for now using mike's toon shader)
   // artificially lighten it near silhouettes:
   float t = mix(tone, 1.0, pow(1.0-clamp(nv,0.0,1.0),2.0)); 

   const float t0 = 0.0; // darkest tone that exists
   const float t1 = 0.4; // damping begins here; ends at t2
   const float t2 = 0.6; // lightest tone for drawing lines

   // set a line width that tapers (slightly) with tone:
   // widths are in canonical space with line spacing = 1.0
   const float w0 = (width/2.0);       // semi-width for darkest tone (t0)
   const float w1 = w0 - (w0/2.0);       // semi-width for lightest tone (t1)
   float w = dropoff(t0, w0, t1, w1, t2, t);

   // set line alpha from tone:
   // tells how alpha varies along the line
   const float a0 = 0.75;       // alpha for darkest tone (t0)
   const float a1 = 0.45;       // alpha for lightest tone (t1)
   float a = dropoff(t0, a0, t1, a1, t2, t);

   // multiply in cross-stroke alpha:
   a *= line_alpha(u.y, w);
    
   
   // silhouette alpha:
   //float s = a0*sil_alpha(2.0, 4.0);
    
   return (do_paper==1) ? paper_effect(a) : a;
}

float get_layer(int layer)
{
   // rotated pixel vector relative to origin:
   vec2 p = get_pix(angle[layer]);

   // dynamic pattern coordinates (uv-space):
   vec2 u = pix_to_uv(p, u_vec, v_vec);
  
   float a = 0.0;             // amount of ink to deposit
   float tone = get_tone(min_tone[layer], highlight[layer], channel[layer]);   // target tone
   float s1 = line_spacing[layer];    // spacing for fat lines
   float s0 = s1/2.0;                   // spacing for thin lines
   
   vec2  uv_lo = u/s0;
   vec2  uv_hi = u/s1; 
	
	uv_lo += get_noise2(uv_lo,perlin_amp[layer], perlin_freq[layer]);
	uv_hi += get_noise2(uv_hi,perlin_amp[layer], perlin_freq[layer]);
		 
   if (st <= 0.0) {
      a = stroke_ink(uv_hi, line_width[layer], tone ,do_paper[layer]);
   } else if (st >= 1.0) {
      a = stroke_ink(uv_lo, line_width[layer], tone ,do_paper[layer]);
   } else {
      // blend between the two results
      a = mix(stroke_ink(uv_hi, line_width[layer], tone, do_paper[layer]),
              stroke_ink(uv_lo, line_width[layer], tone, do_paper[layer]), st);
   }   
   
   return a;
}

void main()
{
   vec3 new_col= gl_Color.rgb;
   // We have 4 layers
   for(int i=0; i < 4; ++i) {
      if(visible[i] == 1){         
         new_col  = mix(new_col, color_ub(color[i].x,color[i].y, color[i].z) , get_layer(i));   
      } 
   } 
   // blend the stroke color with the base primitive color:
   gl_FragColor = vec4(vec3(mix(gl_Color.rgb, new_col.rgb, 0.5)),gl_Color.a);
} 

// hatching.fp
