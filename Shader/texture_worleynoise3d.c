/*
 * declaration for shader texture_worleynoise3d, created Sat Dec 15 22:05:24 2012
 */

#include "common.h"

// 3^DIMENSIONS * PTS_PER_CUBE.
#define CACHE_SIZE 108

// has to fit the .mi file
typedef struct {	
	miBoolean jagged_gap;
	
  miColor         inner;
  miColor         outer;
  miColor         gap;
  miInteger       distance_measure;
  miInteger distance_mode;
  miScalar scale;
  miScalar gap_size;
} texture_worleynoise3d_t;

typedef struct {
  miBoolean cache_initialized;
  miVector cacheCube; // the "center" cube of the cache
  miVector cacheVals[CACHE_SIZE]; 
} worley_context3;

DLLEXPORT int texture_worleynoise3d_version(void) {return(2);}

DLLEXPORT miBoolean texture_worleynoise3d_init(
    miState *state,
    texture_worleynoise3d_t *param,
    miBoolean *init_req)
{
  *init_req = miTRUE;
  return(miTRUE);
}

DLLEXPORT miBoolean texture_worleynoise3d_exit(
    miState *state,
    texture_worleynoise3d_t *param)
{
  if (param) { /* shader instance exit */
    // note: is is indeed correct to release the contexts during shader instance exit!!!
    // (TODO: explain why)
    int num;
    worley_context3 **contexts;
    mi_query(miQ_FUNC_TLS_GETALL, state, miNULLTAG, &contexts, &num);
    for(int i=0; i < num; i++) {
      mi_mem_release(contexts[i]);
    }
  } else {
    /* shader exit */
  }
  return(miTRUE);
}

miScalar worleynoise3d_val(miState *state,texture_worleynoise3d_t *param);
DLLEXPORT miBoolean texture_worleynoise3d(
    miColor *result,
    miState *state,
    texture_worleynoise3d_t *param)
{
  worley_context3 *context;
  mi_query(miQ_FUNC_TLS_GET, state, miNULLTAG, &context);
  if (!context) {
    context = mi_mem_allocate( sizeof(worley_context3) );
    mi_query(miQ_FUNC_TLS_SET, state, miNULLTAG, &context);
    context->cache_initialized = 0;
  }
  
  
  miScalar val = worleynoise3d_val(state,param);
  
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

void point_distances3(miState *state,texture_worleynoise3d_t *param, 
                      miVector *pt,
                      miScalar *f1, miVector *p1,
                      miScalar *f2, miVector *p2,
                      miScalar *f3, miVector *p3);

miScalar worleynoise3d_val(miState *state,texture_worleynoise3d_t *param) {
  miScalar f1, f2, f3;
  miVector p1, p2, p3;
  
  // ways to get the current point:
  // state->tex_list[0]; // yields good results only in the x and y coordinate
  // state->point // usable for 3D, but problematic for getting a smooth 2D texture as x,y and z all have to be somehow incorporated in the 2D vector to use
  // state->tex // does not yield usable results / seems to be constant
	// 
	// instead, we just take an u and v value explicitly; they would usually be provided by a 2D placement node.
	
	// note: getting current values must always be wrapped in mi_eval... calls!
	miVector pt = state->point;
	
  point_distances3(state,param,&pt,&f1,&p1,&f2,&p2,&f3,&p3);
  
  miInteger dist_measure = *mi_eval_integer(&param->distance_measure);
  
  miScalar scale = dist_scale(dist_measure);
	miBoolean jagged = *mi_eval_boolean(&param->jagged_gap);
  
  miScalar s = 1.0;
  {
    miScalar gap_size = *mi_eval_scalar(&param->gap_size);
    
    miVector ptX = pt;
		// TODO
		// 
		// // jagged edges. useful for broken earth crusts
		// if(jagged) {
		// 	ptX.u += mi_noise_2d(pt.u*1000,pt.v*1000) * 0.15 * scale;
		// 	ptX.v += mi_noise_2d(pt.u*1000 + 100,pt.v*1000+100) * 0.15 * scale;
		// }
		
    miScalar f1X, f2X, f3X;
    miVector p1X, p2X, p3X;
    
    point_distances3(state,param,&ptX,&f1X,&p1X,&f2X,&p2X,&f3X,&p3X);
     
    // based on code from "Advanced Renderman"
    // this leads to gaps of equal width, in contrast to just simple thresholding of f2 - f1.
    miScalar scaleFactor = (distance3(dist_measure, &p1X, &p2X) * scale) / (f1X + f2X);
    
    // FIXME: there may be some adjustment needed for distance measures that are not just dist_linear
    if(gap_size * scaleFactor > f2X - f1X) //  on left side
      s = -1.0;
  }
  
  {
    f1 /= scale;
    f2 /= scale;
    f3 /= scale;
  }
  
  miScalar dist = 0.0;
  {
    miInteger dist_mode = *mi_eval_integer(&param->distance_mode);
    switch(dist_mode) {
      case DIST_F1: dist = f1; break;
      case DIST_F2_M_F1: dist = f2 - f1; break;
      case DIST_F1_P_F2: dist = (2 * f1 + f2) / 3; break;
      case DIST_F3_M_F2_M_F1: dist = (2 * f3 - f2 - f1) / 2; break;
      case DIST_F1_P_F2_P_F3: dist = (0.5 * f1 + 0.33 * f2 + (1 - 0.5 - 0.33) * f3); break;
      default: ;
    }
  }
  
  return s * scaling_function(dist);
}

miVector point_cube3(miVector *pt, miScalar cube_dist) {
  miVector cube;
  cube.x = floorf((pt->x) / cube_dist) * cube_dist;
  cube.y = floorf((pt->y) / cube_dist) * cube_dist;
	cube.z = floorf((pt->z) / cube_dist) * cube_dist;
  return cube;
}

void update_cache3(worley_context3 *context, miVector *cube, miScalar cube_dist) {
  // in 3d, use mi_vector_dist instead of dist_linear_squared
  if(context->cache_initialized && dist_linear_squared3(&(context->cacheCube), cube) <= FLT_EPSILON * 2) {
    return;
  }
  // note: in theory, we could reuse parts of the old cache if the new cube is adjacent to the cache cube
  context->cacheCube = *cube;
  
  {
    miVector currentCube;
    currentCube.x = cube->x - cube_dist;
    currentCube.y = cube->y - cube_dist;
    currentCube.z = cube->z - cube_dist;
    
    miVector *cache = context->cacheVals;

		// for the 3*3 cubes around the current cube,
		// calculate the random points in that cube
		for(int x=0; x<3; ++x) {
			currentCube.y = cube->y - cube_dist;
			for(int y=0; y<3; ++y) {
				currentCube.z = cube->z - cube_dist;
				for(int z=0; z<3; ++z) {
					miVector seed; 
					seed.x = currentCube.x * 1000;
					seed.y = currentCube.y * 1000;
					seed.z = currentCube.z * 1000;
					miScalar xyzIncrement = cube_dist / (PTS_PER_CUBE + 1);
					for(int k = 0; k < PTS_PER_CUBE; ++k) {
						miVector pt = currentCube;
						
						pt.x += mi_unoise_3d(&seed) * cube_dist;
						seed.x = (seed.x + xyzIncrement) * 1000.0;
						
						pt.y += mi_unoise_3d(&seed) * cube_dist;
						seed.y = (seed.y + xyzIncrement) * 1000.0;

						pt.z += mi_unoise_3d(&seed) * cube_dist;
						seed.z = (seed.z + xyzIncrement) * 1000.0;

						cache[(z * 3 * 3 + (y * 3 + x)) * PTS_PER_CUBE + k] = pt;
					}
					currentCube.z += cube_dist;
				}
				currentCube.y += cube_dist;
			}
			currentCube.x += cube_dist;
		}
	}
	
	context->cache_initialized = 1;
}

void point_distances3(miState *state,texture_worleynoise3d_t *param, 
                      miVector *pt,
                      miScalar *f1, miVector *p1,
                      miScalar *f2, miVector *p2,
                      miScalar *f3, miVector *p3) {  
  miScalar cube_dist = CUBE_DIST * (*mi_eval_scalar(&param->scale));
  miVector cube = point_cube3(pt,cube_dist);
  
  worley_context3 *context;
  mi_query(miQ_FUNC_TLS_GET, state, miNULLTAG, &context);
  
  miInteger dist_measure = *mi_eval_integer(&param->distance_measure);
  *f3 = FLT_MAX;
  *f2 = *f3 - 1;
  *f1 = *f2 - 1;

  update_cache3(context, &cube, cube_dist);
  
  miVector *cache = context->cacheVals;
  
  for(int i=0; i < CACHE_SIZE; ++i) { 
    miVector p = cache[i];
    miScalar d = distance3(dist_measure, pt, &p);
    if(d < *f3) {
      if(d < *f2) {
        *f3 = *f2; *p3 = *p2;
        if(d < *f1) {
          *f2 = *f1; *p2 = *p1;
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

