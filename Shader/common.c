/*
 * declaration for shader texture_worleynoise, created Sat Dec 15 22:05:24 2012
 */

#include "common.h"

/************* Distance measures *************/

miScalar dist_linear_squared(miVector2d *v1, miVector2d *v2) {
  // 3D:
  // miVector s; mi_vector_sub(&s,&v1,&v2)
  // return mi_vector_dot(r,r);
  miScalar d1 = v1->u - v2->u;
  miScalar d2 = v1->v - v2->v;
  return d1 * d1 + d2 * d2;
}

miScalar dist_linear(miVector2d *v1, miVector2d *v2) {
  return sqrt( dist_linear_squared(v1,v2) );
}

miScalar dist_manhattan(miVector2d *v1, miVector2d *v2) {
  // 3D:
  // miVector s; mi_vector_sub(&s,&v1,&v2)
  // return s.x + s.y + s.z;
  miScalar d1 = v1->u - v2->u;
  miScalar d2 = v1->v - v2->v;
  return fabs(d1) + fabs(d2);
}

// // note: Behavior might get weird for p <= 0.
//          Usually, use integer values for p.
// miScalar dist_minkowski(miScalar p, miVector2d v1, miVector2d v2) {
//   // 3D:
//   // miVector s; mi_vector_sub(&r,&v1,&v2);
//   // mi_vector_abs(&s,&s);
//   // mi_vector_pow(&s,p);
//   // return powf(s.x + s.y + s.z,(1/p));
//   miScalar d1 = powf(fabs(v1.u - v2.u),p);
//   miScalar d2 = powf(fabs(v1.v - v2.v),p);
//   return powf(d1 + d2, 1/p);
// }

miScalar dist_scale(dist_measure m) {
  switch(m) {
    case DIST_LINEAR: return 0.04;
    case DIST_LINEAR_SQUARED: return 0.01;
    case DIST_MANHATTAN: return 0.07;
    // case DIST_MINKOWSKI: return 30; // actually, this should be based on the parameter p. This is for 2, since that is the same as linear/euclidean distance
    default: return -1;
  }
}

miScalar distance(dist_measure distance_measure, miVector2d *v1, miVector2d *v2) {
  switch(distance_measure) {
    case DIST_LINEAR: return dist_linear(v1,v2);
    case DIST_LINEAR_SQUARED: return dist_linear_squared(v1,v2);
    case DIST_MANHATTAN: return dist_manhattan(v1,v2);
    // case DIST_MINKOWSKI: return 30; // actually, this should be based on the parameter p. This is for 2, since that is the same as linear/euclidean distance
    default: return -1;
  }
}


/************* Shader *************/

void grey_to_color(miScalar val, miColor *color1, miColor *color2, miColor *result) {
  result->r = (color1)->r * (1 - val) + (color2)->r * val;
  result->g = (color1)->g * (1 - val) + (color2)->g * val;
  result->b = (color1)->b * (1 - val) + (color2)->b * val;
  result->a = (color1)->a * (1 - val) + (color2)->a * val;
}

// scales its positive input (which should already be approximately between 0 and 1) to the interval [0, 1)
// it is a tuned logistic sigmoid function
miScalar scaling_function(miScalar x) {
  return 2 * (1 / (1 + expf((-1) * (3*x))) - 0.5);
}
