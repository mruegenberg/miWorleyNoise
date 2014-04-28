/*
 * declaration for shader texture_worleynoise, created Sat Dec 15 22:05:24 2012
 */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <float.h>


/************* Helpers *************/

#define mi_eval_vector2d(p) ((miVector2d  *)mi_eval(state, (p)))

#define mi_vector_abs(r) ((r)->x = fabs((r)->x),\
                          (r)->y = fabs((r)->y),\
                          (r)->z = fabs((r)->z))
                           
#define mi_vector_pow(r,p) ((r)->x = powf((r)->x,p),\
                            (r)->y = powf((r)->y,p),\
                            (r)->z = powf((r)->z,p))


/************* Distance measures *************/

miScalar dist_linear_squared(miVector2d *v1, miVector2d *v2);

miScalar dist_linear(miVector2d *v1, miVector2d *v2);

miScalar dist_manhattan(miVector2d *v1, miVector2d *v2);

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

typedef enum dist_measure {
  DIST_LINEAR = 0
, DIST_LINEAR_SQUARED = 1
, DIST_MANHATTAN = 2
// , DIST_MINKOWSKI = 3
} dist_measure;

typedef enum dist_mode {
  DIST_F1 = 0 // f1
, DIST_F2_M_F1 = 1 // f2 - f1, 
, DIST_F1_P_F2 = 2 // (2 * f1 + f2) / 3, 
, DIST_F3_M_F2_M_F1 = 3 // (2 * f3 - f2 - f1) / 2, 
, DIST_F1_P_F2_P_F3 = 4 // (0.5 * f1 + 0.33 * f2 + (1 - 0.5 - 0.33) * f3)
} dist_mode;

miScalar dist_scale(dist_measure m);

miScalar distance(dist_measure distance_measure, miVector2d *v1, miVector2d *v2);

/************* Shader *************/

#define PTS_PER_CUBE 4
#define CUBE_DIST 0.05

void grey_to_color(miScalar val, miColor *color1, miColor *color2, miColor *result);

// scales its positive input (which should already be approximately between 0 and 1) to the interval [0, 1)
// it is a tuned logistic sigmoid function
miScalar scaling_function(miScalar x);
