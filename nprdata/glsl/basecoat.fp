// tone shader fragment program.

uniform sampler2D toon_tex; 
uniform sampler2D tex_2d; 
uniform bool is_tex_2d;

varying vec4 P; // fragment position, eye space
varying vec3 N; // fragment normal, eye space


uniform bool is_enabled; // constant or not?
uniform bool remap_nl;   // convert n*l in [-1,1] to [0,1]
uniform int  remap;      // remap tone?    0: none, 1: toon, 2: smoothstep
uniform int  backlight;  // add backlight? 0: none, 1: dark, 2: light
uniform float e0;        // smoothstep edge 0
uniform float e1;        // smoothstep edge 1
uniform float s0;        // backlight edge 0
uniform float s1;        // backlight edge 1
uniform vec3  base_color0;     // basecoat color
uniform vec3  base_color1;     // basecoat color
uniform float color_offset;
uniform float color_steepness;
uniform int light_separation;

float luminance(vec4 col)
{
   // convert RGB color to luminance:
   return dot(col.rgb, vec3(0.3, 0.59, .11));
}

vec4 light_contribution(
   gl_LightSourceParameters gl_params, // parameters for current light
   vec3 n,                             // unit surface normal, eye space
   vec3 v                              // unit view vector, eye space
   )
{
   vec4 q = gl_params.position;

   // Direction to light from P:
   vec3 l = normalize(((q.w == 0.0) ? q : (q - P)).xyz);

   // diffuse part:
   float nl = dot(n,l);
   nl = remap_nl ? (nl + 1.0)/2.0 : nl;
   nl = clamp(nl, 0.0, 1.0);

   // specular part:
   vec3  r = reflect(-l,n);
   float rv = dot(r, v);
   rv = clamp(rv, 0.0, 1.0);
   rv = pow(rv, gl_FrontMaterial.shininess);

   // skipping spotlight effect..

   // attenuation:
   float t = 1.0;
   if (q.w != 0.0) { // if positional
      float d = distance(q, P);
      float k0 = gl_params.constantAttenuation;
      float k1 = gl_params.linearAttenuation;
      float k2 = gl_params.quadraticAttenuation;
      t = 1.0 / (k0 + d*(k1 + d*k2));
   }

   // resulting color (before attenuation):
   vec4 ret = (((gl_params.ambient  * gl_FrontMaterial.ambient)      ) +
               ((gl_params.diffuse  * gl_FrontMaterial.diffuse)  * nl) +
               ((gl_params.specular * gl_FrontMaterial.specular) * rv));

   // return result with attenuation:
   return clamp(ret * t, 0.0, 1.0);
}

float sil_val(vec3 n, vec3 v)
{
   // return 1 at silhoutte, falling to 0 away from silhouette
   return smoothstep(s0, s1, 1.0 - dot(n,v));
}

vec4 basecoat_color(
   gl_LightSourceParameters gl_params, // parameters for current light
   vec3 n,                             // unit surface normal, eye space
   vec3 v                              // unit view vector, eye space
   )
{
   vec4 col;
   float ret = 0.0;
   vec3 base_color = base_color0;
   if (is_enabled) {
      // compute tone via standard lighting:

      if(!is_tex_2d){
         ret = luminance(light_contribution(gl_params, n, v));
      }
      else{
         ret = luminance(texture2D(tex_2d, gl_TexCoord[0].st));
      }

      // optionally remap tone:
      if (remap == 1) {                   // toon remapping
         // lookup alpha via light strength;
         // for strength near 0, alpha is near 1,
         // so remapping needs 1.0 - alpha:
         ret = 1.0 - texture2D(toon_tex, vec2(ret, 0.0)).a;
      } else if (remap == 2) {            // smoothstep remapping
         ret = smoothstep(e0, e1, ret);

	 if(ret >= e1){
	    base_color = base_color1;
	 }
	 else if(ret >= e0 && ret < e1){
	    float r = (ret-e0)/(e1-e0);
	    base_color = (base_color0*(1.0-r)+base_color1*r);
	 }
      }

      // optionally add backlight
      if(!is_tex_2d){
         if (backlight == 1) {               // dark                  
            float val = sil_val(n,v);
            ret = min(ret, 1.0 - val);
         } else if (backlight == 2) {        // light
            float val = sil_val(n,v);
            ret = max(ret, val);
         }
      }
      // reduce the effect if incoming alpha value is < 1:
      ret = mix(1.0, ret, gl_Color.a);
      ret = clamp(ret * color_steepness + color_offset, 0.0, 1.0);

      col = vec4(base_color.r*ret, base_color.g*ret, base_color.b*ret, 1.0);


   }
   else{
      col.rgb = base_color;
      col.a = 1.0;

   }

   return col;
}

void main() 
{
   vec3 n = normalize(N);       // normal
   vec3 v = normalize(-P.xyz);  // view vector

   // BTW, the compiler can't deal with for loops

   if(light_separation == 0)
      gl_FragColor = basecoat_color(gl_LightSource[0], n, v);
   else
      gl_FragColor = basecoat_color(gl_LightSource[5], n, v);
}

// basecoat.fp
