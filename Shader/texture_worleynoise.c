/*
 * declaration for shader texture_worleynoise, created Sat Dec 15 22:05:24 2012
 */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <assert.h>
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
  return d1 + d2;
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

miScalar dist_scale(dist_measure m) {
  switch(m) {
    case DIST_LINEAR: return 0.04;
    case DIST_LINEAR_SQUARED: return 0.025;
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

#define PTS_PER_CUBE 4
// 9 * PTS_PER_CUBE.
// in 3D, use 27 instead of 9
#define CACHE_SIZE 36
#define CUBE_DIST 0.05

// Element of the shader state:
// CUBE_RADIUS, defined as distance((0,0),(CUBE_DIST/2,CUBE_DIST/2))
// wrt the chosen distance measure

// has to fit the .mi file
typedef struct {
  miColor         inner;
  miColor         outer;
  miColor         gap;
  miInteger       distance_measure;
  miInteger distance_mode;
  miScalar scale;
  miScalar gap_size;
} texture_worleynoise_t;

typedef struct {
  miScalar cube_radius;
  
  miBoolean cache_initialized;
  miVector2d cacheCube; // the "center" cube of the cache
  miVector2d cacheVals[CACHE_SIZE]; // in 3D, use 27 instead of 9
} worley_context;

DLLEXPORT int texture_worleynoise_version(void) {return(1);}

DLLEXPORT miBoolean texture_worleynoise_init(
    miState *state,
    texture_worleynoise_t *param,
    miBoolean *init_req)
{
  worley_context **contextp;
  worley_context *context;
  
  if (!param) {
    /* shader init */
    *init_req = miTRUE; /* do instance inits */
  } else { /* shader instance init */
    // set up context
    
    mi_query(miQ_FUNC_USERPTR, state, 0, (void *)&contextp);
    context = *contextp = (worley_context*)mi_mem_allocate( sizeof(worley_context) );
    
    {
      miInteger distance_measure = *mi_eval_integer(&param->distance_measure);
      miVector2d zero; zero.u = 0; zero.v = 0;
      miScalar cube_dist = CUBE_DIST * (*mi_eval_scalar(&param->scale));
      miVector2d cube; cube.u = cube_dist / 2; cube.v = cube_dist / 2;
      context->cube_radius = distance(distance_measure, &zero, &cube);
    } // set cube_radius based on the selected distance measure
    
    {
      context->cache_initialized = 0;
    } // set cacheCube to be an invalid cube, so that the first cache hit fails
  }
  return(miTRUE);
}

DLLEXPORT miBoolean texture_worleynoise_exit(
    miState *state,
    texture_worleynoise_t *param)
{
  if (param) { /* shader instance exit */
    worley_context ** contextp;
    mi_query(miQ_FUNC_USERPTR, state, 0, (void *)&contextp);
    mi_mem_release(*contextp);
    *contextp = 0;
  } else {
    /* shader exit */
  }
  return(miTRUE);
}

miScalar worleynoise_val(miState *state,texture_worleynoise_t *param);
void grey_to_color(miScalar val, miColor *color1, miColor *color2, miColor *result);
DLLEXPORT miBoolean texture_worleynoise(
    miColor *result,
    miState *state,
    texture_worleynoise_t *param)
{
  /*
   * get parameter values. It is inefficient to do this all at the beginning of
   * the code. Move the assignments here to where the values are first used.
   * You may want to use pointers for colors and vectors.
   */
        
  miScalar val = worleynoise_val(state,param);
  if(val < 0) {
    miColor gap = *mi_eval_color(&param->gap);
    result->r = gap.r;
    result->g = gap.g;
    result->b = gap.b;
    result->a = gap.a;
  }
  else {
    miColor *inner = mi_eval_color(&param->inner);
    miColor *outer = mi_eval_color(&param->outer);
    
    grey_to_color(val, inner, outer, result);
  }
  
  return(miTRUE);
}

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

void point_distances(miState *state,texture_worleynoise_t *param, 
                     miVector2d *pt,
                     miScalar *f1, miVector2d *p1,
                     miScalar *f2, miVector2d *p2,
                     miScalar *f3, miVector2d *p3);

miScalar worleynoise_val(miState *state,texture_worleynoise_t *param) {
  miScalar f1, f2, f3;
  miVector2d p1, p2, p3;
  
  miVector pt0 = state->tex_list[0];
  miVector2d pt1; pt1.u = pt0.x; pt1.v = pt0.y;
  miVector2d *pt = &pt1;
  point_distances(state,param,pt,&f1,&p1,&f2,&p2,&f3,&p3);
  
  miInteger dist_measure = *mi_eval_integer(&param->distance_measure);
  
  miScalar scale = dist_scale(dist_measure);
  {
    f1 /= scale;
    f2 /= scale;
    f3 /= scale;
  }
  
  /* miScalar s = 1.0; */
  /* { */
  /*   miScalar gap_size = *mi_eval_scalar(&param->gap_size); */
     
  /*   // based on code from "Advanced Renderman" */
  /*   // this leads to gaps of equal width, in contrast to just simple thresholding of f2 - f1. */
  /*   miScalar scaleFactor = (distance(dist_measure, &p1, &p2) / */
  /*   			    (distance(dist_measure, pt, &p1) + distance(dist_measure, pt, &p2))); */
    

  /*   if(gap_size * (scaleFactor * scale) > f2 - f1) //  on left side */
  /*     s = -1.0; */
  /* } */
  
  miScalar dist = 0.0;
  {
    miInteger dist_mode = *mi_eval_integer(&param->distance_mode);
    switch(dist_mode) {
      case DIST_F1: dist = f1; break;
      case DIST_F2_M_F1: dist = f2 - f1; break;
      case DIST_F1_P_F2: dist = (2 * f1 + f2) / 3; break;
      case DIST_F3_M_F2_M_F1: dist = (2 * f3 - f2 - f1) / 2;
      case DIST_F1_P_F2_P_F3: dist = (0.5 * f1 + 0.33 * f2 + (1 - 0.5 - 0.33) * f3); break;
      default: ;
    }
  }
  
  return s * scaling_function(dist);
}

miVector2d point_cube(miVector2d *pt, miScalar cube_dist) {
  miVector2d cube;
  cube.u = floorf((pt->u) / cube_dist) * cube_dist;
  cube.v = floorf((pt->v) / cube_dist) * cube_dist;
  return cube;
}

void update_cache(worley_context *context, miVector2d *cube, miScalar cube_dist) {
  // in 3d, use mi_vector_dist instead of dist_linear_squared
  if(context->cache_initialized && dist_linear_squared(&(context->cacheCube), cube) <= FLT_EPSILON) {
    return;
  }
  // can do this cheaper in theory by reusing old cache values if the new cacheCube is not too far away from the old one
  context->cacheCube = *cube;
  
  {
    miVector2d currentCube;
    currentCube.u = cube->u - cube_dist;
    currentCube.v = cube->v - cube_dist;
    
    miVector2d *cache = context->cacheVals;

    // for the 3*3 cubes around the current cube,
    // calculate the random points in that cube
    for(int u=0; u<3; ++u) {
      currentCube.v = cube->v - cube_dist;
      for(int v=0; v<3; ++v) {
	miScalar uSeed = currentCube.u;
	miScalar vSeed = currentCube.v;
	miScalar uvIncrement = cube_dist / (PTS_PER_CUBE + 1);
	for(int k = 0; k < PTS_PER_CUBE; ++k) {
	  miVector2d pt = currentCube;
	  
	  // FIXME: this can be made better
	  // multiplication of seed by 500 is somewhat arbitrary.
	  pt.u += mi_unoise_1d(uSeed*500) * cube_dist;
	  pt.v += mi_unoise_1d(vSeed*500) * cube_dist;
	  cache[(v * 3 + u) * 4 + k] = pt;

	  uSeed += uvIncrement;
	  vSeed += uvIncrement;
	}

	currentCube.v += cube_dist;
      }
      currentCube.u += cube_dist;
    }
  }  
}

void point_distances(miState *state,texture_worleynoise_t *param, 
                     miVector2d *pt,
                     miScalar *f1, miVector2d *p1,
                     miScalar *f2, miVector2d *p2,
                     miScalar *f3, miVector2d *p3) {  
  miScalar cube_dist = CUBE_DIST * (*mi_eval_scalar(&param->scale));
  miVector2d cube = point_cube(pt,cube_dist);
  
  worley_context **contextp;
  worley_context *context;
  mi_query(miQ_FUNC_USERPTR, state, 0, (void *)&contextp);
  context = *contextp;
  
  update_cache(context, &cube, cube_dist);
  
  miVector2d *cache = context->cacheVals;
  miInteger dist_measure = *mi_eval_integer(&param->distance_measure);
  
  int i = 0;
  *p1 = cache[i++]; *f1 = distance(dist_measure, pt, p1);
  *p2 = cache[i++]; *f2 = distance(dist_measure, pt, p3);
  *p3 = cache[i++]; *f3 = distance(dist_measure, pt, p3);
  
  for(; i < 3 * 3 * PTS_PER_CUBE; ++i) {
    miVector2d p = cache[i];
    miScalar d = distance(dist_measure, pt, &p);
    if(d < *f3) {
      if(d < *f2) {
        f3 = f2; p3 = p2;
        if(d < *f1) {
          f2 = f1; p2 = p1;
          *f1 = d; *p1 = p;
        }
        else {
          *f2 = d; *p2 = p;
        }
      }
      else {
        *f3 = d; *p3 = p;
      }
    }
  }
}

