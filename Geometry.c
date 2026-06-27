// Geometry.cpp : Defines the entry point for the DLL application.
//
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <memory.h>
#include <windows.h>

#include "stdafx.h"

#include "Geometry.h"
#include "gpc.h"

#ifdef __cplusplus
extern "C"{
#endif 

//__declspec(cpu_specific(Pentium_4))

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
		break;
    }
    return TRUE;
}

// This is an example of an exported function.
GEOMETRY_API int __stdcall fnGeometry(void)
{
	return 42;
}

GEOMETRY_API int __stdcall pointinpoly(gpc_vertex_list * pl,gpc_vertex * pt)
{
int i, j, c = 0;
  for (i = 0, j = pl->num_vertices-1; i < pl->num_vertices; j = i++) {
    if ((((pl->vertex[i].y<=pt->y) && (pt->y<pl->vertex[j].y)) ||
    ((pl->vertex[j].y<=pt->y) && (pt->y<pl->vertex[i].y))) &&
    (pt->x < (pl->vertex[j].x - pl->vertex[i].x) * (pt->y - pl->vertex[i].y)
	/ (pl->vertex[j].y - pl->vertex[i].y) + pl->vertex[i].x))
       c = !c;
  }
  return c;
}     
GEOMETRY_API int __stdcall pointinarea(gpc_polygon * pl,gpc_vertex * pt)
{
int i;
for(i=0;i<pl->num_contours;i++)
  if(!(pl->hole[i])) 
	  if (pointinpoly(&(pl->contour[i]),pt)) return 0;

for(i=0;i<pl->num_contours;i++)
  if((pl->hole[i])) 
	  if (pointinpoly(&(pl->contour[i]),pt)) return 1;

return 0;
};     

typedef void __stdcall TCallBack1(int pos);
typedef TCallBack1* TCallBack;

GEOMETRY_API int __stdcall gridinarea( int nx,int ny,int step,int xc,int yc,gpc_polygon * plg,int * Internal,TCallBack CallBack )
{
int i,ii,xi,yi,ind,pz,pzo,j,k, c = 0,n=0;
gpc_vertex pt;
double *buf;
double t;
gpc_vertex_list *pl;
  if ((nx<=0)||(ny<=0)) return 0;
  pzo=0;
  buf=malloc(10000*sizeof(double)); 
  for(yi=0;yi<ny;yi++){
	pt.y=yi*step+yc;
    n=0;
	for(k=0;k<plg->num_contours;k++){
      pl=&(plg->contour[k]);
	  for (i = 0, j = pl->num_vertices-1; i < pl->num_vertices; j = i++) 
        if (((pl->vertex[i].y<=pt.y) && (pt.y<pl->vertex[j].y)) ||
		    ((pl->vertex[j].y<=pt.y) && (pt.y<pl->vertex[i].y))) {
            buf[n++]=(pl->vertex[j].x - pl->vertex[i].x) * (pt.y - pl->vertex[i].y)
	            /(pl->vertex[j].y - pl->vertex[i].y) + pl->vertex[i].x;
		};           
	};
    for(i=0;i<n;i++)
      for(j=i+1;j<n;j++)
		  if(buf[j]<buf[i]){t=buf[i];buf[i]=buf[j];buf[j]=t;};
	i=0;
    c=0;
	for(xi=0;xi<nx;xi++){
      ind=(xi)*ny+(yi);
      pt.x=xi*step+xc;
	  while((i<n)&&(buf[i]<=pt.x)) {c=!c;i++;};
	  Internal[ind]=c;
    };
    pz=yi*100/(ny);
    if (CallBack && (pz!=pzo)){CallBack(pz);pzo=pz;}
  }; 
  free(buf);
  return 1;
}

GEOMETRY_API int __stdcall pointinarea2(gpc_polygon * pl,gpc_vertex * pt)
{
int i;
for(i=0;i<pl->num_contours;i++)
  if((pl->hole[i])) 
	  if (pointinpoly(&(pl->contour[i]),pt)) return 0;

for(i=0;i<pl->num_contours;i++)
  if(!(pl->hole[i])) 
	  if (pointinpoly(&(pl->contour[i]),pt)) return 1;

return 0;
} 
GEOMETRY_API int __stdcall pointinarea3(gpc_polygon * plg,gpc_vertex * pt)
{
int i, j,k, c = 0,n=0;
double *buf;
gpc_vertex_list *pl;
  buf=malloc(10000*sizeof(double)); 
  for(k=0;k<plg->num_contours;k++){
    pl=&(plg->contour[k]);
	for (i = 0, j = pl->num_vertices-1; i < pl->num_vertices; j = i++) 
      if (((pl->vertex[i].y<=pt->y) && (pt->y<pl->vertex[j].y)) ||
		  ((pl->vertex[j].y<=pt->y) && (pt->y<pl->vertex[i].y))) {
          buf[n++]=(pl->vertex[j].x - pl->vertex[i].x) * (pt->y - pl->vertex[i].y)
	            /(pl->vertex[j].y - pl->vertex[i].y) + pl->vertex[i].x;
      };           
  };
  for (i=0;i<n;i++)
    if (buf[i]<pt->x) c=!c;
  free(buf);
  return !c;
}     
    

GEOMETRY_API FLOATTYPE __stdcall polyarea(gpc_vertex_list * pl)
{
int i, i1, i2;
FLOATTYPE a=0.0;
  for (i = 0; i < pl->num_vertices;i++) {
	  i1=i-1;if (i1<0) i1=pl->num_vertices-1;
	  i2=i+1;if (i2>pl->num_vertices-1) i2=0;
	  a=a+(pl->vertex[i].x*(pl->vertex[i2].y-pl->vertex[i1].y))/2.0;//2 A(P) = sum_{i=0}^{n-1} ( x_i  (y_{i+1} - y_{i-1}) )
  }
  return a;
}     

GEOMETRY_API int __stdcall iscrosssegments(gpc_vertex *A,gpc_vertex *B,gpc_vertex *C,gpc_vertex *D,gpc_vertex *E)
{
	FLOATTYPE r,s,delta;
    int Res=FALSE;
	E->x=0.0; E->y=0.0;
    delta=(B->x-A->x)*(D->y-C->y)-(B->y-A->y)*(D->x-C->x);
    if (delta==0.0) return FALSE;
    r=((A->y-C->y)*(D->x-C->x)-(A->x-C->x)*(D->y-C->y))/delta;
    s=((A->y-C->y)*(B->x-A->x)-(A->x-C->x)*(B->y-A->y))/delta;
    Res=(r>=0.0) && (r<=1.0) && (s>=0.0) && (s<=1.0);
    E->x=A->x+r*(B->x-A->x);
    E->y=A->y+r*(B->y-A->y);
    return Res;
}
GEOMETRY_API int __stdcall iscrosslinesegment(gpc_vertex *A,gpc_vertex *B,gpc_vertex *C,gpc_vertex *D,gpc_vertex *E)
{
	FLOATTYPE r,s,delta;
    int Res=FALSE;
	E->x=0.0; E->y=0.0;
    delta=(B->x-A->x)*(D->y-C->y)-(B->y-A->y)*(D->x-C->x);
    if (delta==0.0) return FALSE;
    r=((A->y-C->y)*(D->x-C->x)-(A->x-C->x)*(D->y-C->y))/delta;
    s=((A->y-C->y)*(B->x-A->x)-(A->x-C->x)*(B->y-A->y))/delta;
    Res=(r>=0.0) && (r<=1.0);
    E->x=A->x+r*(B->x-A->x);
    E->y=A->y+r*(B->y-A->y);
    return Res;
}

GEOMETRY_API void __stdcall gpc_cutconturs(gpc_polygon *subject_polygon,gpc_polygon *clip_polygon,gpc_polygon *result_polygon)
{
int i,j,j1,k,l,m,fs,f,tvs;
gpc_polygon *TP;
gpc_vertex *TV;
gpc_vertex_list *Tl;

  tvs=10000; 
  for(i=0;i<subject_polygon->num_contours;i++)
	  if (tvs<subject_polygon->contour[i].num_vertices) tvs=subject_polygon->contour[i].num_vertices;
  TP=malloc(sizeof(gpc_polygon)); 
  TP->contour=malloc(1000*sizeof(gpc_vertex_list));
  TV=malloc(tvs*sizeof(gpc_vertex));
  m=0;
  for(i=0;i<subject_polygon->num_contours;i++){
      j=0;
	  while(j<subject_polygon->contour[i].num_vertices){
	    while((j<subject_polygon->contour[i].num_vertices)&&
			(!pointinarea(clip_polygon,&(subject_polygon->contour[i].vertex[j])))) 
			    j++;
		l=0;
		while((j<subject_polygon->contour[i].num_vertices)&&
			(pointinarea(clip_polygon,&(subject_polygon->contour[i].vertex[j])))) 
		{
	     TV[l]=subject_polygon->contour[i].vertex[j];
		 l++;
		 j++;
		}		
        if(l>0){
	      TP->contour[m].vertex=malloc(l*sizeof(gpc_vertex));
	      for(j1=0; j1<l ; j1++) TP->contour[m].vertex[j1]=TV[j1];
          TP->contour[m].num_vertices=l;
		  m++;  
		}
	  } 
    
  }
  result_polygon->num_contours=m;
  result_polygon->contour=malloc(m*sizeof(gpc_vertex_list));
  for(i=0;i<m;i++)
	result_polygon->contour[i]=TP->contour[i];
  free(TV);
  free(TP->contour);
  free(TP);
}
GEOMETRY_API void __stdcall gpc_cutconturs2(gpc_polygon *subject_polygon,gpc_polygon *clip_polygon,gpc_polygon *result_polygon)
{
int i,j,j1,k,l,m,fs,f,tvs,tps;
gpc_polygon *TP;
gpc_vertex *TV;
gpc_vertex_list *Tl;

tvs=10000;tps=2000; 
  for(i=0;i<subject_polygon->num_contours;i++)
	  if (tvs<subject_polygon->contour[i].num_vertices) tvs=subject_polygon->contour[i].num_vertices;
  TP=malloc(sizeof(gpc_polygon)); 
  TP->contour=malloc(tps*sizeof(gpc_vertex_list));
  TV=malloc(tvs*sizeof(gpc_vertex));
  m=0;
  for(i=0;i<subject_polygon->num_contours;i++){
      j=0;
	  while(j<subject_polygon->contour[i].num_vertices){
	    while((j<subject_polygon->contour[i].num_vertices)&&
			(!pointinarea2(clip_polygon,&(subject_polygon->contour[i].vertex[j])))) 
			    j++;
		l=0;
		while((j<subject_polygon->contour[i].num_vertices)&&
			(pointinarea2(clip_polygon,&(subject_polygon->contour[i].vertex[j])))) 
		{
	     TV[l]=subject_polygon->contour[i].vertex[j];
		 l++;
		 j++;
		}		
        if((l>0)&&(m<(tps-1))){
	      TP->contour[m].vertex=malloc(l*sizeof(gpc_vertex));
	      for(j1=0; j1<l ; j1++) TP->contour[m].vertex[j1]=TV[j1];
          TP->contour[m].num_vertices=l;
		  m++;  
		}
	  } 
    
  }
  result_polygon->num_contours=m;
  result_polygon->contour=malloc(m*sizeof(gpc_vertex_list));
  for(i=0;i<m;i++)
	result_polygon->contour[i]=TP->contour[i];
  free(TV);
  free(TP->contour);
  free(TP);
}

GEOMETRY_API void __stdcall gpc_freecutconturs(gpc_polygon *subject_polygon)
{
	int i,j;
	for(i=0;i<subject_polygon->num_contours;i++)
		free(subject_polygon->contour[i].vertex);
	free(subject_polygon->contour);
}

TCross *Crosses;
int numcrosses;

gpc_vertex_list *Segments;
int nSegments;
int mmsize=0;

void * mymalloc(int size)
{
    mmsize=mmsize+size;
//	return (void *)GlobalAlloc(GMEM_FIXED,size);
	return malloc(size);
}
void myfree(void *pt)
{
//    mmsize=mmsize-_msize(pt);
//	GlobalFree(pt);
	free(pt);
}


void findintersectionoflines (gpc_polygon *alllines )
{
   int i,j,n,m,cnt;
   gpc_vertex_list  *OneLine,*SecLine;
   gpc_vertex P1,P2,P3,P4,P0;

  numcrosses=0;
  Crosses=mymalloc(1000*sizeof(TCross));
  for (i=0;i<(alllines->num_contours);i++){
    OneLine=&(alllines->contour[i]);
    SecLine=&(alllines->contour[i]);
    j=i;
	for (n=0;n<(OneLine->num_vertices-3);n++) {
	  P1=OneLine->vertex[n];
      P2=OneLine->vertex[n+1];
      for(m=n+2;m<(SecLine->num_vertices-1);m++){
	    P3=SecLine->vertex[m];
	    P4=SecLine->vertex[m+1];
        if (((P2.x)==(P3.x)) && ((P2.y)==(P3.y))) continue;
	    if ((iscrosssegments (&P1,&P2,&P3,&P4,&P0))&&(numcrosses<1000)) {
          Crosses[numcrosses].line1=i;
	      Crosses[numcrosses].line2=j;
	      Crosses[numcrosses].n1=n;
	      Crosses[numcrosses].n2=m;
	      Crosses[numcrosses].cr=P0;
	      numcrosses++;
		}
	  }
	}
    for (j=i+1;j<alllines->num_contours;j++){
      SecLine=&(alllines->contour[j]);
  	  cnt=0;
      for (n=0;n<(OneLine->num_vertices-1);n++) {
    	P1=OneLine->vertex[n];
	    P2=OneLine->vertex[n+1];
	    for(m=0;m<(SecLine->num_vertices-1);m++){
	      P3=SecLine->vertex[m];
	      P4=SecLine->vertex[m+1];
	      if ((numcrosses<1000)&&iscrosssegments (&P1,&P2,&P3,&P4,&P0)) {
	        cnt++;
			Crosses[numcrosses].line1=i;
	        Crosses[numcrosses].line2=j;
	        Crosses[numcrosses].n1=n;
	        Crosses[numcrosses].n2=m;
	        Crosses[numcrosses].cr=P0;
	        numcrosses++;
		  }
		}
	  }
	  if (cnt>50) numcrosses-=cnt; 
    }
  }
}


int TestClosed(gpc_vertex_list *nLine)
{
return((fabs(nLine->vertex[0].x-nLine->vertex[nLine->num_vertices-1].x)<CREPS)&&
(fabs(nLine->vertex[0].y-nLine->vertex[nLine->num_vertices-1].y)<CREPS));
}
void AddSegmentTo(gpc_vertex_list *nLine,gpc_vertex_list *OneLine)
{
int i,n,m;
m=OneLine->num_vertices;
n=nLine->num_vertices;
for (i=0;i<m;i++) nLine->vertex[n+i]=OneLine->vertex[i];
nLine->num_vertices=n+m;
}

void Makesegments(gpc_polygon *alllines)
{
   int i,j,nn,k,k1,t,intr[300],nintr;
   gpc_vertex intpt[300],tpt,pt1,pt2,pt3;
   gpc_vertex_list  *OneLine;
   FLOATTYPE d1,d2;

//  intr=mymalloc(100*sizeof(int));
//  intpt=mymalloc(100*sizeof(gpc_vertex));
  Segments=mymalloc(1000*sizeof(gpc_vertex_list));
  nSegments=0;
  for (i=0;i<(alllines->num_contours);i++){
    OneLine=&(alllines->contour[i]);
    nintr=0;
    for (j=0;j<numcrosses;j++){
      if (Crosses[j].line1==i){intr[nintr]=Crosses[j].n1;
			      intpt[nintr]=Crosses[j].cr;nintr++;}
      if (Crosses[j].line2==i){intr[nintr]=Crosses[j].n2;
			      intpt[nintr]=Crosses[j].cr;nintr++;}
    };
    if ((nintr>0)&&(nintr<300)){
    for (j=0;j<nintr;j++)
	  for (k=j+1;k<nintr;k++){
    	if (intr[k]<intr[j]) {t=intr[j];intr[j]=intr[k];intr[k]=t;
			     tpt=intpt[j];intpt[j]=intpt[k];intpt[k]=tpt;continue;};
		if ((intr[k]==intr[j])){
			pt1=intpt[j];
			pt2=intpt[k];
			pt3=OneLine->vertex[intr[j]];
            d1=sqrt((pt1.x-pt3.x)*(pt1.x-pt3.x)+(pt1.y-pt3.y)*(pt1.y-pt3.y));
		    d2=sqrt((pt2.x-pt3.x)*(pt2.x-pt3.x)+(pt2.y-pt3.y)*(pt2.y-pt3.y));
		    if (d1>d2) {t=intr[j];intr[j]=intr[k];intr[k]=t;
			     tpt=intpt[j];intpt[j]=intpt[k];intpt[k]=tpt;};
		  
		};	
	  };
    for (j=0;j<nintr-1;j++){
      pt1=intpt[j];
      pt2=intpt[j+1];
      nn=intr[j+1]-intr[j];
      Segments[nSegments].vertex=mymalloc((nn+2)*sizeof(gpc_vertex));
      k1=0;
      if ((pt1.x!=OneLine->vertex[intr[j]+1].x)||(pt1.y!=OneLine->vertex[intr[j]+1].y)) {
	Segments[nSegments].vertex[k1]=pt1;k1++;};
      for(k=intr[j]+1;k<=intr[j+1];k++){
	Segments[nSegments].vertex[k1]=OneLine->vertex[k];
	k1++;
      }
/*      if ((k1==1)&&((pt1.x!=pt2.x)||(pt1.y!=pt2.y))) {
		  d1=sqrt((pt1.x-OneLine->vertex[intr[j]].x)*(pt1.x-OneLine->vertex[intr[j]].x)
			  +(pt1.y-OneLine->vertex[intr[j]].y)*(pt1.y-OneLine->vertex[intr[j]].y));
		  d2=sqrt((pt2.x-OneLine->vertex[intr[j]].x)*(pt2.x-OneLine->vertex[intr[j]].x)+
			  (pt2.y-OneLine->vertex[intr[j]].y)*(pt2.y-OneLine->vertex[intr[j]].y));
		  if (d1<=d2)
		    Segments[nSegments].vertex[1]=pt2;
		  else {
		    Segments[nSegments].vertex[0]=pt2;
        	Segments[nSegments].vertex[1]=pt1;
          }
		k1++;}
	    else*/
        if ((pt2.x!=OneLine->vertex[intr[j+1]].x)||(pt2.y!=OneLine->vertex[intr[j+1]].y)) {
	       Segments[nSegments].vertex[k1]=pt2;k1++;
		};
	if (k1>1) { 
      Segments[nSegments].num_vertices=k1;
      nSegments++;
	}
    }
    }
    else {
      if (TestClosed(OneLine))
      {
	nn=OneLine->num_vertices;
	Segments[nSegments].num_vertices=0;
	Segments[nSegments].vertex=mymalloc(nn*sizeof(gpc_vertex));
	AddSegmentTo(&(Segments[nSegments]),OneLine);
	nSegments++;
      }
    }
  }
//  myfree(intr);
//  myfree(intpt);

}


void AddSegmentFrom(gpc_vertex_list *nLine,gpc_vertex_list *OneLine)
{
int i,n,m;
m=OneLine->num_vertices;
n=nLine->num_vertices;
for (i=n-1;i>=0;i--) nLine->vertex[m+i]=nLine->vertex[i];
for (i=0;i<m;i++) nLine->vertex[i]=OneLine->vertex[i];
nLine->num_vertices=n+m;
}

void AddClosed(gpc_polygon *ClosedLines,gpc_vertex_list *OneLine)
{
int i,n,m;
n=ClosedLines->num_contours;
m=OneLine->num_vertices;
ClosedLines->contour[n].num_vertices=m;
ClosedLines->contour[n].vertex=mymalloc(m*sizeof(gpc_vertex));
for (i=0;i<m;i++) ClosedLines->contour[n].vertex[i]=OneLine->vertex[i];
ClosedLines->num_contours++;
}


gpc_polygon * MakeClosed()
{
int i,j,n,m,flag;
gpc_polygon *ClosedLines;
gpc_vertex_list *OneLine,*SecLine,*nLine;
int *usedSegments;


ClosedLines=mymalloc(sizeof(gpc_polygon));
ClosedLines->num_contours=0;
ClosedLines->contour=mymalloc(1000*sizeof(gpc_vertex_list));
usedSegments=mymalloc(1000*sizeof(int));
for(i=0;i<nSegments;i++) usedSegments[i]=0;
for(i=0;i<nSegments;i++){
  OneLine=&(Segments[i]);
  if(TestClosed(OneLine)){
     AddClosed(ClosedLines,OneLine);
     usedSegments[i]=1;
  }
}
nLine=mymalloc(sizeof(gpc_vertex_list));
nLine->vertex=mymalloc(50000*sizeof(gpc_vertex));
for(i=0;i<nSegments;i++) if (!usedSegments[i]){
  nLine->num_vertices=0;
  OneLine=&(Segments[i]);
  AddSegmentTo(nLine,OneLine);
  n=OneLine->num_vertices-1;
  do{
    flag=0;
    for(j=i+1;j<nSegments;j++) 
	  if (!usedSegments[j]){
        n=nLine->num_vertices-1;
        SecLine=&(Segments[j]);
        m=SecLine->num_vertices-1;
        if ((fabs((nLine->vertex[n].x)-(SecLine->vertex[0].x))<CREPS)&&
	    (fabs((nLine->vertex[n].y)-(SecLine->vertex[0].y))<CREPS)){
	      AddSegmentTo(nLine,SecLine);
	      usedSegments[j]=1;
	      flag=1;
		}
        if ((fabs(nLine->vertex[0].x-SecLine->vertex[m].x)<CREPS)&&
	    (fabs(nLine->vertex[0].y-SecLine->vertex[m].y)<CREPS)&&(!flag)){
          AddSegmentFrom(nLine,SecLine);
	      usedSegments[j]=1;
	      flag=1;
		}
        if (flag) break;
	  }
    if (TestClosed(nLine)) {
      AddClosed(ClosedLines,nLine);
	  break;
    };
  } while(flag);
}
myfree(nLine->vertex);
myfree(nLine);
myfree(usedSegments);
myfree(Crosses);
for(i=0;i<nSegments;i++) 
  myfree(Segments[i].vertex);
myfree(Segments);
return ClosedLines;
}

gpc_polygon * MakeClosed1(gpc_polygon * NotClosed)
{
int i,j,n,m,flag,cflag;
gpc_polygon *ClosedLines;
gpc_vertex_list *OneLine,*SecLine,*nLine;
int *usedSegments;


ClosedLines=mymalloc(sizeof(gpc_polygon));
ClosedLines->num_contours=0;
ClosedLines->contour=mymalloc(1000*sizeof(gpc_vertex_list));
NotClosed->num_contours=0;
NotClosed->contour=mymalloc(1000*sizeof(gpc_vertex_list));
usedSegments=mymalloc(1000*sizeof(int));
for(i=0;i<nSegments;i++) usedSegments[i]=0;
for(i=0;i<nSegments;i++){
  OneLine=&(Segments[i]);
  if(TestClosed(OneLine)){
     AddClosed(ClosedLines,OneLine);
     usedSegments[i]=1;
  }
}
nLine=mymalloc(sizeof(gpc_vertex_list));
nLine->vertex=mymalloc(50000*sizeof(gpc_vertex));
for(i=0;i<nSegments;i++) if (!usedSegments[i]){
  nLine->num_vertices=0;
  OneLine=&(Segments[i]);
  AddSegmentTo(nLine,OneLine);
  n=OneLine->num_vertices-1;
  cflag=0;
  do{
    flag=0;
    for(j=i+1;j<nSegments;j++) if (!usedSegments[j]){
      n=nLine->num_vertices-1;
      SecLine=&(Segments[j]);
      m=SecLine->num_vertices-1;
      if ((fabs((nLine->vertex[n].x)-(SecLine->vertex[0].x))<CREPS)&&
	  (fabs((nLine->vertex[n].y)-(SecLine->vertex[0].y))<CREPS)){
	AddSegmentTo(nLine,SecLine);
	usedSegments[j]=1;
	flag=1;
      }
     
      if ((fabs(nLine->vertex[0].x-SecLine->vertex[m].x)<CREPS)&&
	  (fabs(nLine->vertex[0].y-SecLine->vertex[m].y)<CREPS)&&(!flag)){
	AddSegmentFrom(nLine,SecLine);
	usedSegments[j]=1;
	flag=1;
      }
      if (flag) break;
    }
    if (TestClosed(nLine)) {
      AddClosed(ClosedLines,nLine);
      cflag=1;
      break;
    };
  } while(flag);
  if (!cflag) AddClosed(NotClosed,nLine);
}
myfree(nLine->vertex);
myfree(nLine);
myfree(usedSegments);
myfree(Crosses);
for(i=0;i<nSegments;i++) 
  myfree(Segments[i].vertex);
myfree(Segments);
return ClosedLines;
}

GEOMETRY_API gpc_polygon * __stdcall makezone(gpc_polygon *cnt)
{
gpc_polygon *CLines;

/*FILE *ff;
int ss,tt,i,j;
FLOATTYPE vx,vy;
gpc_polygon *CLines;

cnt=mymalloc(sizeof(gpc_polygon));
ff=fopen("tt.dat","r+t");
if (!ff) return;
fscanf(ff,"%d\n",&ss);
cnt->num_contours=ss;
cnt->hole=mymalloc(ss*sizeof(int));
cnt->contour=mymalloc(ss*sizeof(gpc_vertex_list));
for (i=0;i<cnt->num_contours;i++){
  fscanf(ff,"%d %d\n",&ss,&tt);
  cl=&(cnt->contour[i]);
  cl->num_vertices=ss;
  cl->vertex=mymalloc(ss*sizeof(gpc_vertex));
  for(j=0;j<cl->num_vertices;j++){
    fscanf(ff,"%f %f\n",&vx,&vy);
    cl->vertex[j].x=vx;
    cl->vertex[j].y=vy;
  }
}
fclose(ff);
*/
//_set_new_mode(1//);
findintersectionoflines(cnt);
Makesegments(cnt);
CLines=MakeClosed();
//while (1) 
//  mymalloc(10000*sizeof(float)); 
return(CLines);
}

GEOMETRY_API void __stdcall freezone(gpc_polygon *cnt)
{
int i;
for (i=0;i<cnt->num_contours;i++){
  myfree(cnt->contour[i].vertex);
}
myfree(cnt->contour);
myfree(cnt);
cnt=NULL;
}



GEOMETRY_API gpc_polygon * __stdcall makezone1(gpc_polygon *cnt,gpc_polygon *nu)
{
gpc_polygon *CLines;

findintersectionoflines(cnt);
Makesegments(cnt);
CLines=MakeClosed1(nu);
return(CLines);
}

GEOMETRY_API void __stdcall freezone1(gpc_polygon *cnt,gpc_polygon *nu)
{
int i;
for (i=0;i<cnt->num_contours;i++){
  myfree(cnt->contour[i].vertex);
}
myfree(cnt->contour);
myfree(cnt);
cnt=NULL;
for (i=0;i<nu->num_contours;i++){
  myfree(nu->contour[i].vertex);
}
myfree(nu->contour);
nu=NULL;
}





/////////////////////////////////////
//Isolines
/////////////////////////////////////




typedef long ID_t;

#define get_ID() glob_ID++


typedef struct _mem_rec mem_rec;
typedef struct _mem_def mem_def;

struct _mem_def {
  mem_rec  *next,*last;
  ID_t      ID;
  long      len;
  long      magic;
};

struct _mem_rec {
  mem_def   md;
  char      mem[1000];
};



typedef struct {
  double x,y;     // Well coord
  double wval;    // Well data
  int    i,j;     // Grid cell where the well is located
  double gval[4]; // Grid values z(i,j),z(i,j+1),z(i+1,j+1),z(i+1,j)
  double gmax,gmin;
} TOneCellStruct;

typedef struct {
  int             magic;
  TOneCellStruct *cell;
  int             ncell;
  float          *xx;
  float          *yy;
  int             nx,ny;
} TShitStruct;


mem_rec *glob_MEM=NULL;
long need_ID=0;
long glob_ID=1;
static long buf=1;



#define mwr(i,j) mwr[nx*(j)+i]


void *w_malloc(long size) {
  mem_rec *p=malloc(sizeof(mem_def)+size+sizeof(long));
  //mem_rec *p=(mem_rec*)VirtualAlloc(NULL,sizeof(mem_def)+size+sizeof(long),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
  if(!p) return NULL;
  p->md.last=NULL; p->md.next=glob_MEM; glob_MEM=p;
  if(p->md.next) p->md.next->md.last=p;
  p->md.magic = MEM_MAGIC;
  p->md.ID    = get_ID();
  p->md.len   = size;
  if(p->md.ID==need_ID) {
    buf=0;
  }
  if(size==1056) {
    buf=0;
  }
  *( (long*) ( ((char*)p)+sizeof(mem_def)+size ) ) = MEM_TAIL;
#ifdef __SAA_MEM_DEBUG__
  w_mtest();
#endif
  return(((char*)p)+sizeof(mem_def));
} /* w_malloc */

void w_free(void *q) {
  mem_rec *p=(mem_rec *)(((char*)q)-sizeof(mem_def));
  long    *t=(long*)( ((char*)p)+sizeof(mem_def)+p->md.len);
  if(p->md.ID==need_ID) {
    buf=0;
  }
  if(p->md.magic != MEM_MAGIC || *t != MEM_TAIL) {
#ifdef __SAA_USE_WIN__
    MessageBox(NULL,"Bad Magic","Memory Error",MB_OK);
#else
    printf("\nSAA ERROR: memory free error, head=%08X, tail=%08X\n",p->md.magic,*t);
#endif
    return;
  }
  if(!glob_MEM) {
#ifdef __SAA_USE_WIN__
    MessageBox(NULL,"No Blocks in the List","Memory Error",MB_OK);
#else
    printf("\nSAA ERROR: no blocks in list . . .\n");
#endif
    return;
  }
  if(!(p->md.next)&&(!p->md.last)) { glob_MEM=NULL; }
  else {
    if(p->md.next != NULL) p->md.next->md.last=p->md.last;
    if(p->md.last != NULL) p->md.last->md.next=p->md.next;
    if(glob_MEM == p) glob_MEM=p->md.next;
  }
  p->md.magic=(-1);
  free(p);
  //VirtualFree(p,0,MEM_RELEASE);
#ifdef __SAA_MEM_DEBUG__
  w_mtest();
#endif
} /* w_free */

int w_mtest(void) {
  mem_rec *p=glob_MEM;
  long    *t;
  while(p) {
    t=(long*)( ((char*)p)+sizeof(mem_def)+p->md.len);
    if(p->md.magic != MEM_MAGIC || *t != MEM_TAIL) {
#ifdef __SAA_USE_WIN__
      MessageBox(NULL,"Bad Magic","Memory Error",MB_OK);
#else
      printf("\nSAA ERROR: memory free error, head=%08X, tail=%08X\n",p->md.magic,*t);
#endif
      return 0;
    }
    p=p->md.next;
  }
  return 1;
} /* w_mtest */

int w_merr(void) {
  return 1;
} /* w_merr */



/*int maxx(int a,int b)
{ return (a > b)? a:b;}
int minx(int a,int b)
{ return (a < b)? a:b;}
*/

FLOATTYPE perim(FLOATTYPE *x,FLOATTYPE *y, int ni)
{
 int i;
 FLOATTYPE buf,dx,dy;

  buf=0;
  for( i=1;i<ni;i++) {
    dx=x[i]-x[i-1];
    dy=y[i]-y[i-1];
    buf=buf+sqrt(dx*dx+dy*dy);
  }
  return buf;
}

int removeshortsegments(FLOATTYPE *x,FLOATTYPE *y,int *signat,int ni,int *no,FLOATTYPE eps)
{
  int i;
  FLOATTYPE d;

  *no = 1;
  for (i=1;i<ni;i++) {
    d=fabsf(x[i]-x[*no-1]);
    if(d < abs(y[i]-y[*no-1])) d=abs(y[i]-y[*no-1]);
    if(d > eps) {
      *no=*no+1;
      x[*no-1] = x[i];
      y[*no-1] = y[i];
	  signat[*no-1] = signat[i];
    }
	else if(i==(ni-1)) {
      x[*no-1] = x[ni-1];
      y[*no-1] = y[ni-1];
	  signat[*no-1] = signat[ni-1];
    };
  };
  return 1;
}

int changelinedirection(FLOATTYPE *x,FLOATTYPE *y,int n)
{
  FLOATTYPE temp;
  int i;

  for(i=1;i<=((int)(n/2));i++) {
      temp   = x[i-1];
      x[i-1] = x[n-i];
      x[n-i] = temp;
      temp   = y[i-1];
      y[i-1] = y[n-i];
      y[n-i] = temp;
  };
  return 1;
}


int findcross(FLOATTYPE *x,int nx,FLOATTYPE *y,int ny,
      FLOATTYPE **z,FLOATTYPE value,char *mwr,FLOATTYPE *xw,FLOATTYPE *yw,
              int *ic,int *jc,int *side,
              int  bignx,
              int  dnx,
              int *ier) {
      int    side2,i;
      FLOATTYPE  zsd,coef;
      int    ishift[4]={ 0, 1, 0,-1   };
      int    jshift[4]={-1, 0, 1, 0   };
      int    kx[5]    ={ 0, 1, 1, 0, 0};
      int    ky[5]    ={ 0, 0, 1, 1, 0};

      *ier = 0;
      if(mwr(*ic,*jc) == 0x0f) goto l10;

      if(!( mwr(*ic,*jc)&(1<<(*side)) )) goto l9001;

      for(i=0;i<4;i++) {
        if(i == *side) continue;
        side2 = i;
        if(mwr(*ic,*jc)&(1<<i)) goto l20;
      }
      goto l9001;

l10:  zsd = z(*ic,*jc)*z(*ic+1,*jc+1)-z(*ic+1,*jc)*z(*ic,*jc+1);
      zsd = zsd/(z(*ic,*jc)+z(*ic+1,*jc+1)-z(*ic+1,*jc)-z(*ic,*jc+1));
      if((value <= zsd) == (z(*ic+kx[*side],*jc+ky[*side]) <= zsd)) {
        side2 = (*side+3) % 4;
      } else {
        side2 = (*side+1) % 4;
      }

l20:  mwr(*ic,*jc)&=(char)(~(1<<(*side)));
      mwr(*ic,*jc)&=(char)(~(1<< side2 ));
      *side = side2;
      coef = (z(*ic+kx[*side+1],*jc+ky[*side+1])-z(*ic+kx[*side],*jc+ky[*side]));
      coef = (value-z(*ic+kx[*side],*jc+ky[*side]))/coef;
      *xw = x[*ic+kx[*side]]+ coef*(x[*ic+kx[*side+1]]-x[*ic+kx[*side]]);
      *yw = y[*jc+ky[*side]]+ coef*(y[*jc+ky[*side+1]]-y[*jc+ky[*side]]);
      *ic += ishift[*side];
      *jc += jshift[*side];
      *side = (*side+2) % 4;
      if((*ic < 0) || (*ic >= nx-1)) goto l9002;
      if((*jc < 0) || (*jc >= ny-1)) goto l9002;
      goto l9000;

l9001:*ier = 1;
      goto l9000;
l9002:*ier = 2;
      goto l9000;
l9000:return true;
}


int findcross2(FLOATTYPE* x, int nx, FLOATTYPE* y, int ny,
    FLOATTYPE** z,
    FLOATTYPE* value, int nv, char* mwr, FLOATTYPE* xw, FLOATTYPE* yw, int* signat, int* iw,
    int* ic, int* jc, int* side,
    int  bignx,
    int  dnx,
    int* ier,int npart)
{
      int    side2,i,j;
      double zsd,coef,z1,z2,z3,z4;
	  double xt,yt,a,b,c,d,x1,y1,x2,y2,xs,ys,xx,yy;
	  int ng,k;
      char   dummy1;
      int    ishift[4]={ 0, 1, 0,-1   };
      int    jshift[4]={-1, 0, 1, 0   };
      int    kx[5]    ={ 0, 1, 1, 0, 0};
      int    ky[5]    ={ 0, 0, 1, 1, 0};
      char   mask[2]  ={0x0f, 0xf0};

      *ier = 0;
      j = 4*nv;

      if(!(mwr(*ic,*jc)&(1<<(*side+j)))) goto l9001;

      dummy1 = mwr(*ic,*jc) & mask[nv];
      if(dummy1 == 0       ) goto l9001;
      if(dummy1 == mask[nv]) goto l10;

      for(i=0;i<4;i++) {
        if(i == *side) continue;
        side2 = i;
        if(mwr(*ic,*jc)&(1<<(i+j))) goto l20;
      }
      goto l9001;

/* Two isoline exits */

l10:  z1 = z(*ic  ,*jc  );
      z2 = z(*ic+1,*jc+1);
      z3 = z(*ic+1,*jc  );
      z4 = z(*ic  ,*jc+1);
      zsd = (z1*z2-z3*z4)/(z1+z2-z3-z4);
      if((value[nv] <= zsd) == (z(*ic+kx[*side],*jc+ky[*side]) <= zsd)) {
        side2 = (*side+3) % 4;
      } else {
        side2 = (*side+1) % 4;
      }

/* Isoline exits cell thrue side2 */

l20:  mwr(*ic,*jc)&=(char)(~(1<<(*side +j)));
      mwr(*ic,*jc)&=(char)(~(1<<( side2+j)));
      *side = side2;
      z1 = z(*ic+kx[*side],*jc+ky[*side]);
      z2 = z(*ic+kx[*side+1],*jc+ky[*side+1]);
      coef = (value[nv]-z1)/(z2-z1);
	  xt = x[*ic+kx[*side]]+ coef*(x[*ic+kx[*side+1]]-x[*ic+kx[*side]]);
      yt = y[*jc+ky[*side]]+ coef*(y[*jc+ky[*side+1]]-y[*jc+ky[*side]]);
	  if (npart){ 
		  if ((fabs(xt-xw[*iw])>0.001)&&(fabs(yt-yw[*iw])>0.001)) {
            ng=npart;
            x1=xw[*iw];y1=yw[*iw];x2=xt;y2=yt;
            xs=(x2-x1)/ng;
            ys=(y2-y1)/ng;
            d=z(*ic,*jc);
            a=(z(*ic+1,*jc)-z(*ic,*jc))/(x[*ic+1]-x[*ic]);
            b=(z(*ic,*jc+1)-z(*ic,*jc))/(y[*jc+1]-y[*jc]);
            c=(z(*ic+1,*jc+1)-z(*ic,*jc)-a*(x[*ic+1]-x[*ic])-b*(y[*jc+1]-y[*jc]))
            /((x[*ic+1]-x[*ic])*(y[*jc+1]-y[*jc]));
			if (fabs(xt-xw[*iw])>fabs(yt-yw[*iw])){
			  for(k=1;k<(ng-1);k++){
                xx=x1+k*xs;
                yy=(b+c*(xx-x[*ic]));
				if (yy!=0){  
                  yy=y[*jc]+(value[nv]-d-a*(xx-x[*ic]))/yy;
                 
				  xw[*iw+1]=xx;
                  yw[*iw+1]=yy;
                  signat[*iw + 1] = nv;
				    (*iw)++;
				}
			  }
			}
			else {
			  for(k=1;k<(ng-1);k++){
                yy=y1+k*ys;
                xx=(a+c*(yy-y[*jc]));
				if (xx!=0) {
                  xx=x[*ic]+(value[nv]-d-b*(yy-y[*jc]))/xx;
                  xw[*iw+1]=xx;
                  yw[*iw+1]=yy;
                  signat[*iw + 1] = nv;
                  (*iw)++;
				}    
			  }
			}
		  }
	  }
      xw[*iw+1]=xt;
      yw[*iw+1]=yt;
	  
	//  xw[*iw+1] = x[*ic+kx[*side]]+ coef*(x[*ic+kx[*side+1]]-x[*ic+kx[*side]]);
    //  yw[*iw+1] = y[*jc+ky[*side]]+ coef*(y[*jc+ky[*side+1]]-y[*jc+ky[*side]]);
      signat[*iw+1]=nv; 
      *ic = *ic + ishift[*side];
      *jc = *jc + jshift[*side];
      *side = (*side+2) % 4;
      if((*ic < 0) || (*ic >= nx-1)) goto l9002;
      if((*jc < 0) || (*jc >= ny-1)) goto l9002;
      goto l9000;

l9001:*ier = 1;   /* Isoline not exits cell (border) */
      goto l9000;
l9002:*ier = 2;   /* Isoline enter border            */
      *signat=2; 
      goto l9000;
l9000:return true;
}

int getcontourline(float *x,int nx,float *y,int ny,FLOATTYPE **z,
                   char *mwr,FLOATTYPE value,FLOATTYPE *wx,FLOATTYPE *wy,int *signat, int *nw,
                   int nmax,
                   int  bignx,
                   int  dnx,
                   FLOATTYPE eps,FLOATTYPE epsl,
                   int *nc) 
{
      int    side,is,js,ic,jc,i,j,iw,sides,ier;
      FLOATTYPE  coef;
      int    ishift[4]={ 0, 1, 0,-1 };
      int    jshift[4]={-1, 0, 1, 0 };
      int    kx[5]    ={ 0, 1, 1, 0, 0 };
      int    ky[5]    ={ 0, 0, 1, 1, 0 };
      int    linedirection;
      static int icur,jcur;

      if(*nc == 0) {
        for(i=0;i<nx;i++) {
          for(j=0;j<ny;j++) {
            if     (z(i,j) >= value) mwr(i,j) = 1;
            else                     mwr(i,j) = 2;
          }
        }
        for(i=0;i<nx-1;i++) {
          for(j=0;j<ny-1;j++) {
            if(      mwr(i,j) == 2) {
              mwr(i,j) = 0;
              if(mwr(i+1,j) == 1) mwr(i,j)|=(1<<0);
              if(mwr(i,j+1) == 1) mwr(i,j)|=(1<<3);
            } else if(mwr(i,j) == 1) {
              mwr(i,j) = 0;
              if(mwr(i+1,j) == 2) mwr(i,j)|=(1<<0);
              if(mwr(i,j+1) == 2) mwr(i,j)|=(1<<3);
            }
          }
        }
        for(i=0;i<nx-1;i++) {
          if       (mwr(i,ny-1) == 2) {
            mwr(i,ny-1) = 0;
            if(mwr(i+1,ny-1) == 1) mwr(i,ny-1)|=(1<<0);
          } else if(mwr(i,ny-1) == 1) {
            mwr(i,ny-1) = 0;
            if(mwr(i+1,ny-1) == 2) mwr(i,ny-1)|=(1<<0);
          }
        }
        for(j=0;j<ny-1;j++) {
          if(mwr(nx-1,j) == 2) {
            mwr(nx-1,j) = 0;
            if(mwr(nx-1,j+1) == 1) mwr(nx-1,j)|=(1<<3);
          } else if(mwr(nx-1,j) == 1) {
            mwr(nx-1,j) = 0;
            if(mwr(nx-1,j+1) == 2) mwr(nx-1,j)|=(1<<3);
          }
        }
        for(i=0;i<nx-1;i++) {
          for(j=0;j<ny-1;j++) {
            if(mwr(i+1,j)&(1<<3)) mwr(i,j)|=(1<<1);
            if(mwr(i,j+1)&(1<<0)) mwr(i,j)|=(1<<2);
          }
        }
        icur = 0;
        jcur = 0;
      }

test_cross:
      if(mwr(icur,jcur) == 0) goto try_next_cell;
      for(i=0;i<4;i++) {
        side = i;
        if(mwr(icur,jcur)&(1<<i)) {

/* High values have to be on left */

          if((linedirection = (z(icur+kx[side],jcur+ky[side]) >= value)) == 1) break;
        }
      }
      if(i == 4) goto try_next_cell;

/* Start new isoline from cross on side */

      iw = 0;
      is = icur; js = jcur; sides = side; /* Start isoline */
      ic = icur; jc = jcur;               /* Current cell  */
      linedirection = (z(ic+kx[side],jc+ky[side]) <= value);
      coef = (z(ic+kx[side+1],jc+ky[side+1])-z(ic+kx[side],jc+ky[side]));
      coef = (value-z(ic+kx[side],jc+ky[side]))/coef;
      wx[iw] = x[ic+kx[side]]+ coef*(x[ic+kx[side+1]]-x[ic+kx[side]]);
      wy[iw] = y[jc+ky[side]]+ coef*(y[jc+ky[side+1]]-y[jc+ky[side]]);

/* Look for next cross */

l20:  findcross(x,nx,y,ny,z,value,mwr,wx+iw+1,wy+iw+1,&ic,&jc,&side,
                bignx,
                dnx,
                &ier);
      if(ier == 1) goto l30;       /* Cross grid border */
      iw = iw+1;
      if(iw >= nmax-1) goto l9001; /* No more room for isoline */
      if(ier == 2) goto l30;       /* Cross grid border */
      goto l20;

l30:  if(is == -1) goto l50; /* Isoline ready */

/* Start isoline traweling from isoline start in opposite direction */

      ic = is+ishift[sides];
      jc = js+jshift[sides];
      is = -1;
      js = -1;
      if((ic == -1) || (jc == -1) || (ic == nx-1) || (jc == ny-1)) goto l50;
      side = (sides+2) % 4;
      changelinedirection(wx,wy,iw+1);
      linedirection = !linedirection;
      goto l20;

/* Turn isoline in right direction */

l50:  if(!linedirection) changelinedirection(wx,wy,iw+1);
      removeshortsegments(wx,wy,signat,iw+1,&iw,eps);
      if(iw <= 1 || perim(wx,wy,iw) <= epsl) goto try_next_cell;
      *nw = iw;
      goto l9000;

try_next_cell:
      if(++icur == nx-1) { icur = 0; jcur++; }
      if(jcur == ny-1) goto l8999;
      goto test_cross;

l9001:*nc = -1;
      *nw = 0;
      return true;

l8999:*nw = 0;
l9000:(*nc)++;
      return true;
}





int getcontourarea(FLOATTYPE *x,int nx,FLOATTYPE *y,int ny,
                   FLOATTYPE **z,
                   FLOATTYPE lowvalue,FLOATTYPE highvalue,
                   char *mwr,FLOATTYPE *wx,FLOATTYPE *wy,int *signat,int *nw,int nwmax,
                   int *ibu,int *nu,int maxcomp,
                   int  bignx,
                   int  dnx,
                   FLOATTYPE eps,FLOATTYPE epsl,
                   int *ier,int npart) {
      int    side,is,js,ic,jc,icur,jcur,i,j,iw,
             sides,level,iws;
      double coef,z1,z2;
      FLOATTYPE   value[2];
      char   kx[5]={ 0, 1, 1, 0, 0};
      char   ky[5]={ 0, 0, 1, 1, 0};
      int    bndentry,clflag;

      value[0] = lowvalue;
      value[1] = highvalue;
      if(ibu) { ibu[0] = 0; }
      *nu = 1;

      for(i=0;i<nx;i++) {
        for(j=0;j<ny;j++) {
          mwr(i,j) = 0;
        }
      }

      for(j=0;j<ny;j++) {
        for(i=0;i<nx;i++) {
          if(z(i,j) >= lowvalue) mwr(i,j) = 1;
          else                   mwr(i,j) = 2;
        }
      }

      for(i=0;i<nx-1;i++) {
        for(j=0;j<ny-1;j++) {
          if(mwr(i,j) == 2) {
            mwr(i,j) = 0;
            if(mwr(i+1,j) == 1) mwr(i,j)|=(1<<0);
            if(mwr(i,j+1) == 1) mwr(i,j)|=(1<<3);
          } else {
            mwr(i,j) = 0;
            if(mwr(i+1,j) == 2) mwr(i,j)|=(1<<0);
            if(mwr(i,j+1) == 2) mwr(i,j)|=(1<<3);
          }
        }
      }

      for(i=0;i<nx-1;i++) {
        if(mwr(i,ny-1) == 2) {
          mwr(i,ny-1) = 0;
          if(mwr(i+1,ny-1) == 1) mwr(i,ny-1)|=(1<<0);
        } else {
          mwr(i,ny-1) = 0;
          if(mwr(i+1,ny-1) == 2) mwr(i,ny-1)|=(1<<0);
        }
      }
      for(j=0;j<ny-1;j++) {
        if(mwr(nx-1,j) == 2) {
          mwr(nx-1,j) = 0;
          if(mwr(nx-1,j+1) == 1) mwr(nx-1,j)|=(1<<3);
        } else {
          mwr(nx-1,j) = 0;
          if(mwr(nx-1,j+1) == 2) mwr(nx-1,j)|=(1<<3);
        }
      }
      for(i=0;i<nx-1;i++) {
        for(j=0;j<ny-1;j++) {
          if(mwr(i+1,j)&(1<<3)) mwr(i,j)|=(1<<1);
          if(mwr(i,j+1)&(1<<0)) mwr(i,j)|=(1<<2);
        }
      }

      for(i=0;i<nx;i++) {
        for(j=0;j<ny;j++) {
          if(z(i,j) > highvalue) mwr(i,j)|=(1<<5);
          else                   mwr(i,j)|=(1<<6);
        }
      }

      for(i=0;i<nx-1;i++) {
        for(j=0;j<ny-1;j++) {
          if(mwr(i,j)&(1<<6)) {
            mwr(i,j)&=(~(1<<6));
            if(mwr(i+1,j)&(1<<5)) mwr(i,j)|=(1<<4);
            if(mwr(i,j+1)&(1<<5)) mwr(i,j)|=(1<<7);
          } else {
            mwr(i,j)&=(~(1<<5));
            if(mwr(i+1,j)&(1<<6)) mwr(i,j)|=(1<<4);
            if(mwr(i,j+1)&(1<<6)) mwr(i,j)|=(1<<7);
          }
        }
      }

      for(i=0;i<nx-1;i++) {
        if(mwr(i,ny-1)&(1<<6)) {
          mwr(i,ny-1)&=(~(1<<6));
          if(mwr(i+1,ny-1)&(1<<5)) mwr(i,ny-1)|=(1<<4);
        } else {
          mwr(i,ny-1)&=(~(1<<5));
          if(mwr(i+1,ny-1)&(1<<6)) mwr(i,ny-1)|=(1<<4);
        }
      }
      for(j=0;j<ny-1;j++) {
        if(mwr(nx-1,j)&(1<<6)) {
          mwr(nx-1,j)&=(~(1<<6));
          if(mwr(nx-1,j+1)&(1<<5)) mwr(nx-1,j)|=(1<<7);
        } else {
          mwr(nx-1,j)&=(~(1<<5));
          if(mwr(nx-1,j+1)&(1<<6)) mwr(nx-1,j)|=(1<<7);
        }
      }
      for(i=0;i<nx-1;i++) {
        for(j=0;j<ny-1;j++) {
          if(mwr(i+1,j)&(1<<7)) mwr(i,j)|=(1<<5);
          if(mwr(i,j+1)&(1<<4)) mwr(i,j)|=(1<<6);
        }
      }

      icur = 0;
      jcur = 0;
      iws  = 0;
      bndentry = false;

l1:   if(mwr(icur,jcur) == 0) goto l500;
      for(i=0;i<8;i++) {
        if(mwr(icur,jcur)&(1<<i)) {
          side  = i % 4;
          level = i / 4;
          z1 = z(icur+kx[side],jcur+ky[side]);
          if((level == 0) && (z1 < lowvalue )) break;
          if((level == 1) && (z1 > highvalue)) break;
        }
      }
l10:  iw = iws;
      is = icur; js = jcur; sides = side;
      ic = icur; jc = jcur;
      clflag = true;
l11:  z2 = z(ic+kx[side+1],jc+ky[side+1]);
      coef = z2-z1;
      coef = (value[level]-z1)/coef;
      wx[iw] = x[ic+kx[side]]+ coef*(x[ic+kx[side+1]]-x[ic+kx[side]]);
      wy[iw] = y[jc+ky[side]]+ coef*(y[jc+ky[side+1]]-y[jc+ky[side]]);
      if (bndentry) 
	     signat[iw]=2;
      else
	     signat[iw]=level;
l20:  findcross2(x,nx,y,ny,z,&value[0],level,mwr,wx,wy,signat,&iw,&ic,&jc,&side,bignx,dnx,ier,npart);
      if(*ier == 1) goto l50;
  	  iw = iw+1;
      if(iw == nwmax) goto l9001;
      if(*ier == 2) goto l30;
      goto l20;

l30:  bndentry = true;
      clflag = false;
      if(ic <     0) goto l101;
      if(jc >= ny-1) goto l201;
      if(ic >= nx-1) goto l301;
      if(jc <     0) goto l401;

l101: ic = 0;
      side = 3;
l102: if       (mwr(ic,jc)&(1<<3)) {
        iw = iw+1;
        level = 0;
        z1 = z(ic+kx[side],jc+ky[side]);
        goto l11;
      } else if(mwr(ic,jc)&(1<<7)) {
        iw = iw+1;
        level = 1;
        z1 = z(ic+kx[side],jc+ky[side]);
        goto l11;
      } else if((is == ic) && (js == jc) && (sides == side)) {
        iw = iw+1;
        wx[iw] = wx[iws];
        wy[iw] = wy[iws];
		signat[iw]=level;
        goto l50;
      }
      jc = jc+1;
      if(jc < ny-1) goto l102;
      iw = iw+1;
      wx[iw] = x[0];
      wy[iw] = y[ny-1];
	  signat[iw]=2;
      ic = 0;

l201: jc = ny-2;
      side = 2;
l202: if       (mwr(ic,jc)&(1<<2)) {
        iw = iw+1;
        level = 0;
        z1 = z(ic+kx[side],jc+ky[side]);
        goto l11;
      } else if(mwr(ic,jc)&(1<<6)) {
        iw = iw+1;
        level = 1;
        z1 = z(ic+kx[side],jc+ky[side]);
        goto l11;
      } else if((is == ic) && (js == jc) && (sides == side)) {
        iw = iw+1;
        wx[iw] = wx[iws];
        wy[iw] = wy[iws];
		signat[iw]=level;
        goto l50;
      }
      ic = ic+1;
      if(ic < nx-1) goto l202;
      iw = iw+1;
      wx[iw] = x[nx-1];
      wy[iw] = y[ny-1];
	  signat[iw]=2;
      jc = ny-2;

l301: ic = nx-2;
      side = 1;
l302: if       (mwr(ic,jc)&(1<<1)) {
        iw = iw+1;
        level = 0;
        z1 = z(ic+kx[side],jc+ky[side]);
        goto l11;
      } else if(mwr(ic,jc)&(1<<5)) {
        iw = iw+1;
        level = 1;
        z1 = z(ic+kx[side],jc+ky[side]);
        goto l11;
      } else if((is == ic) && (js == jc) && (sides == side)) {
        iw = iw+1;
        wx[iw] = wx[iws];
        wy[iw] = wy[iws];
		signat[iw]=level;
        goto l50;
      }
      jc = jc-1;
      if(jc >= 0) goto l302;
      iw = iw+1;
      wx[iw] = x[nx-1];
      wy[iw] = y[0];
	  signat[iw]=2;
      ic = nx-2;

l401: jc = 0;
      side = 0;
l402: if(mwr(ic,jc)&(1<<0)) {
        iw = iw+1;
        level = 0;
        z1 = z(ic+kx[side],jc+ky[side]);
        goto l11;
      } else if(mwr(ic,jc)&(1<<4)) {
        iw = iw+1;
        level = 1;
        z1 = z(ic+kx[side],jc+ky[side]);
        goto l11;
      } else if((is == ic) && (js == jc) && (sides == side)) {
        iw = iw+1;
        wx[iw] = wx[iws];
        wy[iw] = wy[iws];
		signat[iw]=2;
        goto l50;
      }
      ic = ic-1;
      if(ic >= 0) goto l402;
      iw = iw+1;
      wx[iw] = x[0];
      wy[iw] = y[0];
	  signat[iw]=2;
      jc = 0;
      goto l101;

l50:  removeshortsegments(wx+iws,wy+iws,signat+iws,iw-iws+1,&i,eps);
      if(  (i <= 2)
         || (clflag && (perim(wx+iws,wy+iws,i) <= epsl))) goto l500;
      wx[iws+i-1] = wx[iws];
      wy[iws+i-1] = wy[iws];
      signat[iws+i-1] = signat[iws];
      
	  iws = iws+i;
      (*nu)++;
      if(*nu == maxcomp-1) goto l9003;
      if(ibu) {ibu[*nu-1] = iws; }

l500: if(++icur == nx-1) {
        icur = 0;
        if(++jcur == ny-1) goto l8999;
      }
      goto l1;

l9001:*ier = 1;
      *nw = 0;
      goto l9000;
l9003:*ier = 3;
      goto l8998;

l8999:*ier = 0;
l8998:*nw = iws;
      if(bndentry) goto l9000;
      if((z(0,0) < lowvalue) || (z(0,0) >= highvalue)) goto l9000;
      if(*nw+5 > nwmax-1) goto l9001;
       wx[*nw] = x[0];    wy[*nw] = y[0];signat[*nw]=2;    (*nw)++;
       wx[*nw] = x[0];    wy[*nw] = y[ny-1];signat[*nw]=2; (*nw)++;
       wx[*nw] = x[nx-1]; wy[*nw] = y[ny-1];signat[*nw]=2; (*nw)++;
       wx[*nw] = x[nx-1]; wy[*nw] = y[0];signat[*nw]=2;    (*nw)++;
       wx[*nw] = x[0];    wy[*nw] = y[0];signat[*nw]=2;    (*nw)++;
      (*nu)++;
      if(ibu) { ibu[*nu-1] = ibu[*nu-2]+5; }
l9000:return true;
}



typedef struct {
     int magic;
     FLOATTYPE Epsilon;
     FLOATTYPE MinPerim;
     char *mwr;
     int rc;
} TIsoStruct;
//typedef TIsoStruct* pIsoStr;
int SAA_ISOMAGIC=28101999;

GEOMETRY_API int __stdcall SAAIsolineInit(FLOATTYPE MinPerim,FLOATTYPE Epsilon,TIsoStruct *p)
{

	p->magic   =SAA_ISOMAGIC;
    p->Epsilon =Epsilon;
    p->MinPerim=MinPerim;
    p->mwr     =NULL;
    return 0;
};

GEOMETRY_API int __stdcall SAAIsolineRelease(TIsoStruct *p)
{
   if((p == NULL) || (p->magic != SAA_ISOMAGIC)) return -2;
      
    p->magic=0;
    if(p->mwr != NULL) myfree(p->mwr); p->mwr=NULL;
  return 0;
}  


GEOMETRY_API int __stdcall SAAIsoline1(FLOATTYPE **zz,FLOATTYPE *xx,FLOATTYPE *yy, int Nx, int Ny,
                     FLOATTYPE R_left,FLOATTYPE R_top,FLOATTYPE R_right,FLOATTYPE R_bottom,FLOATTYPE Z_min,
                     int MaxPoints,FLOATTYPE *x,FLOATTYPE *y,int * signat, int *nw, int *nc,
                     TIsoStruct *p)
{
 int i,nx1,ny1,nx2,ny2,ms;

    if(p->magic != SAA_ISOMAGIC) return -2;
    i=Nx-1; while((i>0) && (xx[i] >= R_left))   i--; if(i<0) i=0; nx1=i;
    i=0;    while((i<Nx-1) && (R_right > xx[i])) i++; if(i>Nx-1) i=Nx-1; nx2=i;
    i=Ny-1; while((i>0) && (yy[i] >= R_bottom)) i--; if(i<0) i=0;    ny1=i;
    i=0;   while((i<Ny-1)&&(R_top > yy[i])) i++; if(i>Ny-1) i=Ny-1; ny2=i;

	  if(nx1 >= nx2) { nx1=max(0,nx1-1); nx2=min(Nx-1,nx2+1);};
	  if(ny1 >= ny2) { ny1=max(0,ny1-1); ny2=min(Ny-1,ny2+1);};

	  if(*nc == 0) { ms=(int)(nx2-nx1+1)*(ny2-ny1+1); p->mwr=mymalloc(ms);}
	  else {if(p->mwr == NULL) return -2;};
    getcontourline(&(xx[nx1]),nx2-nx1+1,&(yy[ny1]),ny2-ny1+1,&(zz[ny1]),p->mwr,Z_min,x,y,signat,nw,
                   MaxPoints,Nx,nx1,p->Epsilon,p->MinPerim,nc);

    if(*nw == 0) {
      myfree(p->mwr);
      p->mwr=NULL;
    }
  
	if(*nc < 0) {*nc=0; return 1;}
	else return 0;

}

GEOMETRY_API int __stdcall SAAIsolineWithSmooth (FLOATTYPE **zz,FLOATTYPE *xx,FLOATTYPE *yy, int Nx, int Ny,
				FLOATTYPE R_left,FLOATTYPE R_top,FLOATTYPE R_right,FLOATTYPE R_bottom,
                     FLOATTYPE Z_min,FLOATTYPE Z_max,
                     int MaxPoly,int *ibu, int *nu, int MaxPoints,
					 FLOATTYPE *x,FLOATTYPE *y,int *signat, int *nw,
                     TIsoStruct *p,int npart)

{
int   i,ier,nx1,ny1,nx2,ny2;
 char *mwr;

    if(p->magic != SAA_ISOMAGIC) return -2;
    i=Nx-1; while((i>0) && (xx[i] >= R_left)) i--; if(i<0) i=0; nx1=i;
    i=0;    while((i<Nx-1)&&(R_right > xx[i])) i++; if(i>Nx-1) i=Nx-1; nx2=i;
    i=Ny-1; while((i>0)&&(yy[i] >= R_bottom))  i--; if(i<0) i=0;    ny1=i;
    i=0;    while((i<Ny-1)&&(R_top > yy[i])) i++; if(i>Ny-1) i=Ny-1; ny2=i;

    if(nx1 >= nx2) {nx1=max(0,nx1-1); nx2=min(Nx-1,nx2+1);}
    if(ny1 >= ny2) {ny1=max(0,ny1-1); ny2=min(Ny-1,ny2+1);}

    mwr=mymalloc((nx2-nx1+1)*(ny2-ny1+1));

    getcontourarea(&(xx[nx1]),nx2-nx1+1,&(yy[ny1]),ny2-ny1+1,&(zz[ny1]),Z_min,Z_max,mwr,x,y,signat,nw,
                   MaxPoints,ibu,nu,MaxPoly,Nx,nx1,p->Epsilon,p->MinPerim,&ier,npart);
    myfree(mwr);
	return 0;
}
GEOMETRY_API int __stdcall SAAIsoline(FLOATTYPE** zz, FLOATTYPE* xx, FLOATTYPE* yy, int Nx, int Ny,
    FLOATTYPE R_left, FLOATTYPE R_top, FLOATTYPE R_right, FLOATTYPE R_bottom,
    FLOATTYPE Z_min, FLOATTYPE Z_max,
    int MaxPoly, int* ibu, int* nu, int MaxPoints,
    FLOATTYPE* x, FLOATTYPE* y, int* signat, int* nw,
    TIsoStruct* p)

{
    SAAIsolineWithSmooth(zz, xx, yy, Nx, Ny, R_left, R_top, R_right, R_bottom, Z_min, Z_max, MaxPoly, ibu, nu, MaxPoints, x, y, signat, nw, p, 4);
}

//-------------------------------------------------------------------
int left(double x1,double y1,double xn,double yn,double xk,double yk) {
  return ((xk-xn)*(y1-yn)-(yk-yn)*(x1-xn)) > 0;
}
//-------------------------------------------------------------------
static int locateinterval(double t,float *x,int n) {
  int i,j,k;

  if(n == 1) {
    if(t < x[0]) return -1;
    else         return  0;
  }
  if(t < x[0]) return -1;
  if(t >= x[n-1]) {
   if(t == x[n-1]) return n-2;
   else            return n-1;
  }
  i = 0; j = n;
  while(1) {
    k = (i + j)/2;
    if(t < x[k]) j = k;
    else         i = k;
    if( j <= i+1 ) return i;
  }
}
//-------------------------------------------------------------------
static TOneCellStruct *locatecell(TShitStruct *pSS,int ci,int cj) {
  TOneCellStruct *p=pSS->cell;
  int i,j,k,n=pSS->ncell;

  if(ci < p[0  ].i) return NULL;
  if(ci > p[n-1].i) return NULL;
  i = 0; j = n;
  while(1) {
    k = (i + j)/2;
    if(ci < p[k].i) j = k;
    else            i = k;
    if(j <= i+1) break;
  }
  if(p[i].i != ci && i < n-1) i++; 
  for(k=i;p[k].i == ci && k < n;k++)
    if(p[k].j == cj) return p+k;
  return NULL;
}
//-------------------------------------------------------------------
static int __cdecl sort_cell( const void *a, const void *b) {
   TOneCellStruct *p1=(TOneCellStruct *)a;
   TOneCellStruct *p2=(TOneCellStruct *)b;

   if(p1->i < p2->i) return -1;
   if(p1->i > p2->i) return +1;
   if(p1->j < p2->j) return -1;
   if(p1->j > p2->j) return +1;
   return 0;
}
//-------------------------------------------------------------------
int locatewells(float *xx,int nx,float *yy,int ny,
               real **z,
               int  bignx,
               int  dnx,
               double value,float *wx,float *wy,float *wval,int nw,TShitStruct *pSS) {
  int             i,j,k,n;
  TOneCellStruct *p;

  p=pSS->cell;
  for(n=k=0;k<nw;k++) {
    j=locateinterval(wy[k],yy,ny);
    i=locateinterval(wx[k],xx,nx);
    if(i < 0 || i > nx-2 || j < 0 || j > ny-2) continue;
    p[n].x    = wx[k];
    p[n].y    = wy[k];
    p[n].wval = wval[k];
    p[n].i    = i;
    p[n].j    = j;
    p[n].gval[0]=z(i  ,j  );
    p[n].gval[1]=z(i+1,j  );
    p[n].gval[2]=z(i+1,j+1);
    p[n].gval[3]=z(i  ,j+1);
    p[n].gmin=p[n].gval[0];
    p[n].gmax=p[n].gval[0];
    for(i=1;i<4;i++) {
      if(p[n].gmax < p[n].gval[i]) p[n].gmax=p[n].gval[i];
      if(p[n].gmin > p[n].gval[i]) p[n].gmin=p[n].gval[i];
    }
    n++;
  }
  if(n <= 0) goto alles;
  qsort(p,n,sizeof(*p),sort_cell);
  pSS->ncell=n;
  for(k=n=0;k<pSS->ncell;k++) {
    for(j=1,i=k+1;i<pSS->ncell && (p[k].i == p[i].i && p[k].j == p[i].j);i++) {
      p[k].x   +=p[i].x;
      p[k].y   +=p[i].y;
      p[k].wval+=p[i].wval;
      j++;
    }
    if(j > 1) {
      p[k].x   /=j;
      p[k].y   /=j;
      p[k].wval/=j;
    }
    p[n]=p[k];
    n++;
  }
  pSS->ncell=n;
  for(k=0;k<pSS->ncell;k++) {
    double dx=0.1*(xx[p[k].j+1]-xx[p[k].j]);
    double dy=0.1*(yy[p[k].j+1]-yy[p[k].j]);
    if(p[k].x       - xx[p[k].i] < dx) p[k].x=xx[p[k].i  ] + dx;
    if(xx[p[k].i+1] - p[k].x     < dx) p[k].x=xx[p[k].i+1] - dx;
    if(p[k].y       - yy[p[k].j] < dy) p[k].y=yy[p[k].j  ] + dy;
    if(yy[p[k].j+1] - p[k].y     < dy) p[k].y=yy[p[k].j+1] - dy;
  }
  return 1;
alles:
  return 0;
}
//-------------------------------------------------------------------
int findpupsik(float *x,float *y,float z,int *n,int maxn,TShitStruct *pSS) {
  int    i,k,num=0,side1;
  float  xbuf[5],ybuf[5],coef;
  int    kx[5]={ 0, 1, 1, 0, 0};
  int    ky[5]={ 0, 0, 1, 1, 0};
  for(k=0;k<pSS->ncell && num+5 <= maxn;k++) {
    TOneCellStruct *p=pSS->cell+k;
    if((p->gmax < z && z < p->wval) || (p->wval < z && z < p->gmin)) {
      for(side1=i=0;i<=4;i++) {
        coef=fabs(z - p->gval[side1])/fabs(p->wval - p->gval[side1]);
        x[num]=pSS->xx[p->i+kx[side1]] + coef*(p->x - pSS->xx[p->i+kx[side1]]);
        y[num]=pSS->yy[p->j+ky[side1]] + coef*(p->y - pSS->yy[p->j+ky[side1]]);
        num++;
        side1=(side1+1)%4;
      }
    }
  }
  return 1;
}
//-------------------------------------------------------------------
int fixline(float *x,float *y,float z,int *n,int maxn,TShitStruct *pSS) {
  int    i,j,k,num=*n,nbuf,side1,side2,CCW;
  float  cx,cy,xbuf[5],ybuf[5],coef;
  int    kx[5]={ 0, 1, 1, 0, 0};
  int    ky[5]={ 0, 0, 1, 1, 0};
  TOneCellStruct *p;

  for(k=0;k<num-1;k++) {
    cx=(x[k]+x[k+1])/2;
    cy=(y[k]+y[k+1])/2;
    j=locateinterval(cy,pSS->yy,pSS->ny);
    i=locateinterval(cx,pSS->xx,pSS->nx);
    if(i < 0 || i > pSS->nx-2 || j < 0 || j > pSS->ny-2) continue;
    p=locatecell(pSS,i,j);
    if(p == NULL) continue;

    if(pSS->yy[j] < y[k] && y[k] < pSS->yy[j+1]) { // 1 or 3
      side1=1+2*(fabs(x[k]-pSS->xx[i]) < fabs(x[k]-pSS->xx[i+1]));
    } else {                                       // 0 or 2
      side1=2*(fabs(y[k]-pSS->yy[j]) > fabs(y[k]-pSS->yy[j+1]));
    }

    if(pSS->yy[j] < y[k+1] && y[k+1] < pSS->yy[j+1]) { // 1 or 3
      side2=1+2*(fabs(x[k+1]-pSS->xx[i]) < fabs(x[k+1]-pSS->xx[i+1]));
    } else {                                       // 0 or 2
      side2=2*(fabs(y[k+1]-pSS->yy[j]) > fabs(y[k+1]-pSS->yy[j+1]));
    }

    CCW=((p->gval[side1]-z)*(p->wval - z) < 0);

    if(CCW/*!left(p->x,p->y,x[k],y[k],x[k+1],y[k+1]) && p->wval > z*/) { // CCW
      nbuf=0;
      while(side1 != side2) {
        coef=fabs(z - p->gval[side1])/fabs(p->wval - p->gval[side1]);
        xbuf[nbuf]=pSS->xx[i+kx[side1]] + coef*(p->x - pSS->xx[i+kx[side1]]);
        ybuf[nbuf]=pSS->yy[j+ky[side1]] + coef*(p->y - pSS->yy[j+ky[side1]]);
        nbuf++;
        side1=(side1+3)%4;
      }
    } else /*if(left(p->x,p->y,x[k],y[k],x[k+1],y[k+1]) && p->wval < z)*/ { // CW
      nbuf=0;
      while(side1 != side2) {
        side1=(side1+1)%4;
        coef=fabs(z - p->gval[side1])/fabs(p->wval - p->gval[side1]);
        xbuf[nbuf]=pSS->xx[i+kx[side1]] + coef*(p->x - pSS->xx[i+kx[side1]]);
        ybuf[nbuf]=pSS->yy[j+ky[side1]] + coef*(p->y - pSS->yy[j+ky[side1]]);
        nbuf++;
      }
    }
    if(num+nbuf < maxn) {
      memmove(x+k+1+nbuf,x+k+1,(num-k-1)*sizeof(x[0]));
      memmove(y+k+1+nbuf,y+k+1,(num-k-1)*sizeof(y[0]));
      memmove(x+k+1,xbuf,nbuf*sizeof(x[0]));
      memmove(y+k+1,ybuf,nbuf*sizeof(y[0]));
      num+=nbuf;
      k  +=nbuf;
    }
  }
  *n=num;
  return 1;
};


//---------------------------------------------------------------------------
GEOMETRY_API int __stdcall SAAFixShitInDataPrepare(
        float **zz,float *xx,float *yy,
        int Nx,int Ny,
        float R_left,float R_top,float R_right,float R_bottom,
        float Z_min,
        float *wx,float *wy,float *wval,int nw,TShitStruct *p) {
  int   i,rc,nx1,ny1,nx2,ny2;

//  try {
    for(i=Nx-1; i>0 && xx[i] >= R_left;) i--; if(i<0) i=0; nx1=i;
    for(i=0; i<Nx-1 && R_right > xx[i];) i++; if(i>Nx-1) i=Nx-1; nx2=i;
    for(i=Ny-1; i>0 && yy[i] >= R_bottom;) i--; if(i<0) i=0; ny1=i;
    for(i=0; i<Ny-1 && R_top > yy[i];) i++; if(i>Ny-1) i=Ny-1; ny2=i;

    if(nx1 >= nx2) { nx1=max(0,nx1-1); nx2=min(Nx-1,nx2+1); }
    if(ny1 >= ny2) { ny1=max(0,ny1-1); ny2=min(Ny-1,ny2+1); }

    p->magic=SAA_SHITMAGIC;
    p->cell =NULL;
    p->xx   =NULL;
    p->yy   =NULL;
    p->ncell=0;
    if(nw <= 0 || (p->cell=(TOneCellStruct*)w_malloc(nw*sizeof(TOneCellStruct))) == NULL) { rc=-1; goto alles; }
    if((p->nx=nx2-nx1+1) <= 1 || (p->xx=(float*)w_malloc(p->nx*sizeof(float))) == NULL)   { rc=-1; goto alles; }
    if((p->ny=ny2-ny1+1) <= 1 || (p->yy=(float*)w_malloc(p->ny*sizeof(float))) == NULL)   { rc=-1; goto alles; }
    memmove(p->xx,xx+nx1,p->nx*sizeof(float));
    memmove(p->yy,yy+ny1,p->ny*sizeof(float));
    if(!locatewells(xx+nx1,nx2-nx1+1,yy+ny1,ny2-ny1+1,zz+ny1,Nx,nx1,Z_min,
                    wx,wy,wval,nw,p)) { rc=-2; goto alles; }
    p->ncell=nw;
//  } catch (Exception &E) {
 //   Application->ShowException(&E);
//    rc=-3; goto alles;
//  }
  rc=0;
alles:
  return rc;
};
//---------------------------------------------------------------------------
GEOMETRY_API int __stdcall SAAFixShitInDataSegment(float *x,float *y,float z,int *n,int maxn,TShitStruct *p) {
  int rc=0;
//  try {
    if(!p || p->magic != SAA_SHITMAGIC || p->cell == NULL) { rc=-2; goto alles; }
    if(!fixline(x,y,z,n,maxn,p)) { rc=-1; goto alles; }
 // } catch (Exception &E) {
//    Application->ShowException(&E);
//    goto alles;
//  }
  return rc;
alles:
  return rc;
}
//---------------------------------------------------------------------------
GEOMETRY_API int __stdcall SAAFixShitInDataPupsik(float *x,float *y,float z,int *n,int maxn,TShitStruct *p) {
  int rc=0;
//  try {
    if(!p || p->magic != SAA_SHITMAGIC || p->cell == NULL) { rc=-2; goto alles; }
    if(!findpupsik(x,y,z,n,maxn,p)) { rc=-1; goto alles; }
//  } catch (Exception &E) {
//    Application->ShowException(&E);
//    goto alles;
//  }
  return rc;
alles:
  return rc;
}
//---------------------------------------------------------------------------
GEOMETRY_API int __stdcall SAAFixShitInDataRelease(TShitStruct *p) {
//  try {
    if(!p || p->magic != SAA_SHITMAGIC) goto alles;
    p->magic=0;
    if(p->cell) w_free(p->cell); p->cell=NULL;
    if(p->xx)   w_free(p->xx);   p->xx  =NULL;
    if(p->yy)   w_free(p->yy);   p->yy  =NULL;
//  } catch (Exception &E) {
//    Application->ShowException(&E);
//    goto alles;
//  }
  return 0;
alles:
  return -2;
}
//---------------------------------------------------------------------------









//-------------------------------------------------------------
//Curve Procs
//-------------------------------------------------------------
//-------------------------------------------------------------
FLOATTYPE frtvv(FLOATTYPE *v1x,FLOATTYPE *v1y,FLOATTYPE *v2x,FLOATTYPE *v2y) {
  FLOATTYPE dx=*v2x - *v1x, dy=*v2y - *v1y; return sqrt(dx*dx + dy*dy);
}

//-------------------------------------------------------------
int ptonline(FLOATTYPE *vx,FLOATTYPE *vy,FLOATTYPE *v1x,FLOATTYPE *v1y,FLOATTYPE
*v2x,FLOATTYPE *v2y,FLOATTYPE tol) {
  FLOATTYPE  rx1n,ry1n,rxkn,rykn,rx1k,ry1k,d;

  rx1n=*vx  - *v1x; ry1n=*vy  - *v1y;
  rx1k=*vx  - *v2x; ry1k=*vy  - *v2y;
  rxkn=*v2x - *v1x; rykn=*v2y - *v1y;

  if     (frtvv(v1x,v1y,v2x,v2y) < tol) d=frtvv(vx,vy,v1x,v1y);
  else if(rx1n*rxkn+ry1n*rykn <= 0)     d=sqrt(rx1n*rx1n+ry1n*ry1n);
  else if(rx1k*rxkn+ry1k*rykn >= 0)     d=sqrt(rx1k*rx1k+ry1k*ry1k);
  else
    d=fabs(rx1n*rykn-ry1n*rxkn)/sqrt(rxkn*rxkn+rykn*rykn);
  return d <= tol;
}

//-------------------------------------------------------------
int simplify_cont(gpc_vertex *b,int bufnum,FLOATTYPE tol,FLOATTYPE dist) {
    int i,j,k,m,in_band;
    FLOATTYPE d;

    if (bufnum > 3) {
        for (i = m = 0, j = 2; j < bufnum; j++) {
            in_band = 1; d = 0;
            for (k = m + 1;( k < j) && in_band && ((!dist) || (d <= dist)); k++) {
                in_band = ptonline(&b[k].x, &b[k].y, &b[i].x, &b[i].y, &b[j].x, &b[j].y, tol);
                d += hypot(b[k].x - b[k - 1].x, b[k].y - b[k - 1].y);
            }
            if (!in_band || ((dist > 0) && (d > dist))) {
                i++;
                b[i] = b[j - 1];
                m = j - 1;
            }
        }
        if (m < (bufnum - 1)) { i++;b[i] = b[bufnum - 1];  }
        return i + 1;
    }
    else return bufnum;
}

/*
  ÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛ*/

GEOMETRY_API void __stdcall simplify(gpc_vertex_list *c,FLOATTYPE tol)
{
   if (c->num_vertices>=3) c->num_vertices = simplify_cont(c->vertex,c->num_vertices,tol,0);
}
GEOMETRY_API void __stdcall simplifyd(gpc_vertex_list* c, FLOATTYPE tol, FLOATTYPE dist) {
    c->num_vertices = simplify_cont(c->vertex, c->num_vertices, tol, dist);
}
GEOMETRY_API void __stdcall simplify2(gpc_vertex_list* c, gpc_vertex_list* c2, FLOATTYPE tol, FLOATTYPE dist)
{
    int i;
    for (i = 0; i < c->num_vertices; i++) c2->vertex[i] = c->vertex[i];
    c2->num_vertices = simplify_cont(c2->vertex, c2->num_vertices, tol,dist);
}


double acubic [2][4]={ {1./8,6./8,1./8,0   },{0     ,4./8 ,4./8 ,0     } };
double icubic [2][4]={ {0   ,1   ,0   ,0   },{-1./16,9./16,9./16,-1./16} };
double aquad  [2][4]={ {0   ,3./4,1./4,0   },{0     ,1./4 ,3./4 ,0     } };

double kobbelt[3][3]={ {10./27,16./27, 1./27},
                       { 4./27,19./27, 4./27},
                       { 1./27,16./27,10./27}
                     };
#define PI    3.14159
#define FEPS  0.001


typedef struct curve1
{
 int    num;   // ç¨á«® ¢¥àè¨­
 int    close,needcorrect,needcompact,maxpoints; // ä« £ § ¬ª­ãâ®áâ¨
 FLOATTYPE dupdist;     // ¢ëá®â 
 FLOATTYPE len,original_len;   // ¤«¨­  ªà¨¢®©
 gpc_vertex *p;     // ¢¥àè¨­ë
 gpc_vertex* ap;     // ¢¥àè¨­ë
} curve;
double fsqr(double x) { return x * x; }
double curveLength(curve* c)
{
    double len = FEPS;
    int n;
    for (n = 1; n < c->num; n++)
        len += hypot(c->p[n - 1].x - c->p[n].x,c->p[n - 1].y - c->p[n].y);
    if (c->close)
        len += hypot(c->p[n - 1].x - c->p[0].x,c->p[n - 1].y - c->p[0].y);
    return len;
}
void normal(gpc_vertex* p0, gpc_vertex* p1, gpc_vertex* p2, double len, gpc_vertex* n)
{
    double a0, b0, a1, b1, c0, c1;
    a0 = -(p1->y - p0->y);
    b0 = (p1->x - p0->x);
    a1 = -(p2->y - p1->y);
    b1 = (p2->x - p1->x);
    if ((c0 = sqrt(b0 * b0 + a0 * a0)) < FEPS) a0 = b0 = 0; else a0 /= c0, b0 /= c0;
    if ((c1 = sqrt(b1 * b1 + a1 * a1)) < FEPS) a1 = b1 = 0; else a1 /= c1, b1 /= c1;
    a0 += a1;
    b0 += b1;
    if ((c0 = sqrt(b0 * b0 + a0 * a0)) < FEPS) n->x = n->y = 0; else n->x = a0 * len / c0, n->y = b0 * len / c0;
}
static void freecurves(curve* c)
{
    free(c->p - 1);
    free(c);
}




static curve* readcurves(gpc_vertex_list *cnt,FLOATTYPE hh,int cls,int iscorrect,int iscompact,int maxvrt)
{
    int num,i;
    curve *c;
    num= cnt->num_vertices;
    c=(curve *)malloc(sizeof(curve));
    c->ap=(gpc_vertex *)malloc(sizeof(gpc_vertex)*(num+3));
    c->p=c->ap+1;
    c->num=num;
    c->dupdist=hh;
    c->close=cls;
    c->needcorrect = iscorrect;
    c->needcompact = iscompact;
    c->maxpoints = maxvrt;
    for (i=0;i<num;i++) {
	    c->p[i].x=cnt->vertex[i].x;
        c->p[i].y=cnt->vertex[i].y;
    }
 //   memcpy(c->sp,c->p,sizeof(gpc_vertex)*num);
    c->len=curveLength(c);
    c->original_len = c->len;
    return c;
}
static int writecurves(gpc_vertex_list *cnt, curve *c)
{ 
    int i,n;
    n = c->num;
    if ((c->maxpoints)&&(n > c->maxpoints)) n = c->maxpoints;
    cnt->num_vertices=n;
    for (i=0;i<n;i++){
        cnt->vertex[i].x=c->p[i].x;
        cnt->vertex[i].y=c->p[i].y;
    }
    if ((c->close) &&(c->num>1)&&((c->p[0].x!=c->p[n-1].x)||(c->p[0].y != c->p[n - 1].y))) {
        cnt->num_vertices++;
        cnt->vertex[n].x = cnt->vertex[0].x;
        cnt->vertex[n].y = cnt->vertex[0].y;
    }
    return (!c->maxpoints) || (c->num <= c->maxpoints);
}

void deldups(curve* c)
{
int n,i,j=0;
FLOATTYPE h;
    do {
        h = c->dupdist*pow(2,j);
        for (n = 0, i = c->num - 1; n < c->num;)
            if (hypot(c->p[i].x - c->p[n].x, c->p[i].y - c->p[n].y) < h) {
                memcpy(c->p + n, c->p + n + 1, (c->num - (n + 1)) * sizeof(c->p[0]));
                c->num--;
            }
            else i = n++;
//        for (n = 0, i = c->num - 1; n < c->num;)
//            if (fsqr(c->p[i].x - c->p[n].x) + fsqr(c->p[i].y - c->p[n].y) < dupdist * dupdist)
//                memcpy(c->p + n, c->p + n + 1, (--c->num - n) * sizeof(c->p[0]));
//            else i = n++;

    } while ((c->needcompact)&&(c->num>c->maxpoints));
}



void cBoundary(curve* c)
{
    if (c->num>=3)
        if (!c->close)
        {
            c->p[-1].x      =c->p[0       ].x*2-c->p[1       ].x;
            c->p[-1].y      =c->p[0       ].y*2-c->p[1       ].y;
            c->p[c->num].x  =c->p[c->num-1].x*2-c->p[c->num-2].x;
            c->p[c->num].y  =c->p[c->num-1].y*2-c->p[c->num-2].y;
            c->p[c->num+1].x=c->p[c->num  ].x*2-c->p[c->num-1].x;
            c->p[c->num+1].y=c->p[c->num  ].y*2-c->p[c->num-1].y;
        }
        else{
            c->p[-1]      =c->p[c->num-1];
            c->p[c->num]  =c->p[0];
            c->p[c->num+1]=c->p[1];
        }
}

static void correctCurves(curve* c)
{
    int     n,i,j,n1;
    double  len;
    double  dr;
    gpc_vertex *nrm;
    gpc_vertex *np;
    if (c->needcorrect){
        if (c->num>=3){
            len=curveLength(c);
            for (i=0;i<4;i++){
                dr=(c->len-len)/(16*PI/*8*PI*/); if (dr<=0) break;
                nrm=(gpc_vertex*)malloc(c->num*sizeof(gpc_vertex));
                for (n = 0, np = nrm; n < c->num; n++, np++)
                    normal(&(c->p[n-1]),&(c->p[n]),&(c->p[n+1]),dr,&(np[0]));
                for (n = 0, np = nrm; n < c->num; n++, np++) { c->p[n].x += np->x; c->p[n].y += np->y; }
                if (curveLength(c)<len) // CCW
                    for (n = 0, np = nrm; n < c->num; n++, np++) {
                        c->p[n].x -= np->x * 2; c->p[n].y -= np->y * 2;
                    }
                free(nrm);
            }
        }
    }
}

static void subdiv3(curve* c,double *o0,double *e,double *o1)
{
    int  n,oldnum;
    gpc_vertex *p;
    gpc_vertex *f;
    gpc_vertex *oldp;
    cBoundary(c);
    if (c->num>=3) {
        oldp = c->ap;
        oldnum = c->num;
        p=(gpc_vertex*)malloc(sizeof(p[0])*(c->num*3+3));
        for (n=0,f=p+1;n<c->num;n++,f+=3)
        {
            f[0].x=c->p[n-1].x*o0 [0]+c->p[n].x*o0 [1]+c->p[n+1].x*o0 [2];
            f[0].y=c->p[n-1].y*o0 [0]+c->p[n].y*o0 [1]+c->p[n+1].y*o0 [2];
            f[1].x=c->p[n-1].x*e  [0]+c->p[n].x*e  [1]+c->p[n+1].x*e  [2];
            f[1].y=c->p[n-1].y*e  [0]+c->p[n].y*e  [1]+c->p[n+1].y*e  [2];
            f[2].x=c->p[n-1].x*o1 [0]+c->p[n].x*o1 [1]+c->p[n+1].x*o1 [2];
            f[2].y=c->p[n-1].y*o1 [0]+c->p[n].y*o1 [1]+c->p[n+1].y*o1 [2];
        }
        c->ap  = p;
        c->p   =p+1;
        c->num =c->num*3-(!c->close)*2;
        if (!c->close) memcpy(p,p+1,(c->num+1)*sizeof(gpc_vertex));
        cBoundary(c);
        deldups(c);
        if ((!c->needcompact) && (c->num > c->maxpoints)) {
            free(c->ap);
            c->num = oldnum;
            c->ap = oldp;
            c->p = oldp + 1;
        }
        else {
            free(oldp);
        }
        //        c->len =curveLength(c);
    }
    else{
       cBoundary(c);
       deldups(c);
    }
}

static void subdiv(curve* c,double *e,double *o)
{
    int  n, oldnum;
    gpc_vertex *p;
    gpc_vertex *f;
    gpc_vertex* oldp;
    cBoundary(c);
    if (c->num>=3){
        oldp = c->ap;
        oldnum = c->num;
        p=(gpc_vertex*)malloc(sizeof(p[0])*(c->num*2+3));
        for (n=0,f=p+1;n<c->num;n++,f+=2){
            f[0].x=c->p[n-1].x*e[0]+c->p[n].x*e[1]+c->p[n+1].x*e[2]+c->p[n+2].x*e[3];
            f[0].y=c->p[n-1].y*e[0]+c->p[n].y*e[1]+c->p[n+1].y*e[2]+c->p[n+2].y*e[3];
            f[1].x=c->p[n-1].x*o[0]+c->p[n].x*o[1]+c->p[n+1].x*o[2]+c->p[n+2].x*o[3];
            f[1].y=c->p[n-1].y*o[0]+c->p[n].y*o[1]+c->p[n+1].y*o[2]+c->p[n+2].y*o[3];
        }
        c->ap  = p;
        c->p   =p+1;
        c->num =c->num*2-!c->close;
        cBoundary(c);
        deldups(c);
        if ((!c->needcompact) && (c->num > c->maxpoints)) {
            free(c->ap);
            c->num = oldnum;
            c->ap = oldp;
            c->p = oldp + 1;
        }
        else {
            free(oldp);
        }
//        c->len =curveLength(c);
    }
    else {
        // nSubdiv++;
        cBoundary(c);
        deldups(c);
    }
}


static void filter3(curve* c, double mask[],double w)
{
    int  n;
    //gpc_vertex *p;
    gpc_vertex *f;
    cBoundary(c);
    if (c->num>=3){
        f=(gpc_vertex*)malloc(sizeof(f[0])*(c->num+2));
        memcpy(f,c->p-1,(c->num+2)*sizeof(f[0]));
        for (n=0;n<c->num;n++){
            c->p[n].x=(f[n+0].x*mask[0]+f[n+1].x*mask[1]+f[n+2].x*mask[2])*w;
            c->p[n].y=(f[n+0].y*mask[0]+f[n+1].y*mask[1]+f[n+2].y*mask[2])*w;
        }
        free(f);
    }
    cBoundary(c);
    correctCurves(c);
}

static void filter5(curve* c,double mask[],double w)
{
    int n;
    //gpc_vertex *p;
    gpc_vertex *f;
    cBoundary(c);
    if (c->num>=3){
        f=(gpc_vertex*)malloc(sizeof(f[0])*(c->num+4));
        memcpy(f+1,c->p-1,(c->num+2)*sizeof(f[0]));
        if (c->close){
            f[0]       =c->p[c->num-2];
            f[c->num+3]=c->p[1];
        }
        else{
            f[0].x=f[1].x*2-f[2].x;
            f[0].y=f[1].y*2-f[2].y;
            f[c->num+3].x=f[c->num+2].x*2-f[c->num+1].x;
            f[c->num+3].y=f[c->num+2].y*2-f[c->num+1].y;
        }
        for (n=0;n<c->num;n++){
            c->p[n].x=(f[n+0].x*mask[0]+
                f[n+1].x*mask[1]+
                f[n+2].x*mask[2]+
                f[n+3].x*mask[3]+
                f[n+4].x*mask[4])*w;
            c->p[n].y=(f[n+0].y*mask[0]+
                f[n+1].y*mask[1]+
                f[n+2].y*mask[2]+
                f[n+3].y*mask[3]+
                f[n+4].y*mask[4])*w;
        }
        free(f);
    }
    cBoundary(c);
    correctCurves(c);
    deldups(c);
}

void gauss3(curve* c)
{
    int    i;
    double x,mask[3],w,sigma=1.0/(0.85*0.85*2.0);
    for (w=0,i=0,x=-1;i<3;x++,i++) w+=mask[i]=exp(-x*x*sigma);
    filter3(c, mask,1/w);
}
void gauss5(curve* c)
{
    int    i;
    double x,mask[5],w,sigma=1.0/(0.85*0.85*2.0);
    for (w=0,i=0,x=-2;i<5;x++,i++) w+=mask[i]=exp(-x*x*sigma);
    filter5(c,mask,1/w);
}

void cosinebell5(curve* c)
{
    int    i;
    double x,mask[5],w,xm=2;
    for (w=0,i=0,x=-2;i<5;x++,i++) w+=mask[i]=1+cos(PI*fabs(x)/xm);
    filter5(c,mask,1/w);
}

void windowedSinc5(curve* c)
{
    int    i;
    double a,x,mask[5],w,dev=0.0001,xm=2+dev*2;
    for (w=0,i=0,x=-2+dev;i<5;x++,i++){
        a=fabs(x)/xm;
        w+=mask[i]=(1+cos(PI*a))*(sin(a)-a*cos(a))/(a*a*a);
    }
    filter5(c,mask,1/w);
}


void  acubicSubdiv(curve* c)
{
    subdiv(c,acubic[0],acubic[1]);
}

void icubicSubdiv(curve* c)
{
    subdiv(c,icubic[0],icubic[1]);
}

void aquadSubdiv(curve* c)
{
    subdiv(c,aquad[0],aquad[1]);
}

void kobbeltSubdiv(curve* c)
{
    subdiv3(c,kobbelt[0],kobbelt[1],kobbelt[2]);
}


GEOMETRY_API void __stdcall smoothcontx(gpc_vertex_list *cnt1,gpc_vertex_list *cnt2,FLOATTYPE hh,int cls,int kind,int iternum,int ccv, int maxvertex, int iscompact)
{

    curve* c;
    int itn;
    itn=abs(iternum);
    c=readcurves(cnt1,hh,cls,ccv, iscompact, maxvertex);
// delDups    ();
    switch(kind){
        case 0:while(itn--) acubicSubdiv(c);break;
	    case 1:while(itn--) aquadSubdiv(c);break;
	    case 2:while(itn--) kobbeltSubdiv(c);break;
	    case 3:while(itn--) icubicSubdiv(c);break;
	    case 4:while(itn--) gauss3(c);break;
        case 5:while (itn--) gauss5(c); break;
        case 6:while (itn--) cosinebell5(c);break;
        case 7:while (itn--) windowedSinc5(c);
    }
// cBoundary  ();
    writecurves(cnt2,c);
    freecurves (c);
}
GEOMETRY_API void __stdcall smoothcont(gpc_vertex_list* cnt1, gpc_vertex_list* cnt2, FLOATTYPE hh, int cls, int kind, int iternum, int ccv, int maxvertex) {
    smoothcontx(cnt1, cnt2, hh, cls, kind, iternum, ccv, maxvertex, FALSE);
}


#ifdef __cplusplus
}
#endif 
