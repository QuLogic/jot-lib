// Dynamic 2D Patterns for Shading 3D Scenes
// 2007

varying float opacity;          // interpolated per-vertex opacity

// Layer Style Properties
struct layer_t
{
  int       mode;               // Should we use this layer?
  bool      is_highlight;       // If true: more light more ink, if false: more light less ink
  float     pattern_scale;      // Scale of the pattern
  sampler2D pattern;            // Hatching Texture
  vec3      ink_color;          // Color of the strokes
  int       channel;

  float     angle;              // Deafult relative orientation of the hatching XXX - degrees?
  float     paper_contrast;     // Contrast amplifies paper effect XXX - range?

  float     paper_scale;        // scale of paper texture
  float     tone_push;
};

uniform bool doing_sil;
uniform float op_remap;

uniform sampler2D paper_tex;    // Paper texture accessor

uniform layer_t layer[4];       // Currently we support 8 layers of hatching

// Coordinate system variables used to map transfomation matrix into UV Space
uniform vec2 origin;
uniform vec2 u_vec;
uniform vec2 v_vec;

// LOD
uniform float st;               // Interpolation parameter from LOD levels

// Tone Image
uniform sampler2D tone_tex;     // same dimensions as window
uniform sampler2D halo_tex;
uniform ivec2 tone_dims;        // (width, height) of the window

//Global vars
float halo_factor = texture2D(halo_tex, gl_FragCoord.xy / vec2(tone_dims)).r;
vec4 sil_ink = vec4(0.0);

// Maps pixel loction into UV space (needs to be scaled for diffent textures)
vec2
pix_to_uv(vec2 p, vec2 u, vec2 v)
{
  // return components of vector p wrt u and v vectors
  return vec2(dot(p, u) / dot(u, u), dot(p, v) / dot(v, v));
}

// Rotated pixel pt
vec2
get_rotated_pix(vec2 pix, float angle)
{
  // rotate fragment position and express it
  // relative to the dynamic pattern origin.

  float rad = radians(angle);
  float c = cos(rad);
  float s = sin(rad);
  return mat2(vec2(c, -s), vec2(s, c)) * (pix - origin);
}

float
get_tone(in vec2 pix, in int layer_num)
{
  // get tone at given pixel location via tone reference image

  // XXX - just doing texture2D(tone_tex, pix/vec2(tone_dims))[layer[layer_num].channel] crashed for some reason !!!! wtf...
  vec4 t = texture2D(tone_tex, pix / vec2(tone_dims));
  float ret = (layer[layer_num].channel == 0) ? t.r
            : (layer[layer_num].channel == 1) ? t.g
            : (layer[layer_num].channel == 2) ? t.b
            : (layer[layer_num].channel == 3) ? t.a : t.r;

  return ret;
}

float
paper_effect(in float pressure, in int layer_num)
{
  float p_scale = layer[layer_num].paper_scale;
  vec2 uv = pix_to_uv(gl_FragCoord.xy - origin, u_vec, v_vec);

  // XXX - why 10?
  uv = uv / (p_scale * 10.0);

  vec4 tex = vec4(0.0);

  vec4 h_lo = texture2D(paper_tex, uv);
  vec4 h_hi = texture2D(paper_tex, uv * 2.0);

  if (st <= 0.0) {
    tex = h_lo;
  } else if (st >= 1.0) {
    tex = h_hi;
  } else {
    // blend between the two results
    tex = mix(h_lo, h_hi, st);
  }

  // get height of paper at current pixel
  // (increase contrast)
  float h = clamp(layer[layer_num].paper_contrast * (tex.a - 0.5) + 0.5, 0.0, 1.0);
  float p = min(2.0 * pressure,       1.0);     // peak function
  float v = max(2.0 * pressure - 1.0, 0.0);     // valley function

  return mix(v, p, h);          // remapped alpha
}

vec2
hatch_uv(int layer_num)
{
  // rotate pix, map pix --> uv, scale uv:
  return pix_to_uv(get_rotated_pix(gl_FragCoord.xy, layer[layer_num].angle),
                   u_vec, v_vec) / (layer[layer_num].pattern_scale * 10.0);
}

vec4
hatching(in float tone, in int layer_num)
{
  vec4 result;

  vec2 uv = hatch_uv(layer_num);

  vec4 h1 = texture2D(layer[layer_num].pattern, uv);
  vec4 h2 = texture2D(layer[layer_num].pattern, uv * 2.0);

  float op = pow(opacity, op_remap);

  h1.a = (layer_num == 0) ? h1.a : h1.a * tone;
  h2.a = (layer_num == 0) ? h2.a : h2.a * tone;

  h1.a = 0.9 * sqrt(h1.a);
  h2.a = 0.9 * sqrt(h2.a);
  // LOD, we have two levels of detail

  h1.a = paper_effect(h1.a * op, layer_num);
  h2.a = paper_effect(h2.a * op, layer_num);

  if (st <= 0.0) {
    result = h1;
  } else if (st >= 1.0) {
    result = h2;
  } else {
    // blend between the two results
    result = mix(h1, h2, st);
  }

  return result;
}

void
add_layer_contribution(in vec2 uv_coord,
                       in int layer_num, inout vec3 ink, inout float alpha)
{
  if (layer[layer_num].mode == 1) {
    float tone = get_tone(gl_FragCoord.xy, layer_num);
    tone = (layer[layer_num].is_highlight) ? (1.0 - tone) : tone;
    tone = clamp(tone + layer[layer_num].tone_push / 2.0, 0.0, 1.0);
    vec4 col = hatching(tone, layer_num);
    if (layer_num == 2)
      sil_ink = col;

    ink = mix(ink, col.rgb, col.a);

    alpha = alpha * (1.0 - col.a);
  }
}

void
main()
{
  // UV coordinates:
  vec2 uv_coord = pix_to_uv(gl_FragCoord.xy - origin, u_vec, v_vec);

  vec3 ink = vec3(0.0);
  float a = 1.0;
  // We have 4 layers, each layer modifies the ink
  add_layer_contribution(uv_coord, 0, ink, a);
  add_layer_contribution(uv_coord, 1, ink, a);
  add_layer_contribution(uv_coord, 2, ink, a);
  add_layer_contribution(uv_coord, 3, ink, a);

  if (doing_sil) {
    ink = sil_ink.rgb;
    a = sqrt(1.0 - halo_factor);
  }

  gl_FragColor = vec4(ink, a);
}
