/** @internal
 ** @file     dft.c
 ** @author   Andrea Vedaldi
 ** @brief    Dense Feature Transform (SIFT) - MEX
 **/

#include <mexutils.h>
#include <vl/mathop.h>
#include <vl/dft.h>

#include <math.h>
#include <assert.h>

/* option codes */
enum {
  opt_step = 0,
  opt_size,
  opt_magnif,
  opt_verbose
} ;

/* options */
uMexOption options [] = {
{"Step",         1,   opt_step          },
{"Size",         1,   opt_size          },
{"Magnif",       1,   opt_magnif        },
{"Verbose",      0,   opt_verbose       },
{0,              0,   0                 }
} ;

/** -------------------------------------------------------------------
 ** @internal
 ** @brief Ordering of tuples by increasing scale
 ** 
 ** @param a tuple.
 ** @param b tuble.
 **
 ** @return @c a[2] < b[2]
 **/

static int
korder (void const* a, void const* b) {
  double x = ((double*) a) [2] - ((double*) b) [2] ;
  if (x < 0) return -1 ;
  if (x > 0) return +1 ;
  return 0 ;
}

/** ------------------------------------------------------------------
 ** @brief MEX entry point
 **/

void
mexFunction(int nout, mxArray *out[], 
            int nin, const mxArray *in[])
{
  enum {IN_I=0,IN_END} ;
  enum {OUT_FRAMES, OUT_DESCRIPTORS} ;
  
  int                verbose = 0 ;
  int                opt ;
  int                next = IN_END ;
  mxArray const     *optarg ;
  
  float const       *data ;
  int                M, N ;
  
  int                step = 1 ;
  int                size = 3 ;
  double             magnif = 3.0f ;
  
  VL_USE_MATLAB_ENV ;
  
  /* -----------------------------------------------------------------
   *                                               Check the arguments
   * -------------------------------------------------------------- */
  
  if (nin < 1) {
    mexErrMsgTxt("One argument required.") ;
  } else if (nout > 2) {
    mexErrMsgTxt("Too many output arguments.");
  }
  
  if (mxGetNumberOfDimensions (in[IN_I]) != 2              ||
      mxGetClassID            (in[IN_I]) != mxSINGLE_CLASS  ) {
    mexErrMsgTxt("I must be a matrix of class SINGLE") ;
  }
  
  data = (float*) mxGetData (in[IN_I]) ;
  M    = mxGetM (in[IN_I]) ;
  N    = mxGetN (in[IN_I]) ;
  
  while ((opt = uNextOption(in, nin, options, &next, &optarg)) >= 0) {
    switch (opt) {
        
      case opt_verbose :
        ++ verbose ;
        break ;
        
      case opt_size :
        if (!uIsRealScalar(optarg) || (size = (int) *mxGetPr(optarg)) < 0) {
          mexErrMsgTxt("'Size' must be a positive integer.") ;
        }
        break ;
        
      case opt_step :
        if (!uIsRealScalar(optarg) || (step = (int) *mxGetPr(optarg)) < 0) {
          mexErrMsgTxt("'Step' must be a positive integer.") ;
        }
        break ;
        
      case opt_magnif :
        if (!uIsRealScalar(optarg) || (magnif = *mxGetPr(optarg)) < 0) {
          mexErrMsgTxt("'Magnif' must be a non-negative real.") ;
        }
        break ;
        
      default :
        assert(0) ;
        break ;
    }
  }
  
  /* -----------------------------------------------------------------
   *                                                            Do job
   * -------------------------------------------------------------- */
  {
    int nkeys, k, i ;
    VlDftKeypoint* keys ;
    float* descs ;

    VlDftFilter *dft ;    
    dft = vl_dft_new (M, N, step, size) ; 
 
    nkeys = vl_dft_get_keypoint_num (dft) ;
    keys  = vl_dft_get_keypoints (dft) ;
    descs = vl_dft_get_descriptors (dft) ;
    
    if (verbose) {
      mexPrintf("dft: image size: %d x %d\n", M, N) ;
      mexPrintf("dft: step:       %d\n", step) ;
      mexPrintf("dft: size:       %d\n", size) ;
      mexPrintf("dft: num keys:   %d\n", nkeys) ;  
    }
    
    vl_dft_process (dft, data) ;
    
    
    /* ---------------------------------------------------------------
     *                                            Create output arrays
     * ------------------------------------------------------------ */
    {
      int dims [2] ;
      
      dims [0] = 128 ;
      dims [1] = nkeys ;
      
      out[OUT_DESCRIPTORS] = mxCreateNumericArray
      (2, dims, mxUINT8_CLASS, mxREAL) ;
      
      dims [0] = 2 ;
      
      out[OUT_FRAMES] = mxCreateNumericArray
      (2, dims, mxDOUBLE_CLASS, mxREAL) ;
    }
    
    /* ---------------------------------------------------------------
     *                                                       Copy back
     * ------------------------------------------------------------ */
    {
      double *kpt = mxGetPr(out[OUT_FRAMES]) ;
      vl_uint8  *dpt = mxGetData(out[OUT_DESCRIPTORS]) ;
      for (k = 0 ; k < nkeys ; ++k) {
        float tmpdesc [128] ;
        *kpt++ = keys [k].y + 1 ;
        *kpt++ = keys [k].x + 1 ;
        vl_dft_transpose_descriptor(tmpdesc, descs + 128 * k) ;
        for (i = 0 ; i < 128 ; ++i) {
          *dpt++ = (vl_uint8) (VL_MIN(512.0f * tmpdesc[i], 256.0f)) ;
        }
      }
    }
    
    vl_dft_delete (dft) ;
  }
}
