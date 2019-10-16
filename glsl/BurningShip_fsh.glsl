#version 410
precision highp float;

out vec4 fragColor;

in vec2 vFragCoord;

uniform float whichFractal;
uniform float zoom;
uniform vec2 currShift;
uniform float one;

//const int MAX_ITERS = 511;
const int MAX_ITERS = 255;
const float MAX_ITERS_f = float(MAX_ITERS);

const float X_MIN = -2.5;
const float X_MAX = 1.0;

const float Y_MIN = -1.5;
const float Y_MAX = 1.0;

const float ASPECT_RATIO = (X_MAX - X_MIN) / (Y_MAX - Y_MIN);

const float X_INCR = X_MAX - X_MIN;
const float Y_INCR = Y_MAX - Y_MIN;


/***** This block of code is from here (this emulates double-precision floating point arithmetic): https://github.com/10110111/QSMandel/blob/master/EmuMandel.fsh *****/
// Emulation based on Fortran-90 double-single package. See http://crd-legacy.lbl.gov/~dhbailey/mpdist/
// Add: res = ds_add(a, b) => res = a + b
vec2 ds_add (vec2 dsa, vec2 dsb)
{
vec2 dsc;
float t1, t2, e;

 t1 = dsa.x + dsb.x;
 e = (t1*one) - dsa.x;
 t2 = ((dsb.x - e) + (dsa.x - (t1 - e))) + dsa.y + dsb.y;

 dsc.x = t1 + t2;
 dsc.y = t2 - ((dsc.x*one) - t1);
 return dsc;
}

// Subtract: res = ds_sub(a, b) => res = a - b
vec2 ds_sub (vec2 dsa, vec2 dsb)
{
vec2 dsc;
float e, t1, t2;

 t1 = dsa.x - dsb.x;
 e = (t1*one) - dsa.x;
 t2 = ((-dsb.x - e) + (dsa.x - (t1 - e))) + dsa.y - dsb.y;

 dsc.x = t1 + t2;
 dsc.y = t2 - ((dsc.x*one) - t1);
 return dsc;
}

// Compare: res = -1 if a < b
//              = 0 if a == b
//              = 1 if a > b
float ds_compare(vec2 dsa, vec2 dsb)
{
 if (dsa.x < dsb.x) return -1.;
 else if (dsa.x == dsb.x) 
        {
        if (dsa.y < dsb.y) return -1.;
        else if (dsa.y == dsb.y) return 0.;
        else return 1.;
        }
 else return 1.;
}

// Multiply: res = ds_mul(a, b) => res = a * b
vec2 ds_mul (vec2 dsa, vec2 dsb)
{
vec2 dsc;
float c11, c21, c2, e, t1, t2;
float a1, a2, b1, b2, cona, conb, split = 8193.;

 cona = dsa.x * split;
 conb = dsb.x * split;
 a1 = cona - ((cona*one) - dsa.x);
 b1 = conb - ((conb*one) - dsb.x);
 a2 = dsa.x - a1;
 b2 = dsb.x - b1;

 c11 = dsa.x * dsb.x;
 c21 = a2 * b2 + (a2 * b1 + (a1 * b2 + (a1 * b1 - c11)));

 c2 = dsa.x * dsb.y + dsa.y * dsb.x;

 t1 = c11 + c2;
 e = t1 - (c11*one);
 t2 = dsa.y * dsb.y + ((c2 - e) + (c11 - (t1 - e))) + c21;
 
 dsc.x = t1 + t2;
 dsc.y = t2 - ((dsc.x*one) - t1);
 
 return dsc;
}

// create double-single number from float
vec2 ds_set(float a)
{
 vec2 z;
 z.x = a;
 z.y = 0.0;
 return z;
}
/***** End of block *****/

vec2 ds_abs(vec2 ds) {
  return vec2(abs(ds.x), abs(ds.y));
}

struct complex {
  vec2 re, im;
};

complex complexMult(complex v, complex w) {
  complex z;

  z.re = ds_sub(ds_mul(v.re, w.re), ds_mul(v.im, w.im));
  z.im = ds_add(ds_mul(v.re, w.im), ds_mul(v.im, w.re));
  return z;
}

complex complexAdd(complex v, complex w) {
  complex result;
  result.im = ds_add(v.im, w.im);
  result.re = ds_add(v.re, w.re);
  return result;
}

complex complexSquare(complex a) {
  return complexMult(a, a);
}

vec2 complexDot(complex v, complex w) {
  return ds_add(ds_mul(v.im, w.im), ds_mul(v.re, w.re));
}

complex nextZ(complex c, complex z) {
  complex absed;
  // absed.re = ds_dot(whichFractal, vec2(ds_abs(z.re), z.re));
  // absed.im = ds_dot(whichFractal, vec2(ds_abs(z.im), z.im));

  // absed.re = ds_abs(z.re);
  // absed.im = ds_abs(z.im);

  vec2 reAbs = ds_abs(z.re);
  vec2 imAbs = ds_abs(z.im);

  absed.re.x = mix(reAbs.x, z.re.x, whichFractal);
  absed.re.y = mix(reAbs.y, z.re.y, whichFractal);

  absed.im.x = mix(imAbs.x, z.im.x, whichFractal);
  absed.im.y = mix(imAbs.y, z.im.y, whichFractal);

  return complexAdd(complexSquare(absed), c);
}

int escapesAt(complex c) {
  complex z;
  z.im = ds_set(0.0);
  z.re = ds_set(0.0);

  for (int iters = 0; iters < MAX_ITERS; ++iters) {
    // if dot(z, z) > 4 ...
    if (ds_compare(complexDot(z,z), ds_set(4.0)) > 0.0) {
      return iters;
    }

    z = nextZ(c, z);
  }

  return -1;
}

// This function is from https://www.shadertoy.com/view/XljGzV
vec3 hsl2rgb( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );

    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}


vec4 coordColor(vec2 coord) {
  complex c;
  c.im = ds_set(coord.y);
  c.re = ds_set(coord.x);
  int iters = escapesAt(c);

  if (iters < 0) {
    return vec4(0, 0, 0, 1);
  } else {
    float adjIters = float(iters + 180);

    //float adjIters = float(iters + 350);

    return vec4(hsl2rgb(vec3(
            mod(adjIters, MAX_ITERS_f) / MAX_ITERS_f,
            1,
            0.5
            )
        ),
      1);
  }
}

vec2 rescaleCoord(vec2 v) {
  return vec2(v.x * X_INCR, v.y * -Y_INCR);
}

vec2 zoomCoord(vec2 v) {
  return vec2(v.x * zoom, v.y * zoom);
}

vec2 panCoord(vec2 v) {
  return vec2(v.x + currShift.x, v.y + currShift.y);
}

void main() {
  fragColor = coordColor(panCoord(zoomCoord(rescaleCoord(vFragCoord))));
}

