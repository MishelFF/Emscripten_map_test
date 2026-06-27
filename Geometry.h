
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GEOMETRY_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GEOMETRY_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#include "gpc.h"
#ifdef __cplusplus
extern "C"{
#endif 

#include <float.h>
//#define GEOMETRY_API __declspec(dllexport)
#define GEOMETRY_API 

#define true  1
#define false 0

#define FLOATTYPE float
#define real float
#define FLTYPE_MAX FLT_MAX
#define CREPS 1.0

#define MEM_MAGIC 0x15031957l
#define MEM_TAIL  0x12061981l
#define SAA_SHITMAGIC 322223

#define z(i,j)   z[(j)][bignx-1-((i)+dnx)]



//extern void *w_malloc(long size);
//extern void  w_free(void *q);
//extern int   w_mtest(void);
//extern int   w_merr(void);
//int locatewells(float *x,int nx,float *y,int ny,
//                   real **z,
//                   int  bignx,
//                   int  dnx,
//                   double value,float *wx,float *wy,float *wval,int nw,TShitStruct *pSS);

//int fixline(float *x,float *y,float z,int *n,int maxn,TShitStruct *pSS);
//int findpupsik(float *x,float *y,float z,int *n,int maxn,TShitStruct *pSS);


GEOMETRY_API int __stdcall fnGeometry(void);

#ifdef __cplusplus
}
#endif 


