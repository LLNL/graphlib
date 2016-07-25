/*
Copyright (c) 2007-2013
Lawrence Livermore National Security, LLC.

Produced at the Lawrence Livermore National Laboratory.
Written by Martin Schulz, Dorian Arnold, and Gregory L. Lee
Contact email: schulzm@llnl.gov
LLNL-CODE-624053
All rights reserved.

This file is part of GraphLib. For details see https://github.com/lee218llnl/graphlib.

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

  err=graphlib_newGraph(&gr, NULL);
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

  /*err=graphlib_exportGraph("demo-a.gml",GRF_GML,gr);*/
  err=graphlib_exportGraph("demo-a.dot",GRF_DOT,gr);
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

  err=graphlib_newGraph(&gr, NULL);
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

  /*err=graphlib_exportGraph("demo-b.gml",GRF_GML,gr);*/
  err=graphlib_exportGraph("demo-b.dot",GRF_DOT,gr);
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

  err=graphlib_loadGraph("demo-a.grl",&gr1,NULL);
  CHECKERROR(err,TESTNO,"Step 1");

  err=graphlib_loadGraph("demo-b.grl",&gr2,NULL);
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

  err=graphlib_loadGraph("demo-c.grl",&gr,NULL);
  CHECKERROR(err,TESTNO,"Step 1");

  /*err=graphlib_exportGraph("demo-d.gml",GRF_GML,gr);*/
  err=graphlib_exportGraph("demo-d.dot",GRF_DOT,gr);
  CHECKERROR(err,TESTNO,"Step 2");

  err=graphlib_delGraph(gr);
  CHECKERROR(err,TESTNO,"Step 3");
}

#undef TESTNO

/*----------------------------------------------*/
/* TEST E: load, serialize, deserialize, export */

#define TESTNO "E"

void testE()
{
  graphlib_graph_p gr=NULL,gr2=NULL;
  graphlib_error_t err;
  char *ba=0;
  unsigned long ba_len=0;

  err=graphlib_loadGraph("demo-c.grl",&gr,NULL);
  CHECKERROR(err,TESTNO,"Step 1");

  err=graphlib_serializeGraph(gr,&ba,&ba_len);
  CHECKERROR(err,TESTNO,"Step 2");

  err=graphlib_deserializeGraph(&gr2,NULL,ba,ba_len);
  CHECKERROR(err,TESTNO,"Step 3");
  if (ba!=NULL)
    free(ba);

  /*err=graphlib_exportGraph("demo-e.gml",GRF_GML,gr2);*/
  err=graphlib_exportGraph("demo-e.dot",GRF_DOT,gr2);
  CHECKERROR(err,TESTNO,"Step 4");

  err=graphlib_delGraph(gr);
  CHECKERROR(err,TESTNO,"Step 5");
}

#undef TESTNO

/*-----------------------------------------------------*/
/* TEST F: Create annotated graph and save it */

#define TESTNO "TEST F"

void testF()
{
  graphlib_graph_p gr,gr2;
  graphlib_error_t err;

  err=graphlib_newAnnotatedGraph(&gr,NULL,3);
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

  err=graphlib_loadGraph("demo-f.grl",&gr2,NULL);
  CHECKERROR(err,TESTNO,"Step 24");
  err=graphlib_exportGraph("demo-f-2.gml",GRF_GML,gr2);
  CHECKERROR(err,TESTNO,"Step 25");
  err=graphlib_delGraph(gr2);
  CHECKERROR(err,TESTNO,"Step 26");
}

#undef TESTNO

/*-----------------------------------------------------*/
/* TEST G: Create annotated graph and save it */

#define TESTNO "TEST G"

void testG()
{
  graphlib_graph_p    gr,gr2,gr3;
  graphlib_error_t    err;
  graphlib_nodeattr_p nattr;
  graphlib_edgeattr_p eattr;
  char                *ba=0;
  unsigned long        ba_len=0;

  err=graphlib_newGraph(&gr, NULL);
  CHECKERROR(err,TESTNO,"Step 0");

  nattr=(graphlib_nodeattr_p)malloc(sizeof(graphlib_nodeattr_t));
  graphlib_setDefNodeAttr(nattr);
  nattr->label="node1";
  err=graphlib_addNode(gr,10000,nattr);
  free(nattr);
  CHECKERROR(err,TESTNO,"Step 1");
  nattr=(graphlib_nodeattr_p)malloc(sizeof(graphlib_nodeattr_t));
  graphlib_setDefNodeAttr(nattr);
  nattr->label="node2";
  err=graphlib_addNode(gr,11,nattr);
  free(nattr);
  CHECKERROR(err,TESTNO,"Step 2");
  nattr=(graphlib_nodeattr_p)malloc(sizeof(graphlib_nodeattr_t));
  graphlib_setDefNodeAttr(nattr);
  nattr->label="node3";
  err=graphlib_addNode(gr,2,nattr);
  free(nattr);
  CHECKERROR(err,TESTNO,"Step 3");
  nattr=(graphlib_nodeattr_p)malloc(sizeof(graphlib_nodeattr_t));
  graphlib_setDefNodeAttr(nattr);
  nattr->label="node4";
  err=graphlib_addNode(gr,13,nattr);
  free(nattr);
  CHECKERROR(err,TESTNO,"Step 4");
  nattr=(graphlib_nodeattr_p)malloc(sizeof(graphlib_nodeattr_t));
  graphlib_setDefNodeAttr(nattr);
  nattr->label="node5";
  err=graphlib_addNode(gr,14,nattr);
  free(nattr);
  CHECKERROR(err,TESTNO,"Step 5");
  nattr=(graphlib_nodeattr_p)malloc(sizeof(graphlib_nodeattr_t));
  graphlib_setDefNodeAttr(nattr);
  nattr->label="node6";
  err=graphlib_addNode(gr,5,nattr);
  free(nattr);
  CHECKERROR(err,TESTNO,"Step 6");

  eattr=(graphlib_edgeattr_p)malloc(sizeof(graphlib_edgeattr_t));
  graphlib_setDefEdgeAttr(eattr);
  eattr->label="edge1";
  err=graphlib_addDirectedEdge(gr,10000,11,eattr);
  free(eattr);
  CHECKERROR(err,TESTNO,"Step 7");
  eattr=(graphlib_edgeattr_p)malloc(sizeof(graphlib_edgeattr_t));
  graphlib_setDefEdgeAttr(eattr);
  eattr->label="edge2";
  err=graphlib_addDirectedEdge(gr,11,2,eattr);
  free(eattr);
  CHECKERROR(err,TESTNO,"Step 8");
  eattr=(graphlib_edgeattr_p)malloc(sizeof(graphlib_edgeattr_t));
  graphlib_setDefEdgeAttr(eattr);
  eattr->label="edge3";
  err=graphlib_addDirectedEdge(gr,2,14,eattr);
  free(eattr);
  CHECKERROR(err,TESTNO,"Step 9");
  eattr=(graphlib_edgeattr_p)malloc(sizeof(graphlib_edgeattr_t));
  graphlib_setDefEdgeAttr(eattr);
  eattr->label="edge4";
  err=graphlib_addDirectedEdge(gr,11,13,eattr);
  free(eattr);
  CHECKERROR(err,TESTNO,"Step 10");
  eattr=(graphlib_edgeattr_p)malloc(sizeof(graphlib_edgeattr_t));
  graphlib_setDefEdgeAttr(eattr);
  eattr->label="edge5";
  err=graphlib_addDirectedEdge(gr,13,14,eattr);
  free(eattr);
  CHECKERROR(err,TESTNO,"Step 11");
  eattr=(graphlib_edgeattr_p)malloc(sizeof(graphlib_edgeattr_t));
  graphlib_setDefEdgeAttr(eattr);
  eattr->label="edge6";
  err=graphlib_addDirectedEdge(gr,14,5,eattr);
  free(eattr);
  CHECKERROR(err,TESTNO,"Step 12");

  err=graphlib_saveGraph("demo-g.grl",gr);
  CHECKERROR(err,TESTNO,"Step 13");

  err=graphlib_loadGraph("demo-g.grl",&gr2,NULL);
  CHECKERROR(err,TESTNO,"Step 14");
  nattr=(graphlib_nodeattr_p)malloc(sizeof(graphlib_nodeattr_t));
  graphlib_setDefNodeAttr(nattr);
  nattr->label="node7";
  err=graphlib_addNode(gr2,6,nattr);
  free(nattr);
  CHECKERROR(err,TESTNO,"Step 15");
  eattr=(graphlib_edgeattr_p)malloc(sizeof(graphlib_edgeattr_t));
  graphlib_setDefEdgeAttr(eattr);
  eattr->label="edge7";
  err=graphlib_addDirectedEdge(gr2,10000,6,eattr);
  free(eattr);
  CHECKERROR(err,TESTNO,"Step 16");
  nattr=(graphlib_nodeattr_p)malloc(sizeof(graphlib_nodeattr_t));
  graphlib_setDefNodeAttr(nattr);
  nattr->label="node8";
  err=graphlib_addNode(gr2,96,nattr);
  free(nattr);
  CHECKERROR(err,TESTNO,"Step 17");
  eattr=(graphlib_edgeattr_p)malloc(sizeof(graphlib_edgeattr_t));
  graphlib_setDefEdgeAttr(eattr);
  eattr->label="edge8";
  err=graphlib_addDirectedEdge(gr2,10000,96,eattr);
  free(eattr);
  CHECKERROR(err,TESTNO,"Step 18");

  nattr=(graphlib_nodeattr_p)malloc(sizeof(graphlib_nodeattr_t));
  graphlib_setDefNodeAttr(nattr);
  nattr->label="node9";
  err=graphlib_addNode(gr,66,nattr);
  free(nattr);
  CHECKERROR(err,TESTNO,"Step 19");
  eattr=(graphlib_edgeattr_p)malloc(sizeof(graphlib_edgeattr_t));
  graphlib_setDefEdgeAttr(eattr);
  eattr->label="edge9";
  err=graphlib_addDirectedEdge(gr,10000,66,eattr);
  free(eattr);
  CHECKERROR(err,TESTNO,"Step 20");
  nattr=(graphlib_nodeattr_p)malloc(sizeof(graphlib_nodeattr_t));
  graphlib_setDefNodeAttr(nattr);
  nattr->label="node10";
  err=graphlib_addNode(gr,96,nattr);
  free(nattr);
  CHECKERROR(err,TESTNO,"Step 21");
  eattr=(graphlib_edgeattr_p)malloc(sizeof(graphlib_edgeattr_t));
  graphlib_setDefEdgeAttr(eattr);
  eattr->label="edge10";
  err=graphlib_addDirectedEdge(gr,10000,96,eattr);
  free(eattr);
  CHECKERROR(err,TESTNO,"Step 22");

  err=graphlib_exportGraph("demo-g1.dot",GRF_DOT,gr);
  CHECKERROR(err,TESTNO,"Step 23");
  err=graphlib_exportGraph("demo-g2.dot",GRF_DOT,gr2);
  CHECKERROR(err,TESTNO,"Step 24");


  err=graphlib_mergeGraphs(gr,gr2);
  CHECKERROR(err,TESTNO,"Step 25");
  err=graphlib_exportGraph("demo-g3.dot",GRF_DOT,gr);
  CHECKERROR(err,TESTNO,"Step 26");

  err=graphlib_serializeBasicGraph(gr,&ba,&ba_len);
  CHECKERROR(err,TESTNO,"Step 27");
  err=graphlib_deserializeBasicGraph(&gr3,NULL,ba,ba_len);
  CHECKERROR(err,TESTNO,"Step 28");
  if (ba!=NULL)
    free(ba);
  err=graphlib_exportGraph("demo-g4.dot",GRF_DOT,gr3);
  CHECKERROR(err,TESTNO,"Step 29");

  err=graphlib_delGraph(gr);
  CHECKERROR(err,TESTNO,"Step 30");
  err=graphlib_delGraph(gr2);
  CHECKERROR(err,TESTNO,"Step 31");
  err=graphlib_delGraph(gr3);
  CHECKERROR(err,TESTNO,"Step 32");
}

#undef TESTNO

/*-----------------------------------------------------*/
/* TEST H: Create annotated graph and save it */

#define TESTNO "TEST H"

void testH()
{
  graphlib_graph_p    gr,gr2,gr3;
  graphlib_error_t    err;
  graphlib_nodeattr_p nattr;
  graphlib_edgeattr_p eattr;
  char                *ba=0,*key;
  unsigned long       ba_len=0;
  int                 index,i,num_attrs;

  err=graphlib_newGraph(&gr, NULL);
  CHECKERROR(err,TESTNO,"Step 0");
  err=graphlib_addNodeAttrKey(gr,"test1",&index);
  CHECKERROR(err,TESTNO,"Step 0.1");
  err=graphlib_addNodeAttrKey(gr,"test2",&index);
  CHECKERROR(err,TESTNO,"Step 0.2");

  nattr=(graphlib_nodeattr_p)malloc(sizeof(graphlib_nodeattr_t));
  graphlib_setDefNodeAttr(nattr);
  nattr->attr_values = (void **)malloc(2*sizeof(void*));

  nattr->label="node1";
  nattr->attr_values[0]="node1";
  nattr->attr_values[1]="node1";
  err=graphlib_addNode(gr,10000,nattr);
  CHECKERROR(err,TESTNO,"Step 1");
  nattr->label="node2";
  nattr->attr_values[0]="node2";
  nattr->attr_values[1]="node2";
  err=graphlib_addNode(gr,11,nattr);
  CHECKERROR(err,TESTNO,"Step 2");
  nattr->label="node3";
  nattr->attr_values[0]="node3";
  nattr->attr_values[1]="node3";
  err=graphlib_addNode(gr,2,nattr);
  CHECKERROR(err,TESTNO,"Step 3");
  nattr->label="node4";
  nattr->attr_values[0]="node4";
  nattr->attr_values[1]="node4";
  err=graphlib_addNode(gr,13,nattr);
  CHECKERROR(err,TESTNO,"Step 4");
  nattr->label="node5";
  nattr->attr_values[0]="node5";
  nattr->attr_values[1]="node5";
  err=graphlib_addNode(gr,14,nattr);
  CHECKERROR(err,TESTNO,"Step 5");
  nattr->label="node6";
  nattr->attr_values[0]="node6";
  nattr->attr_values[1]="node6";
  err=graphlib_addNode(gr,5,nattr);
  CHECKERROR(err,TESTNO,"Step 6");

  err=graphlib_addEdgeAttrKey(gr,"test1",&index);
  CHECKERROR(err,TESTNO,"Step 6.1");
  err=graphlib_addEdgeAttrKey(gr,"test2",&index);
  CHECKERROR(err,TESTNO,"Step 6.2");
  eattr=(graphlib_edgeattr_p)malloc(sizeof(graphlib_edgeattr_t));
  graphlib_setDefEdgeAttr(eattr);
  eattr->attr_values = (void **)malloc(2*sizeof(void*));

  eattr->label="edge1";
  eattr->attr_values[0]="edge1";
  eattr->attr_values[1]="edge1";
  err=graphlib_addDirectedEdge(gr,10000,11,eattr);
  CHECKERROR(err,TESTNO,"Step 7");
  eattr->label="edge2";
  eattr->attr_values[0]="edge2";
  eattr->attr_values[1]="edge2";
  err=graphlib_addDirectedEdge(gr,11,2,eattr);
  CHECKERROR(err,TESTNO,"Step 8");
  eattr->label="edge3";
  eattr->attr_values[0]="edge3";
  eattr->attr_values[1]="edge3";
  err=graphlib_addDirectedEdge(gr,2,14,eattr);
  CHECKERROR(err,TESTNO,"Step 9");
  eattr->label="edge4";
  eattr->attr_values[0]="edge4";
  eattr->attr_values[1]="edge4";
  err=graphlib_addDirectedEdge(gr,11,13,eattr);
  CHECKERROR(err,TESTNO,"Step 10");
  eattr->label="edge5";
  eattr->attr_values[0]="edge5";
  eattr->attr_values[1]="edge5";
  err=graphlib_addDirectedEdge(gr,13,14,eattr);
  CHECKERROR(err,TESTNO,"Step 11");
  eattr->label="edge6";
  eattr->attr_values[0]="edge6";
  eattr->attr_values[1]="edge6";
  err=graphlib_addDirectedEdge(gr,14,5,eattr);
  CHECKERROR(err,TESTNO,"Step 12");

  err=graphlib_saveGraph("demo-h.grl",gr);
  CHECKERROR(err,TESTNO,"Step 13");

  err=graphlib_loadGraph("demo-h.grl",&gr2,NULL);
  CHECKERROR(err,TESTNO,"Step 14");
  nattr->label="node7";
  nattr->attr_values[0]="node7";
  nattr->attr_values[1]="node7";
  err=graphlib_addNode(gr2,6,nattr);
  CHECKERROR(err,TESTNO,"Step 15");
  eattr->label="edge7";
  eattr->attr_values[0]="edge7";
  eattr->attr_values[1]="edge7";
  err=graphlib_addDirectedEdge(gr2,10000,6,eattr);
  CHECKERROR(err,TESTNO,"Step 16");
  nattr->label="node8";
  nattr->attr_values[0]="node8";
  nattr->attr_values[1]="node8";
  err=graphlib_addNode(gr2,96,nattr);
  CHECKERROR(err,TESTNO,"Step 17");
  eattr->label="edge8";
  eattr->attr_values[0]="edge8";
  eattr->attr_values[1]="edge8";
  err=graphlib_addDirectedEdge(gr2,10000,96,eattr);
  CHECKERROR(err,TESTNO,"Step 18");

  nattr->label="node9";
  nattr->attr_values[0]="node9";
  nattr->attr_values[1]="node9";
  err=graphlib_addNode(gr,66,nattr);
  CHECKERROR(err,TESTNO,"Step 19");
  eattr->label="edge9";
  eattr->attr_values[0]="edge9";
  eattr->attr_values[1]="edge9";
  err=graphlib_addDirectedEdge(gr,10000,66,eattr);
  CHECKERROR(err,TESTNO,"Step 20");
  nattr->label="node10";
  nattr->attr_values[0]="node10";
  nattr->attr_values[1]="node10";
  err=graphlib_addNode(gr,96,nattr);
  CHECKERROR(err,TESTNO,"Step 21");
  eattr->label="edge10";
  eattr->attr_values[0]="edge10";
  eattr->attr_values[1]="edge10";
  err=graphlib_addDirectedEdge(gr,10000,96,eattr);
  CHECKERROR(err,TESTNO,"Step 22");

  err=graphlib_exportGraph("demo-h1.dot",GRF_DOT,gr);
  CHECKERROR(err,TESTNO,"Step 23");
  err=graphlib_exportGraph("demo-h2.dot",GRF_DOT,gr2);
  CHECKERROR(err,TESTNO,"Step 24");


  err=graphlib_mergeGraphs(gr,gr2);
  CHECKERROR(err,TESTNO,"Step 25");
  err=graphlib_exportGraph("demo-h3.dot",GRF_DOT,gr);
  CHECKERROR(err,TESTNO,"Step 26");

  err=graphlib_serializeBasicGraph(gr,&ba,&ba_len);
  CHECKERROR(err,TESTNO,"Step 27");
  err=graphlib_deserializeBasicGraph(&gr3,NULL,ba,ba_len);
  CHECKERROR(err,TESTNO,"Step 28");
  if (ba!=NULL)
    free(ba);
  err=graphlib_exportGraph("demo-h4.dot",GRF_DOT,gr3);
  CHECKERROR(err,TESTNO,"Step 29");

  err=graphlib_getNumNodeAttrs(gr3,&num_attrs);
  CHECKERROR(err,TESTNO,"Step 29.1");
  for (i=0;i<num_attrs;i++)
    {
      err=graphlib_getNodeAttrKey(gr3,i,&key);
      CHECKERROR(err,TESTNO,"Step 29.2");
      err=graphlib_getNodeAttrIndex(gr3,key,&index);
      CHECKERROR(err,TESTNO,"Step 29.3");
      printf("%d?=%d, key=%s\n", i, index, key);
    }

  err=graphlib_getNumEdgeAttrs(gr3,&num_attrs);
  CHECKERROR(err,TESTNO,"Step 29.1");
  for (i=0;i<num_attrs;i++)
    {
      err=graphlib_getEdgeAttrKey(gr3,i,&key);
      CHECKERROR(err,TESTNO,"Step 29.2");
      err=graphlib_getEdgeAttrIndex(gr3,key,&index);
      CHECKERROR(err,TESTNO,"Step 29.3");
      printf("%d?=%d, key=%s\n", i, index, key);
    }

  err=graphlib_delGraph(gr);
  CHECKERROR(err,TESTNO,"Step 30");
  err=graphlib_delGraph(gr2);
  CHECKERROR(err,TESTNO,"Step 31");
  err=graphlib_delGraph(gr3);
  CHECKERROR(err,TESTNO,"Step 32");
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
  testE();
  printf("Completed test E\n");
  testF();
  printf("Completed test F\n");
  testG();
  printf("Completed test G\n");
  testH();
  printf("Completed test H\n");

  err=graphlib_Finish();
  CHECKERROR(err,TESTNO,"Step 0");

  return 0;
}
