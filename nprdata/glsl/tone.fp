// tone shader for "dynamic 2D patterns"

uniform sampler2D toon_tex; 
uniform sampler2D tex_2d; 
uniform bool is_tex_2d;
uniform bool is_reciever;

varying vec4 P;     // fragment position, eye space
varying vec3 N;     // fragment normal, eye space

struct layer_t {
   bool is_enabled; // use this layer?
   bool remap_nl;   // convert n*l in [-1,1] to [0,1]
   int  remap;      // remap tone?    0: none, 1: toon, 2: smoothstep
   int  backlight;  // add backlight? 0: none, 1: dark, 2: light
   float e0;        // smoothstep edge 0
   float e1;        // smoothstep edge 1
   float s0;        // backlight edge 0
   float s1;        // backlight edge 1
};
uniform layer_t layer[4]; // max 4 layers

struct occluder_t {
   bool  is_active; // tells if occluder is enabled
   mat4  xf;        // xform from eye space to occluder object space
   float softness;  // ? - not documented, currently unused
};
uniform occluder_t occluder[4]; // max 4 occluders

bool 
intersect_sphere(
   in vec3    o,    // ray origin
   in vec3    d,    // ray vector
   out float t0,    // ray parameter of 1st intersection
   out float t1     // ray parameter of 2nd intersection
   )
{
   // ray-sphere intersection

   // the sphere is in canonical space:
   //   center at origin,
   //   radius 1
   // the ray is given in the same canonical space

   bool ret = false;
   float a = dot(d,d);
   float b = dot(o,d);       // not times 2, that's compensated below
   float c = dot(o,o) - 1.0;
   float det = b*b - a*c;    // ac, not 4ac, same reason
   if (det > 0.0 && a > 1e-8) {
      b = -b;
      det = sqrt(det);
      t0 = (b - det)/a; // divide by a (not 2a), compensates for above changes
      t1 = (b + det)/a;

      // if t1 < 0, there is no intersection.  otherwise, it
      // depends on whether the light is point or directional.
      // so this return value may give a false positive, and
      // the calling function has to do more checking:
      ret = (t1 > 0.0);
   }  
   return ret;
}

float
shadow_effect(in occluder_t occ, in vec4 l, in bool is_directional)
{
   // return value is attenuation due to shadow,
   // so 0 means full shadow, 1 means no shadow

   // l is vector to the light. for positional light, it is the
   // vector from the surface to the light position; otherwise
   // it just encodes direction, not necessarily unit length.

   float ret = 1.0;
   if (occ.is_active) {

      // intersect ray with sphere.
      // convert ray origin/direction to occluder object space
      float t0, t1; // ray parameters at intersection points
      if (intersect_sphere((occ.xf*P).xyz, (occ.xf*l).xyz, t0, t1)) {
         if (is_directional) {
            // directional
            ret = ((t1 < 0.0) ? 1.0 :
                   (t0 < 0.0) ? 0.0 :
                   t0/t1);
         } else {
            // positional
            ret = ((t1 < 0.0 || t0 > 1.0 || (t0 < 0.0 && t1 > 1.0)) ? 1.0 :
                   (t1 > 1.0) ? t0 :
                   (t0 < 0.0) ? 0.0 :
                   t0/t1);
         }
         // tweak the effect for faster fall-off:
         ret = smoothstep(0.0, 0.8, sqrt(ret));
      }
   }
   return ret;
}

float luminance(in vec4 col)
{
   // convert RGB color to luminance:
   return dot(col.rgb, vec3(0.3, 0.59, .11));
}

vec4 light_contribution(
   in gl_LightSourceParameters light, // parameters for current light
   in layer_t                  layer, // layer info
   in vec3 n,                         // unit surface normal, eye space
   in vec3 v                          // unit view vector, eye space
   )
{
   vec4 q = light.position;

   // Direction to light from P:
   vec3 l = normalize(((q.w == 0.0) ? q : (q - P)).xyz);

   // diffuse part:
   float nl = dot(n,l);
   nl = layer.remap_nl ? (nl + 1.0)/2.0 : nl;
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
      float k0 = light.constantAttenuation;
      float k1 = light.linearAttenuation;
      float k2 = light.quadraticAttenuation;
      t = 1.0 / (k0 + d*(k1 + d*k2));
   }

   // resulting color (before attenuation):
   vec4 ret = (((light.ambient  * gl_FrontMaterial.ambient)      ) +
               ((light.diffuse  * gl_FrontMaterial.diffuse)  * nl) +
               ((light.specular * gl_FrontMaterial.specular) * rv));

   // return result with attenuation:
   return clamp(ret * t, 0.0, 1.0);
}

float sil_val(vec3 n, vec3 v, layer_t layer)
{
   // return 1 at silhoutte, falling to 0 away from silhouette
   return smoothstep(layer.s0, layer.s1, 1.0 - dot(n,v));
}

float layer_val(
   in gl_LightSourceParameters light, // parameters for current light
   in layer_t                  layer, // additional parameters...
   in vec3 n,                         // unit surface normal, eye space
   in vec3 v                          // unit view vector, eye space
   )
{
   float ret = 0.0;
   if (layer.is_enabled) {
      // compute tone via standard lighting:
      ret = luminance(light_contribution(light, layer, n, v));

      // optionally remap tone:
      if (layer.remap == 1) {                   // toon remapping
         // lookup alpha via light strength;
         // for strength near 0, alpha is near 1,
         // so remapping needs 1.0 - alpha:
         ret = 1.0 - texture2D(toon_tex, vec2(ret, 0.0)).a;
      } else if (layer.remap == 2) {            // smoothstep remapping
         ret = smoothstep(layer.e0, layer.e1, ret);
      }

      // optionally add backlight
      if (layer.backlight == 1) {               // dark                  
         ret = min(ret, 1.0 - sil_val(n,v,layer));
      } else if (layer.backlight == 2) {        // light
         ret = max(ret, sil_val(n,v,layer));
      }

      // add shadow effect
      if (is_reciever) {

         bool is_directional = (light.position.w == 0.0);

         // vector to light, not unit length (in eye space):
         vec4 l = is_directional ? light.position : (light.position - P);

         // add shadow effect from each occluder:
         ret *= shadow_effect(occluder[0], l, is_directional);
         ret *= shadow_effect(occluder[1], l, is_directional);
         ret *= shadow_effect(occluder[2], l, is_directional);
         ret *= shadow_effect(occluder[3], l, is_directional);
      }
      
      // reduce the effect if incoming alpha value is < 1:
      ret = mix(1.0, ret, gl_Color.a);
   }
   return ret;
}

void main() 
{
   vec3 n = normalize(N);       // normal
   vec3 v = normalize(-P.xyz);  // view vector

   // ATI compilers barf on for loops
   if(!is_tex_2d){
   gl_FragColor = vec4(layer_val(gl_LightSource[0], layer[0], n, v),
                       layer_val(gl_LightSource[1], layer[1], n, v),
                       layer_val(gl_LightSource[2], layer[2], n, v),
                       layer_val(gl_LightSource[3], layer[3], n, v));
   }
   else{
      float lumi = luminance(texture2D(tex_2d, gl_TexCoord[0].st));      
      gl_FragColor = vec4(lumi, lumi, lumi, 1.0);
   }
}

// tone.fp
