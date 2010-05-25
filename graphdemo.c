/*
Copyright (c) 2007
Lawrence Livermore National Security, LLC. 

Produced at the Lawrence Livermore National Laboratory. 
Written by Martin Schulz and Dorian Arnold
Contact email: schulzm@llnl.gov
UCRL-CODE-231600
All rights reserved.

This file is part of GraphLib. 

Please also read the file "LICENSE" included in this package for 
Our Notice and GNU Lesser General Public License.

This program is free software; you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
(as published by the Free Software Foundation) version 2.1 
dated February 1999.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY 
OF MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
terms and conditions of the GNU General Public License for more 
details.

You should have received a copy of the GNU Lesser General Public 
License along with this program; if not, write to the 

Free Software Foundation, Inc., 
59 Temple Place, Suite 330, 
Boston, MA 02111-1307 USA
*/


#include <stdio.h>
#include <stdlib.h>
#include "graphlib.h"

#define CHECKERROR(err,no,s) \
{ \
  if (GRL_IS_NOTOK(err)) \
  { \
    printf("Error %i in Test %s at %s\n",err,no,s); \
    if (GRL_IS_FATALERROR(err)) \
    { \
      printf("Fatal error - exiting ...\n"); \
      exit(1); \
    } \
  } \
}

/*-----------------------------------------------------*/
/* TEST A: Create a graph and save it */

#define TESTNO "TEST A"

void testA()
{
  graphlib_graph_p gr;
  graphlib_error_t err;

  err=graphlib_newGraph(&gr);
  CHECKERROR(err,TESTNO,"Step 0");

  err=graphlib_addNode(gr,0,NULL);
  CHECKERROR(err,TESTNO,"Step 1");
  err=graphlib_addNode(gr,1,NULL);
  CHECKERROR(err,TESTNO,"Step 2");
  err=graphlib_addNode(gr,2,NULL);
  CHECKERROR(err,TESTNO,"Step 3");
  err=graphlib_addNode(gr,3,NULL);
  CHECKERROR(err,TESTNO,"Step 4");
  err=graphlib_addNode(gr,4,NULL);
  CHECKERROR(err,TESTNO,"Step 5");
  err=graphlib_addNode(gr,5,NULL);
  CHECKERROR(err,TESTNO,"Step 6");

  err=graphlib_addDirectedEdge(gr,0,1,NULL);
  CHECKERROR(err,TESTNO,"Step 7");
  err=graphlib_addDirectedEdge(gr,1,2,NULL);
  CHECKERROR(err,TESTNO,"Step 8");
  err=graphlib_addDirectedEdge(gr,2,4,NULL);
  CHECKERROR(err,TESTNO,"Step 9");
  err=graphlib_addDirectedEdge(gr,1,3,NULL);
  CHECKERROR(err,TESTNO,"Step 10");
  err=graphlib_addDirectedEdge(gr,3,4,NULL);
  CHECKERROR(err,TESTNO,"Step 11");
  err=graphlib_addDirectedEdge(gr,4,5,NULL);
  CHECKERROR(err,TESTNO,"Step 12");

  err=graphlib_exportGraph("demo-a.gml",GRF_GML,gr);
  CHECKERROR(err,TESTNO,"Step 13");  
  err=graphlib_exportGraph("demo-a.dot",GRF_DOT,gr);
  CHECKERROR(err,TESTNO,"Step 14");  
  err=graphlib_exportGraph("demo-a.pdot",GRF_PLAINDOT,gr);
  CHECKERROR(err,TESTNO,"Step 15");  

  err=graphlib_saveGraph("demo-a.grl",gr);
  CHECKERROR(err,TESTNO,"Step 16");  

  err=graphlib_delGraph(gr);
  CHECKERROR(err,TESTNO,"Step 17");  
}

#undef TESTNO


/*-----------------------------------------------------*/
/* TEST B: Create second graph and save it */

#define TESTNO "B"

void testB()
{
  graphlib_graph_p gr;
  graphlib_error_t err;
  graphlib_nodeattr_t attr;
  int              i,j,a,id,lastnode;

  err=graphlib_newGraph(&gr);
  CHECKERROR(err,TESTNO,"Step 0");

  err=graphlib_addNode(gr,10000,NULL);
  CHECKERROR(err,TESTNO,"Step 1");
  err=graphlib_addNode(gr,11,NULL);
  CHECKERROR(err,TESTNO,"Step 2");
  err=graphlib_addNode(gr,2,NULL);
  CHECKERROR(err,TESTNO,"Step 3");
  err=graphlib_addNode(gr,13,NULL);
  CHECKERROR(err,TESTNO,"Step 4");
  err=graphlib_addNode(gr,14,NULL);
  CHECKERROR(err,TESTNO,"Step 5");
  err=graphlib_addNode(gr,5,NULL);
  CHECKERROR(err,TESTNO,"Step 6");

  err=graphlib_addUndirectedEdge(gr,10000,11,NULL);
  CHECKERROR(err,TESTNO,"Step 7");
  err=graphlib_addUndirectedEdge(gr,11,2,NULL);
  CHECKERROR(err,TESTNO,"Step 8");
  err=graphlib_addUndirectedEdge(gr,2,14,NULL);
  CHECKERROR(err,TESTNO,"Step 9");
  err=graphlib_addUndirectedEdge(gr,11,13,NULL);
  CHECKERROR(err,TESTNO,"Step 10");
  err=graphlib_addUndirectedEdge(gr,13,14,NULL);
  CHECKERROR(err,TESTNO,"Step 11");
  err=graphlib_addUndirectedEdge(gr,14,5,NULL);
  CHECKERROR(err,TESTNO,"Step 12");

  err=graphlib_addNode(gr,20000,NULL);
  CHECKERROR(err,TESTNO,"Step 13");

  graphlib_setDefNodeAttr(&attr);
  attr.color=GRC_RED;

  a=1;
  for (i=0; i<4; i++)
    {
      for (j=0; j<a; j++)
	{
	  id=j+a;
	  err=graphlib_addNode(gr,20000+id,&attr);
	  CHECKERROR(err,TESTNO,"Step 14");
	  err=graphlib_addDirectedEdge(gr,20000+(id/2),20000+id,NULL);
	  CHECKERROR(err,TESTNO,"Step 15");
	}
      a*=2;
    }
  err=graphlib_deleteInvertedPath(gr,(a/2)+3+20000,&lastnode);
  printf("Deleting %i\n",(a/2)+3+20000);
  CHECKERROR(err,TESTNO,"Step 16");  

  a=1;
  for (i=0; i<8; i++)
    {
      for (j=0; j<a; j++)
	{
	  id=j+a;
	  err=graphlib_addNode(gr,10000+id,NULL);
	  CHECKERROR(err,TESTNO,"Step 17");
	  err=graphlib_addUndirectedEdge(gr,10000+id,10000+(id/2),NULL);
	  CHECKERROR(err,TESTNO,"Step 18");
	}
      a*=2;
    }

  err=graphlib_exportGraph("demo-b.gml",GRF_GML,gr);
  CHECKERROR(err,TESTNO,"Step 19");  

  err=graphlib_saveGraph("demo-b.grl",gr);
  CHECKERROR(err,TESTNO,"Step 20");  

  err=graphlib_delGraph(gr);
  CHECKERROR(err,TESTNO,"Step 21");  
}

#undef TESTNO


/*-----------------------------------------------------*/
/* TEST C: load and merge */

#define TESTNO "C"

void testC()
{
  graphlib_graph_p gr1,gr2;
  graphlib_error_t err;

  err=graphlib_loadGraph("demo-a.grl",&gr1);
  CHECKERROR(err,TESTNO,"Step 1");  

  err=graphlib_loadGraph("demo-b.grl",&gr2);
  CHECKERROR(err,TESTNO,"Step 2");  

  err=graphlib_mergeGraphs(gr1,gr2);
  CHECKERROR(err,TESTNO,"Step 3");  

  err=graphlib_saveGraph("demo-c.grl",gr1);
  CHECKERROR(err,TESTNO,"Step 4");  

  err=graphlib_delGraph(gr1);
  CHECKERROR(err,TESTNO,"Step 5");  

  err=graphlib_delGraph(gr2);
  CHECKERROR(err,TESTNO,"Step 6");  
}

#undef TESTNO


/*-----------------------------------------------------*/
/* TEST D: load and export */

#define TESTNO "D"

void testD()
{
  graphlib_graph_p gr;
  graphlib_error_t err;

  err=graphlib_loadGraph("demo-c.grl",&gr);
  CHECKERROR(err,TESTNO,"Step 1");  

  err=graphlib_exportGraph("demo-d.gml",GRF_GML,gr);
  CHECKERROR(err,TESTNO,"Step 2");  

  err=graphlib_delGraph(gr);
  CHECKERROR(err,TESTNO,"Step 3");  
}

#undef TESTNO

/*----------------------------------------------*/
/* TEST E: load, serialize, deserialize, export */

#ifndef NOSET 

#define TESTNO "E"

void testE()
{
  graphlib_graph_p gr=NULL, gr2=NULL;
  graphlib_error_t err;
  char * ba=0;
  unsigned int ba_len=0;
  
  err=graphlib_loadGraph("demo-c.grl",&gr);
  CHECKERROR(err,TESTNO,"Step 1");  
  
  err=graphlib_serializeGraph( gr, &ba, &ba_len);
  CHECKERROR(err,TESTNO,"Step 2");  
  
  err=graphlib_deserializeGraph( &gr2, ba, ba_len);
  CHECKERROR(err,TESTNO,"Step 3");  
  
  err=graphlib_exportGraph("demo-e.gml",GRF_GML,gr2);
  CHECKERROR(err,TESTNO,"Step 4");  
  
  err=graphlib_delGraph(gr);
  CHECKERROR(err,TESTNO,"Step 5");  
}

#undef TESTNO

#endif

/*-----------------------------------------------------*/
/* TEST F: Create annotated graph and save it */

#define TESTNO "TEST F"

void testF()
{
  graphlib_graph_p gr,gr2;
  graphlib_error_t err;

  err=graphlib_newAnnotatedGraph(&gr,3);
  CHECKERROR(err,TESTNO,"Step 0");

  err=graphlib_AnnotationKey(gr,0,"Label 1");
  CHECKERROR(err,TESTNO,"Step 1");
  err=graphlib_AnnotationKey(gr,2,"Label 3");
  CHECKERROR(err,TESTNO,"Step 2");

  err=graphlib_addNode(gr,0,NULL);
  CHECKERROR(err,TESTNO,"Step 3");
  err=graphlib_addNode(gr,1,NULL);
  CHECKERROR(err,TESTNO,"Step 4");
  err=graphlib_addNode(gr,2,NULL);
  CHECKERROR(err,TESTNO,"Step 5");
  err=graphlib_addNode(gr,3,NULL);
  CHECKERROR(err,TESTNO,"Step 6");
  err=graphlib_addNode(gr,4,NULL);
  CHECKERROR(err,TESTNO,"Step 7");
  err=graphlib_addNode(gr,5,NULL);
  CHECKERROR(err,TESTNO,"Step 8");

  err=graphlib_AnnotationSet(gr,0,0,45.0);
  CHECKERROR(err,TESTNO,"Step 9");
  err=graphlib_AnnotationSet(gr,1,0,42.0);
  CHECKERROR(err,TESTNO,"Step 10");
  err=graphlib_AnnotationSet(gr,2,1,45.0);
  CHECKERROR(err,TESTNO,"Step 11");
  err=graphlib_AnnotationSet(gr,3,1,42.0);
  CHECKERROR(err,TESTNO,"Step 12");
  err=graphlib_AnnotationSet(gr,4,2,45.0);
  CHECKERROR(err,TESTNO,"Step 13");
  err=graphlib_AnnotationSet(gr,5,2,42.0);
  CHECKERROR(err,TESTNO,"Step 14");

  err=graphlib_addDirectedEdge(gr,0,1,NULL);
  CHECKERROR(err,TESTNO,"Step 15");
  err=graphlib_addDirectedEdge(gr,1,2,NULL);
  CHECKERROR(err,TESTNO,"Step 16");
  err=graphlib_addDirectedEdge(gr,2,4,NULL);
  CHECKERROR(err,TESTNO,"Step 17");
  err=graphlib_addDirectedEdge(gr,1,3,NULL);
  CHECKERROR(err,TESTNO,"Step 18");
  err=graphlib_addDirectedEdge(gr,3,4,NULL);
  CHECKERROR(err,TESTNO,"Step 19");
  err=graphlib_addDirectedEdge(gr,4,5,NULL);
  CHECKERROR(err,TESTNO,"Step 20");

  err=graphlib_saveGraph("demo-f.grl",gr);
  CHECKERROR(err,TESTNO,"Step 21");  
  err=graphlib_exportGraph("demo-f-1.gml",GRF_GML,gr);
  CHECKERROR(err,TESTNO,"Step 22");  
  err=graphlib_delGraph(gr);
  CHECKERROR(err,TESTNO,"Step 23");  

  err=graphlib_loadGraph("demo-f.grl",&gr2);
  CHECKERROR(err,TESTNO,"Step 24");  
  err=graphlib_exportGraph("demo-f-2.gml",GRF_GML,gr2);
  CHECKERROR(err,TESTNO,"Step 25");  
  err=graphlib_delGraph(gr2);
  CHECKERROR(err,TESTNO,"Step 26");  
}

#undef TESTNO


/*-----------------------------------------------------*/
/* MAIN */

#define TESTNO "MAIN"

int main(int argc, char **argv)
{
  graphlib_error_t err;

  err=graphlib_Init();
  CHECKERROR(err,TESTNO,"Step 0");

  printf("Graphlib demo\n");
  testA();
  printf("Completed test A\n");
  testB();
  printf("Completed test B\n");
  testC();
  printf("Completed test C\n");
  testD();
  printf("Completed test D\n");
#ifndef NOSET
  testE();
  printf("Completed test E\n");
#endif
  testF();
  printf("Completed test F\n");

  return 0;
}
