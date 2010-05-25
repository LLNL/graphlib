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


/*-----------------------------------------------------------------*/
/* Library to create, manipulate, and export graphs                */
/* Martin Schulz, LLNL, 2005-2006                                  */
/* Additions by Dorian Arnold, LLNL, 2006                          */
/* Modifications by Gregory L. Lee, LLNL, 2006-2007                */
/*-----------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "graphlib.h"

#ifndef NOSET 
#include "IntegerSet.h"
#endif

/*-----------------------------------------------------------------*/
/* Constants */

#define MAXEDGE_GML 5

/*.......................................................*/
/* Attributes */

#define DEFAULT_NODE_COLOR GRC_YELLOW
#define DEFAULT_NODE_WIDTH 20
#define DEFAULT_EDGE_COLOR GRC_BLUE
#define DEFAULT_EDGE_WIDTH 1

#define DEFAULT_FONT_SIZE  -1
#define DEFAULT_BLOCK      GRB_NONE
#define DEFAULT_EDGE_STYLE GRA_LINE

#define DEFAULT_NODE_COOR  -1


#define GRAPHLIB_DEFAULT_ANNOTATION 0.0


/*.......................................................*/
/* Fragment sizes */

#define NODEFRAGSIZE 2000
#define EDGEFRAGSIZE 2000


/*-----------------------------------------------------------------*/
/* Types */

/*............................................................*/
/* Nodes */

typedef struct graphlib_nodeentry_d *graphlib_nodeentry_p;

typedef struct graphlib_nodedata_d *graphlib_nodedata_p;
typedef struct graphlib_nodedata_d
{
  graphlib_node_t     id;
  graphlib_nodeattr_t attr;  
} graphlib_nodedata_t;

typedef union graphlib_innodeentry_d
{
  graphlib_nodeentry_p freeptr;
  graphlib_nodedata_t  data;
} graphlib_innodeentry_t;

typedef struct graphlib_nodeentry_d
{
  int                    full;
  graphlib_innodeentry_t entry;
} graphlib_nodeentry_t;


typedef struct graphlib_nodefragment_d *graphlib_nodefragment_p;
typedef struct graphlib_nodefragment_d
{
  int                      count;
  graphlib_nodefragment_p  next;
  graphlib_annotation_t     *grannot;
  /* must be last */
  graphlib_nodeentry_t     node[NODEFRAGSIZE];
} graphlib_nodefragment_t;


/*............................................................*/
/* Edges */

typedef struct graphlib_edgeentry_d *graphlib_edgeentry_p;

typedef struct graphlib_edgedata_d *graphlib_edgedata_p;
typedef struct graphlib_edgedata_d
{
  graphlib_nodeentry_t *ref_from;
  graphlib_nodeentry_t *ref_to;
  graphlib_node_t       node_from;
  graphlib_node_t       node_to;
  graphlib_edgeattr_t   attr;
} graphlib_edgedata_t;

typedef union graphlib_inedgeentry_d
{
  graphlib_edgeentry_p freeptr;
  graphlib_edgedata_t  data;
} graphlib_inedgeentry_t;

typedef struct graphlib_edgeentry_d
{
  int                    full;
  graphlib_inedgeentry_t entry;
} graphlib_edgeentry_t;

typedef struct graphlib_edgefragment_d *graphlib_edgefragment_p;
typedef struct graphlib_edgefragment_d
{
  int                      count;
  graphlib_edgefragment_p  next;
  /* must be last */
  graphlib_edgeentry_t     edge[EDGEFRAGSIZE];
} graphlib_edgefragment_t;


/*............................................................*/
/* Graph and Graphlist */

typedef struct graphlib_graph_d
{
  int                      directed;
  int                      edgeset;
  int                      numannotation;
  char                    **annotations;
  graphlib_nodefragment_t *nodes;
  graphlib_edgefragment_t *edges;
  graphlib_nodeentry_p     freenodes;
  graphlib_edgeentry_p     freeedges;
} graphlib_graph_t;

typedef struct graphlib_graphlist_d *graphlib_graphlist_p;
typedef struct graphlib_graphlist_d
{
  graphlib_graph_p      graph;
  graphlib_graphlist_p  next;
} graphlib_graphlist_t;


/*-----------------------------------------------------------------*/
/* Variables */

graphlib_graphlist_t *allgraphs;

static unsigned int grlibint_num_colors=0;

#ifdef STAT_BITVECTOR
static void *node_clusters[GRC_RAINBOWCOLORS];
static int grlibint_edgelabelwidth=-1;
#else
static long *node_clusters[GRC_RAINBOWCOLORS];
#endif

static int grlibint_connections=1;
static int *grlibint_connwidths=NULL;

/*-----------------------------------------------------------------*/
/* Simple support routines */

/*............................................................*/
/* find a node in the node table with indices */

graphlib_error_t grlibint_findNodeIndex(graphlib_graph_p graph, 
					graphlib_node_t  node,
					graphlib_nodeentry_p *entry,
					graphlib_nodefragment_p *nodefrag,
					int *index)
{
  *nodefrag=graph->nodes;
  while ((*nodefrag)!=NULL)
    {
      for ((*index)=0; (*index)<(*nodefrag)->count; (*index)++)
	{
	  if ((*nodefrag)->node[*index].full)
	    {
	      if ((*nodefrag)->node[*index].entry.data.id==node)
		{
		  *entry=&((*nodefrag)->node[(*index)]);
		  return GRL_OK;
		}
	    }
	}
      (*nodefrag)=(*nodefrag)->next;
    }
  
  return GRL_NONODE;
}


/*............................................................*/
/* find a node in the node table */

graphlib_error_t grlibint_findNode(graphlib_graph_p graph, 
				   graphlib_node_t  node,
				   graphlib_nodeentry_p *entry)
{
  int                     i;
  graphlib_nodefragment_p nodefrag;

  return grlibint_findNodeIndex(graph,node,entry,&nodefrag,&i);
}


/*............................................................*/
/* find an edge in the edge table */

graphlib_error_t grlibint_findEdge(graphlib_graph_p graph, 
				   graphlib_node_t  node1,
				   graphlib_node_t  node2,
				   graphlib_edgeentry_p *entry)
{
  int                     i;
  graphlib_edgefragment_p edgefrag;

  edgefrag=graph->edges;
  while (edgefrag!=NULL)
    {
      for (i=0; i<edgefrag->count; i++)
	{
	  if (edgefrag->edge[i].full)
	    {
	      if ((edgefrag->edge[i].entry.data.node_from==node1) &&
		  (edgefrag->edge[i].entry.data.node_to==node2))
		{
		  *entry=&(edgefrag->edge[i]);
		  return GRL_OK;
		}
	    }
	}
      edgefrag=edgefrag->next;
    }
  
  return GRL_NOEDGE;
}


/*............................................................*/
/* find an incoming edge of a given color */

graphlib_error_t grlibint_findIncomingEdgeColor(graphlib_graph_p graph,
						graphlib_node_t node,
						graphlib_color_t color,
						graphlib_edgeentry_p *entry)
{
  int                     i;
  graphlib_edgefragment_p edgefrag;

  edgefrag=graph->edges;
  while (edgefrag!=NULL)
    {
      for (i=0; i<edgefrag->count; i++)
	{
	  if (edgefrag->edge[i].full)
	    {
	      if ((edgefrag->edge[i].entry.data.attr.color==color) &&
		  (edgefrag->edge[i].entry.data.node_to==node))
		{
		  *entry=&(edgefrag->edge[i]);
		  return GRL_OK;
		}
	    }
	}
      edgefrag=edgefrag->next;
    }
  
  return GRL_NOEDGE;
}

 
/*............................................................*/
/* find an incoming edge */

graphlib_error_t grlibint_findIncomingEdge(graphlib_graph_p graph,
					   graphlib_node_t node,
					   graphlib_edgeentry_p *entry)
{
  int                     i;
  graphlib_edgefragment_p edgefrag;

  edgefrag=graph->edges;
  while (edgefrag!=NULL)
    {
      for (i=0; i<edgefrag->count; i++)
	{
	  if (edgefrag->edge[i].full)
	    {
	      if (edgefrag->edge[i].entry.data.node_to==node)
		{
		  *entry=&(edgefrag->edge[i]);
		  return GRL_OK;
		}
	    }
	}
      edgefrag=edgefrag->next;
    }
  
  return GRL_NOEDGE;
}

 
/*............................................................*/
/* find an outgoing edge */

graphlib_error_t grlibint_findOutgoingEdge(graphlib_graph_p graph,
					   graphlib_node_t node,
					   graphlib_edgeentry_p *entry)
{
  int                     i;
  graphlib_edgefragment_p edgefrag;

  edgefrag=graph->edges;
  while (edgefrag!=NULL)
    {
      for (i=0; i<edgefrag->count; i++)
	{
	  if (edgefrag->edge[i].full)
	    {
	      if (edgefrag->edge[i].entry.data.node_from==node)
		{
		  *entry=&(edgefrag->edge[i]);
		  return GRL_OK;
		}
	    }
	}
      edgefrag=edgefrag->next;
    }
  
  return GRL_NOEDGE;
}

 
/*............................................................*/
/* find an incoming or outgoing edge */

graphlib_error_t grlibint_findNodeEdge(graphlib_graph_p graph,
				       graphlib_node_t node,
				       graphlib_edgeentry_p *entry)
{
  int                     i;
  graphlib_edgefragment_p edgefrag;

  edgefrag=graph->edges;
  while (edgefrag!=NULL)
    {
      for (i=0; i<edgefrag->count; i++)
	{
	  if (edgefrag->edge[i].full)
	    {
	      if ((edgefrag->edge[i].entry.data.node_from==node) ||
		  (edgefrag->edge[i].entry.data.node_to==node))
		{
		  *entry=&(edgefrag->edge[i]);
		  return GRL_OK;
		}
	    }
	}
      edgefrag=edgefrag->next;
    }
  
  return GRL_NOEDGE;
}

/*............................................................*/
/* delete a node */

graphlib_error_t grlibint_delNode(graphlib_graph_p graph, 
				  graphlib_nodeentry_p  node)
{
  if (node->full==0)
    {
      return GRL_NONODE;
    }

#ifdef GRL_DYNAMIC_NODE_NAME
  if (node->entry.data.attr.name != NULL)
    {
      free(node->entry.data.attr.name);
    }
#endif

  node->full=0;
  node->entry.freeptr=graph->freenodes;
  graph->freenodes=node;

  return GRL_OK;
}


/*............................................................*/
/* delete an edge */

graphlib_error_t grlibint_delEdge(graphlib_graph_p graph, 
				  graphlib_edgeentry_p edge)
{
  if (edge->full==0)
    {
      return GRL_NOEDGE;
    }

  edge->full=0;
  edge->entry.freeptr=graph->freeedges;
#ifdef STAT_BITVECTOR
  if (edge->entry.data.attr.edgelist != NULL)
    bitvec_delete(&(edge->entry.data.attr.edgelist));
#endif
  graph->freeedges=edge;

  return GRL_OK;
}


/*............................................................*/
/* create and register a new graph */

graphlib_error_t grlibint_addGraph(graphlib_graph_p *newgraph)
{
  graphlib_graphlist_p newitem;

  newitem=(graphlib_graphlist_t*) malloc(sizeof(graphlib_graphlist_t));
  if (newitem==NULL)
    return GRL_NOMEM;

  *newgraph=(graphlib_graph_p) malloc(sizeof(graphlib_graph_t));
  if (*newgraph==NULL)
    {
      free(newitem);
      return GRL_NOMEM;
    }

  newitem->next=allgraphs;
  newitem->graph=*newgraph;
  allgraphs=newitem;

  return GRL_OK;
}

  
/*............................................................*/
/* create and initialize new node segment */

graphlib_error_t grlibint_newNodeFragment(graphlib_nodefragment_p *newnodefrag, int numannotation)
{
  int i;

  *newnodefrag=(graphlib_nodefragment_t*) malloc(sizeof(graphlib_nodefragment_t));
  if (*newnodefrag==NULL)
      return GRL_NOMEM;

  (*newnodefrag)->next=NULL;
  (*newnodefrag)->count=0;

  if (numannotation==0)
    (*newnodefrag)->grannot=NULL;
  else
    {
      (*newnodefrag)->grannot=
	(graphlib_annotation_t*) malloc(sizeof(graphlib_annotation_t)*NODEFRAGSIZE*numannotation);
      if (*newnodefrag==NULL)
	{
	  free(*newnodefrag);
	  return GRL_NOMEM;
	}
      for (i=0; i<NODEFRAGSIZE*numannotation; i++)
	{
	  (*newnodefrag)->grannot[i]=GRAPHLIB_DEFAULT_ANNOTATION;
	}
    }

  return GRL_OK;
}

  
/*............................................................*/
/* create and initialize new node segment */

graphlib_error_t grlibint_newEdgeFragment(graphlib_edgefragment_p *newedgefrag)
{
#ifdef STAT_BITVECTOR      
  int i;
#endif
  *newedgefrag=(graphlib_edgefragment_t*) malloc(sizeof(graphlib_edgefragment_t));
  if (*newedgefrag==NULL)
      return GRL_NOMEM;
/* GLL: Initialize edgelists to NULL, since it's not guaranteed! */      
#ifdef STAT_BITVECTOR      
  for (i=0; i<EDGEFRAGSIZE; i++)
    {
      (*newedgefrag)->edge[i].entry.data.attr.edgelist = NULL;
    }
#endif    

  (*newedgefrag)->next=NULL;
  (*newedgefrag)->count=0;

  return GRL_OK;
}

  
/*............................................................*/
/* read a binary segment from disk */

graphlib_error_t grlibint_read(int fh, char *buf, int len)
{
  int sum,count;

  sum=0;
  while (sum!=len)
    {
      count=read(fh,&(buf[sum]),len-sum);
      if (count<=0)
	return GRL_FILEERROR;
      sum+=count;
    }
  return GRL_OK;
}


/*............................................................*/
/* write a binary segment from disk */

graphlib_error_t grlibint_write(int fh, char *buf, int len)
{
  int sum,count;

  sum=0;
  while (sum!=len)
    {
      count=write(fh,&(buf[sum]),len-sum);
      if (count<=0)
	return GRL_FILEERROR;
      sum+=count;
    }
  return GRL_OK;
}


/*............................................................*/
/* write len bytes of data from src to cur_pos (a marker into dest_array) -- realloc() as necessary
   - updates cur_pos after write
   - updates dest_array_len after realloc
*/

graphlib_error_t grlibint_copyDataToBuf( int * idx, const char * src, int len, 
					 char ** dest_array, int * dest_array_len )
{
////GLL: this should be as large as the max name length macro
//  static unsigned int alloc_size=GRL_MAX_NAME_LENGTH;
//  //static unsigned int alloc_size=8192;
//
//  /* check if realloc( necessary ) */
//  while( ( ((*idx) + len) > ( *dest_array_len) ) )
////GLL: we should loop on this, in the case that we need more than the alloc_size
////     Note, with the loop, we can fix the alloc_size at 8192 if we want
////  if( ( ((*idx) + len) > ( *dest_array_len) ) )
//    {
//      /* alloc in 4K chunks */
//      //BUG ALERT: realloc() asserts when tool run w/ more than 144 leaves. no more mem?????
//      *dest_array = (char*)realloc( *dest_array, (*dest_array_len)+alloc_size );
//      *dest_array_len += alloc_size;
//    }

  //GLL: 2010-03-22 - reimplementing reallocation of dest_array
  unsigned int alloc_size;
  if( ( ((*idx) + len) > ( *dest_array_len) ) )
    {
      alloc_size =  8192*( 1 + ((*idx) + len) / 8192);
      *dest_array = (char*)realloc( *dest_array, alloc_size);
      *dest_array_len = alloc_size;
    }


  char * dst = (*dest_array)+(*idx);

  memcpy( dst, src, len );
  (*idx) += len;


#ifdef DEBUG  
  fprintf( stderr, "[%s:%s():%d]: ", __FILE__, __FUNCTION__, __LINE__ );
  if( len == sizeof(int) )
    fprintf( stderr, "\tint: %d (%p) => %d (%p)\n",
	     *((int*)src), src, *((int*)dst), dst );
  else if( len == sizeof(graphlib_width_t) )
    fprintf( stderr, "\tdouble: %.2lf (%p) => %.2lf (%p)\n",
	     *((double*)src), src, *((double*)dst), dst );
  else
    fprintf( stderr, "\tstring: \"%s\" (%p) => \"%s\" (%p)\n",
	     src, src, dst, dst );
#endif
  
  return GRL_OK;
}


/*............................................................*/
/* read graph from a buffer */

graphlib_error_t grlibint_copyDataFromBuf( char * dst, int *idx, int len, char * src_array,
                                           int src_array_len )
{
  char * src=src_array+*idx;
  
  /* check for array bounds read error */
  if( ( ((*idx) + len) > ( src_array_len) ) )
    {
      return GRL_MEMORYERROR; 
    }
  
  memcpy( dst, src, len );
  (*idx) += len;
  
#ifdef DEBUG
  fprintf( stderr, "[%s:%s():%d]: ", __FILE__, __FUNCTION__, __LINE__ );
  if( len == sizeof(int) )
    fprintf( stderr, "int: %d (%p) <= %d (%p)\n", *((int*)dst), dst, *((int*)src), src );
  else if( len == sizeof(graphlib_width_t) )
    fprintf( stderr, "double: %.2lf (%p) <= %.2lf (%p)\n",
	     *((double*)dst), dst, *((double*)src), src );
  else
    fprintf( stderr, "string: \"%s\" (%p) <= \"%s\" (%p)\n",
	     dst, dst, src, src );
#endif
  return GRL_OK;
}


/*-----------------------------------------------------------------*/
/* color management */

/*............................................................*/
/* color settings for GML export */

void grlibint_exp_dot_color(graphlib_color_t color, FILE *fh)
{
  switch (color)
    {
    case GRC_FIREBRICK: 
      fprintf(fh,"\"#B22222\"");
      break;
    case GRC_YELLOW: 
      fprintf(fh,"\"#FFFF00\"");
      break;
    case GRC_ORANGE: 
      fprintf(fh,"\"#FFA500\"");
      break;
    case GRC_TAN: 
      fprintf(fh,"\"#D2B48C\"");
      break;
    case GRC_GOLDENROD: 
      fprintf(fh,"\"#DAA520\"");
      break;
    case GRC_PURPLE: 
      fprintf(fh,"\"#800080\"");
      break;
    case GRC_OLIVE: 
      fprintf(fh,"\"#556B2F\"");
      break;
    case GRC_GREY: 
      fprintf(fh,"\"#AAAAAA\"");
      break;
    case GRC_LIGHTGREY: 
      fprintf(fh,"\"#DDDDDD\"");
      break;
    case GRC_BLACK: 
      fprintf(fh,"\"#000000\"");
      break;
    case GRC_BLUE: 
      fprintf(fh,"\"#0000FF\"");
      break;
    case GRC_GREEN: 
      fprintf(fh,"\"#00FF00\"");
      break;
    case GRC_DARKGREEN: 
      fprintf(fh,"\"#009900\"");
      break;
    case GRC_RED: 
      fprintf(fh,"\"#FF0000\"");
      break;
    case GRC_WHITE: 
      fprintf(fh,"\"#FFFFFF\"");
      break;
    case GRC_RANGE1: 
      fprintf(fh,"\"#808080\"");
      break;
    case GRC_RANGE2: 
      fprintf(fh,"\"#8080A0\"");
      break;
    case GRC_RANGE3: 
      fprintf(fh,"\"#8080D0\"");
      break;
    case GRC_RANGE4: 
      fprintf(fh,"\"#8080F0\"");
      break;
    default:
      {
	if ((color>=GRC_REDSPEC) && (color<GRC_REDSPEC+GRC_SPECTRUMRANGE))
	  fprintf(fh,"\"#FF%02x%02x\"",256-(color-GRC_REDSPEC),256-(color-GRC_REDSPEC));
	else if ((color>=GRC_GREENSPEC) && (color<GRC_GREENSPEC+GRC_SPECTRUMRANGE))
	  fprintf(fh,"\"#%02xFF%02x\"",256-(color-GRC_GREENSPEC),256-(color-GRC_GREENSPEC));
	else if ((color>=GRC_RAINBOW) && (color<GRC_RAINBOW+GRC_RAINBOWCOLORS))
	  {
	    unsigned int color_val;
//GLL comment: added the +.1 b/c when the num_colors was a nice power of 2 (i.e., 2,4,8) the color_vals were all coming out as shades of one color
//	    float normalized_color_val=1.0-(((float)color-GRC_RAINBOW)/((float)grlibint_num_colors+.1));
//	    color_val=(unsigned int)(normalized_color_val*((float)0xFFFFFF));

        //GLL comment: if there are a small number of colors, use a larger 
        //normalizing value to get a better color range
        if (grlibint_num_colors < 18)
            color_val = 16777215 - ((color-GRC_RAINBOW)/18.0)*16777215;
        /* Powers of 2 number of colors create red-only colors */
        else if (grlibint_num_colors % 32 == 0)
            color_val = 16777215 - ((color-GRC_RAINBOW)/(float)(grlibint_num_colors+1))*16777215;
        else
            color_val = 16777215 - ((color-GRC_RAINBOW)/(float)grlibint_num_colors)*16777215;
	    fprintf(fh,"\"#%06x\"",color_val);
	  }
 	else
	  fprintf(fh,"\"#CCCCFF\"");
	break;
      }
    }
}


/*............................................................*/
/* color settings for DOT files with arbitrary colors */

void grlibint_exp_gml_color(graphlib_color_t color, FILE *fh)
{
  grlibint_exp_dot_color(color,fh);
  fprintf(fh,"\n");
}


/*............................................................*/
/* color settings for DOT files with reduced colors */

void grlibint_exp_plaindot_color(graphlib_color_t color, FILE *fh)
{
  if ((color>=GRC_RAINBOW) && (color<GRC_RAINBOW+GRC_RAINBOWCOLORS))
    {
      color=(color-GRC_RAINBOW) % GRL_NUM_COLORS;
    }

  switch (color)
    {
    case GRC_FIREBRICK: 
      fprintf(fh,"red");
      break;
    case GRC_YELLOW: 
      fprintf(fh,"yellow");
      break;
    case GRC_ORANGE: 
      fprintf(fh,"orange");
      break;
    case GRC_TAN: 
      fprintf(fh,"beige");
      break;
    case GRC_GOLDENROD: 
      fprintf(fh,"yellow");
      break;
    case GRC_PURPLE: 
      fprintf(fh,"purple");
      break;
    case GRC_OLIVE: 
      fprintf(fh,"green");
      break;
    case GRC_GREY: 
      fprintf(fh,"grey");
      break;
    case GRC_LIGHTGREY: 
      fprintf(fh,"grey");
      break;
    case GRC_BLACK: 
      fprintf(fh,"black");
      break;
    case GRC_BLUE: 
      fprintf(fh,"blue");
      break;
    case GRC_GREEN: 
      fprintf(fh,"green");
      break;
    case GRC_DARKGREEN: 
      fprintf(fh,"green");
      break;
    case GRC_RED: 
      fprintf(fh,"red");
      break;
    case GRC_WHITE: 
      fprintf(fh,"white");
      break;
    case GRC_RANGE1: 
      fprintf(fh,"blue");
      break;
    case GRC_RANGE2: 
      fprintf(fh,"blue");
      break;
    case GRC_RANGE3: 
      fprintf(fh,"blue");
      break;
    case GRC_RANGE4: 
      fprintf(fh,"blue");
      break;
    default:
      {
	if ((color>=GRC_REDSPEC) && (color<GRC_REDSPEC+GRC_SPECTRUMRANGE))
	  fprintf(fh,"red");
	else if ((color>=GRC_GREENSPEC) && (color<GRC_GREENSPEC+GRC_SPECTRUMRANGE))
	  fprintf(fh,"green");
	else
	  fprintf(fh,"grey");
	break;
      }
    }
}


/*............................................................*/
/* font color settings for GML export */

void grlibint_exp_dot_fontcolor(graphlib_color_t color, FILE *fh)
{
  switch (color)
    {
    case GRC_FIREBRICK: 
      fprintf(fh,"\"#000000\"");
      break;
    case GRC_YELLOW: 
      fprintf(fh,"\"#000000\"");
      break;
    case GRC_ORANGE: 
      fprintf(fh,"\"#000000\"");
      break;
    case GRC_TAN: 
      fprintf(fh,"\"#000000\"");
      break;
    case GRC_GOLDENROD: 
      fprintf(fh,"\"#000000\"");
      break;
    case GRC_PURPLE: 
      fprintf(fh,"\"#FFFFFF\"");
      break;
    case GRC_OLIVE: 
      fprintf(fh,"\"#FFFFFF\"");
      break;
    case GRC_GREY: 
      fprintf(fh,"\"#FFFFFF\"");
      break;
    case GRC_LIGHTGREY: 
      fprintf(fh,"\"#000000\"");
      break;
    case GRC_BLACK: 
      fprintf(fh,"\"#FFFFFF\"");
      break;
    case GRC_BLUE: 
      fprintf(fh,"\"#FFFFFF\"");
      break;
    case GRC_GREEN: 
      fprintf(fh,"\"#FFFFFF\"");
      break;
    case GRC_DARKGREEN: 
      fprintf(fh,"\"#FFFFFF\"");
      break;
    case GRC_RED: 
      fprintf(fh,"\"#FFFFFF\"");
      break;
    case GRC_WHITE: 
      fprintf(fh,"\"#000000\"");
      break;
    case GRC_RANGE1: 
      fprintf(fh,"\"#000000\"");
      break;
    case GRC_RANGE2: 
      fprintf(fh,"\"#000000\"");
      break;
    case GRC_RANGE3: 
      fprintf(fh,"\"#000000\"");
      break;
    case GRC_RANGE4: 
      fprintf(fh,"\"#000000\"");
      break;
    default:
      {
	if ((color>=GRC_REDSPEC) && (color<GRC_REDSPEC+GRC_SPECTRUMRANGE))
	  {
	    if ((color-GRC_REDSPEC)<(GRC_SPECTRUMRANGE/2))
	      fprintf(fh,"\"#000000\"");
	    else
	      fprintf(fh,"\"#FFFFFF\"");
	  }
	else if ((color>=GRC_GREENSPEC) && (color<GRC_GREENSPEC+GRC_SPECTRUMRANGE))
	  {
	    if ((color-GRC_GREENSPEC)<(GRC_SPECTRUMRANGE/2))
	      fprintf(fh,"\"#000000\"");
	    else
	      fprintf(fh,"\"#FFFFFF\"");
	  }
	else if ((color>=GRC_RAINBOW) && (color<GRC_RAINBOW+GRC_RAINBOWCOLORS))
	  {
	    unsigned int color_val;
	    float normalized_color_val=1.0-(((float)color-GRC_RAINBOW)/((float)grlibint_num_colors));
	    color_val=(unsigned int)(normalized_color_val*((float)0xFFFFFF));
	    if (((color_val & 0xFF0000)+(color_val && 0xFF00)+(color_val && 0xFF))>0xFF)
	      fprintf(fh,"\"#000000\"");
	    else
	      fprintf(fh,"\"#FFFFFF\"");
	  }
 	else
	  fprintf(fh,"\"#FFFFFF\"");
	break;
      }
    }
}


/*............................................................*/
/* color settings for DOT files with arbitrary colors */

void grlibint_exp_gml_fontcolor(graphlib_color_t color, FILE *fh)
{
  grlibint_exp_dot_fontcolor(color,fh);
  fprintf(fh,"\n");
}


/*............................................................*/
/* color settings for DOT files with reduced colors */

void grlibint_exp_plaindot_fontcolor(graphlib_color_t color, FILE *fh)
{
  if ((color>=GRC_RAINBOW) && (color<GRC_RAINBOW+GRC_RAINBOWCOLORS))
    {
      color=(color-GRC_RAINBOW) % GRL_NUM_COLORS;
    }

  switch (color)
    {
    case GRC_FIREBRICK: 
      fprintf(fh,"white");
      break;
    case GRC_YELLOW: 
      fprintf(fh,"black");
      break;
    case GRC_ORANGE: 
      fprintf(fh,"black");
      break;
    case GRC_TAN: 
      fprintf(fh,"black");
      break;
    case GRC_GOLDENROD: 
      fprintf(fh,"red");
      break;
    case GRC_PURPLE: 
      fprintf(fh,"white");
      break;
    case GRC_OLIVE: 
      fprintf(fh,"white");
      break;
    case GRC_GREY: 
      fprintf(fh,"white");
      break;
    case GRC_LIGHTGREY: 
      fprintf(fh,"black");
      break;
    case GRC_BLACK: 
      fprintf(fh,"white");
      break;
    case GRC_BLUE: 
      fprintf(fh,"white");
      break;
    case GRC_GREEN: 
      fprintf(fh,"black");
      break;
    case GRC_DARKGREEN: 
      fprintf(fh,"white");
      break;
    case GRC_RED: 
      fprintf(fh,"black");
      break;
    case GRC_WHITE: 
      fprintf(fh,"white");
      break;
    case GRC_RANGE1: 
      fprintf(fh,"white");
      break;
    case GRC_RANGE2: 
      fprintf(fh,"gray");
      break;
    case GRC_RANGE3: 
      fprintf(fh,"yellow");
      break;
    case GRC_RANGE4: 
      fprintf(fh,"orange");
      break;
    default:
      {
	if ((color>=GRC_REDSPEC) && (color<GRC_REDSPEC+GRC_SPECTRUMRANGE))
	  fprintf(fh,"white");
	else if ((color>=GRC_GREENSPEC) && (color<GRC_GREENSPEC+GRC_SPECTRUMRANGE))
	  fprintf(fh,"black");
	else
	  fprintf(fh,"black");
	break;
      }
    }
}


/*............................................................*/
/* dynamic color distribution */
/* THIS IS A HACK RIGHT NOW USING A GLOBAL TABLE */
/* WORKS ONLY FOR ONE GRAPH! */

#ifndef NOSET

int grlibint_getNodeColor(const void *label)
{
  unsigned int i=0;
  
  for( i=0; (i<grlibint_num_colors) && (i<GRC_RAINBOWCOLORS); i++ )
    {
#ifdef STAT_BITVECTOR
      if( bithash( node_clusters[i], grlibint_edgelabelwidth ) == bithash( label, grlibint_edgelabelwidth ) )
#else
      if( strcmp( node_clusters[i], label ) == 0 )
#endif	
	{
	  return GRC_RAINBOW+i+1;
        }
    }
  
  if( i<GRC_RAINBOWCOLORS )
    {
      //need to add new node_cluster
#ifdef STAT_BITVECTOR
      node_clusters[i] = malloc(grlibint_edgelabelwidth*bv_typesize);
      if (node_clusters[i] == NULL)
        {
          fprintf(stderr, "Failed to malloc new node cluster color\n");
	  return GRC_RAINBOW;
        }
      memcpy(node_clusters[i], label, grlibint_edgelabelwidth*bv_typesize);
#else
      node_clusters[i] = strdup( label );
#endif      
      grlibint_num_colors++;
      return GRC_RAINBOW+i+1;
    }
  
  return GRC_RAINBOW;
}

#endif

#ifdef DEBUG
void grlibint_print_color_assignment()
{
    for( unsigned int i=0; (i<grlibint_num_colors) && (i<1024); i++ ){
        fprintf( stderr, "Color[%d]: label:%s val: ",i+1,node_clusters[i] );
        grlibint_exp_dot_color( i+1, stderr );
        fprintf( stderr, "\n" );
    }
}
#endif


/*-----------------------------------------------------------------*/
/* Internal analysis */

#ifndef NOSET 

/*............................................................*/
/* extract graphs based on nodesets */

graphlib_error_t graphlibint_extractSubGraphByEdgeRank(graphlib_graph_p igraph, 
						       int irank,graphlib_graph_p * ograph)
{	
  graphlib_edgefragment_p  ef;
  graphlib_edgedata_p  e;
  graphlib_nodeentry_p n;
  graphlib_error_t err;
#ifndef STAT_BITVECTOR
#ifndef NOSET
  IntegerSet e* rset;
#endif
#endif  


#ifndef STAT_BITVECTOR
  graphlib_edgeattr_t edge_attr = {1,0,"",0,0,14};
#else  
  graphlib_edgeattr_t edge_attr = {1,0,NULL,0,0,14};
#endif

  err=graphlib_newGraph(ograph);
  if (GRL_IS_FATALERROR(err))
    return err;

  //for every edge, if name contains rank, add incident nodes
  ef=igraph->edges;
  while (ef!=NULL) 
    {
      int i;
      for (i=0; i<ef->count; i++) 
	{
	  if (ef->edge[i].full) 
	    {
	      e=&(ef->edge[i].entry.data);
#ifdef STAT_BITVECTOR
	      if (bitvec_contains(e->attr.edgelist, irank))
#else
	      rset = new IntegerSet( e->attr.name );
	      if( rset->contains( irank ) )
#endif	      
		{
		  err=grlibint_findNode(igraph,e->node_from, &n);
		  assert( GRL_IS_OK(err) );
		  graphlib_addNode( *ograph, n->entry.data.id,&(n->entry.data.attr) );
		  
		  err=grlibint_findNode(igraph,e->node_to, &n);
		  assert( GRL_IS_OK(err) );
		  graphlib_addNode( *ograph, n->entry.data.id,&(n->entry.data.attr) );
#ifdef STAT_BITVECTOR
		  graphlib_addDirectedEdge( *ograph, e->node_from,e->node_to, &(e->attr) );
#else
		  char rank_str[16];
		  snprintf( rank_str, 16, "[%u]", irank );
		  strcpy( edge_attr.name, rank_str );
		  graphlib_addDirectedEdge( *ograph, e->node_from,e->node_to, &edge_attr );
#endif		  
                }
#ifdef STAT_BITVECTOR
#else
	      delete rset;
#endif	      
            }
        }
      ef=ef->next;
    }
  
  return GRL_OK;
}

#endif

/*-----------------------------------------------------------------*/
/* Management routines */

/*............................................................*/
/* start library */

graphlib_error_t graphlib_InitVarEdgeLabelsConn(int numconn,
						int *edgelabelwidth,
						int *finalwidth)
{
#ifdef STAT_BITVECTOR
  int i;
#endif

#ifndef NOSET
  *finalwidth = grlibint_edgelabelwidth*bv_typesize*8;
#endif

  allgraphs=NULL;

#ifdef STAT_BITVECTOR
  grlibint_edgelabelwidth=0;
  grlibint_connections=numconn;
  grlibint_connwidths=(int*) malloc(sizeof(int)*numconn);
  if (grlibint_connwidths==NULL)
    return GRL_NOMEM;

  for (i=0; i<numconn; i++)
    {
      grlibint_connwidths[i]=edgelabelwidth[i] / (bv_typesize*8);
      if ((edgelabelwidth[i] % (bv_typesize*8))!=0)
	grlibint_connwidths[i]++;
      grlibint_edgelabelwidth += grlibint_connwidths[i];
    }

  if (bit_vec_initialize(bv_typesize,grlibint_edgelabelwidth))
    return GRL_NOMEM;
#endif

#ifndef NOSET
  *finalwidth = grlibint_edgelabelwidth*bv_typesize*8;
#endif

  return GRL_OK;
}

graphlib_error_t graphlib_InitVarEdgeLabels(int edgelabelwidth)
{
  graphlib_error_t err;
  int total;

  err=graphlib_InitVarEdgeLabelsConn(1,&edgelabelwidth,&total);
  return err;
}

graphlib_error_t graphlib_Init()
{
  return graphlib_InitVarEdgeLabels(GRL_DEFAULT_EDGELABELWIDTH);
}


/*............................................................*/
/* cleanup */

graphlib_error_t graphlib_Finish()
{
  return graphlib_delAll();
}


/*............................................................*/
/* add graph without annotations */

graphlib_error_t graphlib_newGraph(graphlib_graph_p *newgraph)
{
  graphlib_error_t err;

  err=grlibint_addGraph(newgraph);
  if (GRL_IS_FATALERROR(err))
    return err;

  (*newgraph)->edges=NULL;
  (*newgraph)->nodes=NULL;

  (*newgraph)->directed=0;  
  (*newgraph)->edgeset=0;  
  (*newgraph)->numannotation=0;
  (*newgraph)->annotations=NULL;  

  (*newgraph)->freenodes=NULL;
  (*newgraph)->freeedges=NULL;

  return GRL_OK;
}


/*............................................................*/
/* add graph with annotations */

graphlib_error_t graphlib_newAnnotatedGraph(graphlib_graph_p *newgraph, int numannotation)
{
  graphlib_error_t err;
  int i;

  err=grlibint_addGraph(newgraph);
  if (GRL_IS_FATALERROR(err))
    return err;

  (*newgraph)->edges=NULL;
  (*newgraph)->nodes=NULL;

  (*newgraph)->directed=0;  
  (*newgraph)->edgeset=0;  
  (*newgraph)->numannotation=numannotation;
  (*newgraph)->annotations=(char**)malloc(numannotation*sizeof(char*));  
  if ((*newgraph)->annotations==NULL)
    return GRL_NOMEM;
  for (i=0; i<numannotation; i++)
    (*newgraph)->annotations[i]=NULL;

  (*newgraph)->freenodes=NULL;
  (*newgraph)->freeedges=NULL;

  return GRL_OK;
}


/*............................................................*/
/* delete an edge attribute */

graphlib_error_t graphlib_delEdgeAttr(graphlib_edgeattr_t deledgeattr)
{
#ifdef STAT_BITVECTOR
  if (deledgeattr.edgelist != NULL)
    free(deledgeattr.edgelist);
#endif
  return GRL_OK;
}

/*............................................................*/
/* delete a graph */

graphlib_error_t graphlib_delGraph(graphlib_graph_p delgraph)
{
  graphlib_edgefragment_p deledge;
  graphlib_nodefragment_p delnode;
  graphlib_graphlist_p    graphs,oldgraphs;

  graphs=allgraphs;
  oldgraphs=NULL;
  while (graphs!=NULL)
    {
      if (graphs->graph==delgraph)
	{
	  if (oldgraphs==NULL)
	    {
	      allgraphs=graphs->next;
	      free(graphs);
	      graphs=NULL;
	    }
	  else
	    {
	      oldgraphs->next=graphs->next;
	      free(graphs);
	      graphs=NULL;
	    }
	}
      if (graphs!=NULL)
	{
	  oldgraphs=graphs;
	  graphs=graphs->next;
	}
    }

  while ((delgraph->edges)!=NULL)
    {
#ifdef STAT_BITVECTOR
      int i;
      for (i=0; i < delgraph->edges->count; i++)
        {
          if (delgraph->edges->edge[i].entry.data.attr.edgelist != NULL)
            bitvec_delete(&(delgraph->edges->edge[i].entry.data.attr.edgelist));
        }
#endif
      deledge=(delgraph->edges)->next;
      free(delgraph->edges);
      delgraph->edges=deledge;
    }
  
  while ((delgraph->nodes)!=NULL)
    {
      delnode=(delgraph->nodes)->next;
      if (delgraph->nodes->grannot!=NULL)
	free(delgraph->nodes->grannot);
      free(delgraph->nodes);
      delgraph->nodes=delnode;
    }
  
  delgraph->freenodes=NULL;
  delgraph->freeedges=NULL;
  
  free(delgraph);
  
  return GRL_OK;
}


/*............................................................*/
/* delete all graphs */

graphlib_error_t graphlib_delAll()
{
  while (allgraphs!=NULL)
    {
      graphlib_delGraph(allgraphs->graph);
    }

  return GRL_OK;
}


/*-----------------------------------------------------------------*/
/* basic routines */

/*............................................................*/
/* count nodes */

graphlib_error_t graphlib_nodeCount( graphlib_graph_p igraph, int * num_nodes )
{
	graphlib_nodefragment_p nodefrag=igraph->nodes;
    int i=0;
    *num_nodes=0;

	while (nodefrag!=NULL) {
	    for (i=0; i<nodefrag->count; i++) {
            if (nodefrag->node[i].full) {
                (*num_nodes)++;
            }
        }
        nodefrag = nodefrag->next;
    }

    return GRL_OK;
}


/*............................................................*/
/* count edges */

graphlib_error_t graphlib_edgeCount( graphlib_graph_p igraph, int * num_edges )
{
  graphlib_edgefragment_p edgefrag=igraph->edges;
  int i;
  *num_edges=0;
  
  while (edgefrag!=NULL) 
    {
      for (i=0; i<edgefrag->count; i++) 
	{
	  if (edgefrag->edge[i].full) 
	    {
	      (*num_edges)++;
            }
        }
      edgefrag = edgefrag->next;
    }
  
  return GRL_OK;
}


/*-----------------------------------------------------------------*/
/* I/O routines */

/*............................................................*/
/* load a full graph / internal format */

graphlib_error_t graphlib_loadGraph(graphlib_filename_t fn, graphlib_graph_p *newgraph)
{
  int                     fh,i;
  graphlib_error_t        err;
  graphlib_nodefragment_p *runnode,newnode;
  graphlib_edgefragment_p *runedge,newedge;
  graphlib_annotation_t    *saveattrptr;

  fh=open(fn,O_RDONLY);
  if (fh<0)
    return GRL_FILEERROR;

  err=grlibint_addGraph(newgraph);
  if (GRL_IS_FATALERROR(err))
    return err;
  
  err=grlibint_read(fh,(char*) (*newgraph), sizeof(graphlib_graph_t));
  if (GRL_IS_FATALERROR(err))
    return err;
  
  runnode=&((*newgraph)->nodes);
  runedge=&((*newgraph)->edges);
  
  while ((*runnode)!=NULL)
    {
      err=grlibint_newNodeFragment(&newnode,(*newgraph)->numannotation);
      if (GRL_IS_FATALERROR(err))
	return err;
      saveattrptr=newnode->grannot;
      err=grlibint_read(fh,(char*) newnode,sizeof(graphlib_nodefragment_t)-NODEFRAGSIZE*sizeof(graphlib_nodeentry_t));
      if (GRL_IS_FATALERROR(err))
	return err;
      err=grlibint_read(fh,(char*) (newnode->node),newnode->count*sizeof(graphlib_nodeentry_t));
      if (GRL_IS_FATALERROR(err))
	return err;
      newnode->grannot=saveattrptr;
      if ((*newgraph)->numannotation>0)
	{
	  err=grlibint_read(fh,(char*) (newnode->grannot),sizeof(graphlib_annotation_t)*
			    (*newgraph)->numannotation*(newnode->count));
	  if (GRL_IS_FATALERROR(err))
	    return err;
	}
      *runnode=newnode;
      runnode=&(newnode->next);
    }

  while ((*runedge)!=NULL)
    {
      err=grlibint_newEdgeFragment(&newedge);
      if (GRL_IS_FATALERROR(err))
	return err;
      err=grlibint_read(fh,(char*) newedge,sizeof(graphlib_edgefragment_t)-EDGEFRAGSIZE*sizeof(graphlib_edgeentry_t));
      if (GRL_IS_FATALERROR(err))
	return err;
      err=grlibint_read(fh,(char*) (newedge->edge),(newedge->count)*sizeof(graphlib_edgeentry_t));
      if (GRL_IS_FATALERROR(err))
	return err;
      *runedge=newedge;
      runedge=&(newedge->next);
#ifdef STAT_BITVECTOR
      for (i=0; i<newedge->count; i++)
	{
	  if (newedge->edge[i].entry.data.attr.edgelist)
	    {
	      newedge->edge[i].entry.data.attr.edgelist=bitvec_allocate();
	      if (newedge->edge[i].entry.data.attr.edgelist==NULL)
	        return GRL_NOMEM;
	      err=grlibint_read(fh,(char*) (newedge->edge[i].entry.data.attr.edgelist),grlibint_edgelabelwidth*bv_typesize);
	      if (GRL_IS_FATALERROR(err))
		return err;
	    }
	}
#endif
    }

  /* find links to active points */

  newedge=(*newgraph)->edges;
  while (newedge!=NULL)
    {
      for (i=0; i<newedge->count; i++)
	{
	  if (newedge->edge[i].full)
	    {
	      err=grlibint_findNode(*newgraph,newedge->edge[i].entry.data.node_from,
				    &(newedge->edge[i].entry.data.ref_from));
	      if (GRL_IS_NOTOK(err))
		return err;
	      err=grlibint_findNode(*newgraph,newedge->edge[i].entry.data.node_to,
				    &(newedge->edge[i].entry.data.ref_to));
	      if (GRL_IS_NOTOK(err))
		return err;
	    }
	} 
      newedge=newedge->next;
   }

  /* build free lists */

  (*newgraph)->freenodes=NULL;

  newnode=(*newgraph)->nodes;
  while (newnode!=NULL)
    {
      for (i=0; i<newnode->count; i++)
	{
	  if (newnode->node[i].full==0)
	    {
	      newnode->node[i].entry.freeptr=(*newgraph)->freenodes;
	      (*newgraph)->freenodes=&(newnode->node[i]);
	    }
	}
      newnode=newnode->next;
    }

  (*newgraph)->freeedges=NULL;

  newedge=(*newgraph)->edges;
  while (newedge!=NULL)
    {
      for (i=0; i<newedge->count; i++)
	{
	  if (newedge->edge[i].full==0)
	    {
	      newedge->edge[i].entry.freeptr=(*newgraph)->freeedges;
#ifdef STAT_BITVECTOR
          if (newedge->edge[i].entry.data.attr.edgelist != NULL)
            bitvec_delete(&(newedge->edge[i].entry.data.attr.edgelist));
#endif
	      (*newgraph)->freeedges=&(newedge->edge[i]);
	    }
	}
      newedge=newedge->next;
    }

  return GRL_OK;
}


/*............................................................*/
/* save a complete graph / internal format */

graphlib_error_t graphlib_saveGraph(graphlib_filename_t fn, graphlib_graph_p graph)
{
  int fh,i;
  graphlib_error_t        err;
  graphlib_nodefragment_p runnode;
  graphlib_edgefragment_p runedge;

  fh=open(fn,O_WRONLY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE|S_IRGRP|S_IROTH);
  if (fh<0)
    return GRL_FILEERROR;
  runnode=graph->nodes;
  runedge=graph->edges;

  err=grlibint_write(fh,(char*) graph, sizeof(graphlib_graph_t));
  if (GRL_IS_FATALERROR(err))
    return err;
  
  while (runnode!=NULL)
    {
      err=grlibint_write(fh,(char*) runnode,sizeof(graphlib_nodefragment_t)-NODEFRAGSIZE*sizeof(graphlib_nodeentry_t));
      if (GRL_IS_FATALERROR(err))
	return err;
	
      err=grlibint_write(fh,(char*) (runnode->node),(runnode->count)*sizeof(graphlib_nodeentry_t));
      if (GRL_IS_FATALERROR(err))
	return err;
	
      if (graph->numannotation>0)
	{
	  err=grlibint_write(fh,(char*) (runnode->grannot),sizeof(graphlib_annotation_t)*
			    (graph)->numannotation*(runnode->count));
	  if (GRL_IS_FATALERROR(err))
	    return err;
	}
      runnode=runnode->next;
    }

  while (runedge!=NULL)
    {
      err=grlibint_write(fh,(char*) runedge,sizeof(graphlib_edgefragment_t)-EDGEFRAGSIZE*sizeof(graphlib_edgeentry_t));
      if (GRL_IS_FATALERROR(err))
	return err;
      err=grlibint_write(fh,(char*) (runedge->edge),(runedge->count)*sizeof(graphlib_edgeentry_t));
      if (GRL_IS_FATALERROR(err))
	return err;
#ifdef STAT_BITVECTOR
      for (i=0; i<runedge->count; i++)
	{
	  if (runedge->edge[i].entry.data.attr.edgelist)
	    err=grlibint_write(fh,(char*) (runedge->edge[i].entry.data.attr.edgelist),grlibint_edgelabelwidth*bv_typesize);
	  if (GRL_IS_FATALERROR(err))
	    return err;
	}
#endif
      runedge=runedge->next;
    }
  
  return GRL_OK;
}


/*............................................................*/
/* export graph to new format */

graphlib_error_t graphlib_exportGraph(graphlib_filename_t fn, 
                                      graphlib_format_t format, 
                                      graphlib_graph_p graph)
{
  switch (format)
    {

    /*=====================================================*/

    case GRF_DOT:
    case GRF_PLAINDOT:
      {
	FILE                    *fh;
	graphlib_nodefragment_p  nodefrag;
	graphlib_edgefragment_p  edgefrag;
	graphlib_nodedata_p      node;
	graphlib_edgedata_p      edge;
	int                      i;

	/* open file */

	fh=fopen(fn,"w");
	if (fh==NULL)
	  return GRL_FILEERROR;

	/* write header */

	fprintf(fh,"digraph G {\n");
	fprintf(fh,"\tnode [shape=record,style=filled,labeljust=c,height=0.2];\n");

	/* write nodes */
	
	nodefrag=graph->nodes;
	while (nodefrag!=NULL)
	  {
	    for (i=0; i<nodefrag->count; i++)
	      {
		if (nodefrag->node[i].full)
		  {
		    node=&(nodefrag->node[i].entry.data);
		    {
		      /* write one node */
		      
		      fprintf(fh,"\t%i [",node->id);
		      
		      fprintf(fh,"pos=\"%i,%i\", ",node->attr.x,node->attr.y);
		      fprintf(fh,"label=\"%s\", ",node->attr.name);
		      fprintf(fh,"fillcolor=");
		      if (format==GRF_PLAINDOT)
			grlibint_exp_plaindot_color(node->attr.color,fh);
		      else
			grlibint_exp_dot_color(node->attr.color,fh);
		      fprintf(fh,",fontcolor=");
		      if (format==GRF_PLAINDOT)
			grlibint_exp_plaindot_fontcolor(node->attr.color,fh);
		      else
			grlibint_exp_dot_fontcolor(node->attr.color,fh);
		      fprintf(fh,"];\n");		  
		    }
		}
	      }
	    nodefrag=nodefrag->next;
	  }

	/* write edges */

	edgefrag=graph->edges;
	while (edgefrag!=NULL)
	  {
	    for (i=0; i<edgefrag->count; i++)
	      {
		if (edgefrag->edge[i].full)
		  {
		    edge=&(edgefrag->edge[i].entry.data);
		    {
		      /* write one edge */
		      fprintf(fh,"\t%i -> %i [",edge->node_from,edge->node_to);
#ifdef STAT_BITVECTOR
//GLL: Note we can generate a c_str from the Integer Set and let graphlib write it to
//     the file, but this string gets too large and fails
		      fprintf(fh,"label=\"");
		      bitvec_c_str_file(fh, edge->attr.edgelist);
		      fprintf(fh,"\"");
#else
		      fprintf(fh,"label=\"%s\"", edge->attr.name);
#endif		      
		      fprintf(fh,"]\n");		  
		    }
		  }
	      }
	    edgefrag=edgefrag->next;
	  }
	
	/* write footer */
	
	fprintf(fh,"}\n");

	/* done */

	fclose(fh);

	break;
      }

    /*=====================================================*/

    case GRF_GML:
      {
	FILE                    *fh;
	graphlib_nodefragment_p  nodefrag;
	graphlib_edgefragment_p  edgefrag;
	graphlib_nodedata_p      node;
	graphlib_edgedata_p      edge;
	int                      i,j;
	double                   edgescale,maxw;


	/* open file */

	fh=fopen(fn,"w");
	if (fh==NULL)
	  return GRL_FILEERROR;

	/* write header */

	fprintf(fh,"Creator \"LLNL-graphlib\"\n");
	fprintf(fh,"Version 2.2\n");
	
	fprintf(fh,"graph\n");
	fprintf(fh,"[\n");

	/* graph attributes */

	fprintf(fh,"\tdirected %i\n",graph->directed);

	/* write nodes */
	
	nodefrag=graph->nodes;
	while (nodefrag!=NULL)
	  {
	    for (i=0; i<nodefrag->count; i++)
	      {
		if (nodefrag->node[i].full)
		  {
		    node=&(nodefrag->node[i].entry.data);
		    {
		      /* write one node */
		      
		      fprintf(fh,"\tnode\n");
		      fprintf(fh,"\t[\n");
		      fprintf(fh,"\t\tid %i\n",node->id);
		      if (node->attr.name[0]==(char)0)
			{
			  if (node->attr.width!=0.0)
			    fprintf(fh,"\t\tlabel \"%.2f\"\n",node->attr.width*1000.0);
			  else
			    fprintf(fh,"\t\tlabel \"\"\n");
			}
		      else
			fprintf(fh,"\t\tlabel \"%s\"\n",node->attr.name);			

		      for (j=0; j<graph->numannotation; j++)
			{
			  if (graph->annotations[j]!=NULL)
			    {
			      fprintf(fh,"\t\t%s \"%f\"\n",graph->annotations[j],
				      nodefrag->grannot[i*graph->numannotation+j]);
			    }
			}

		      fprintf(fh,"\t\tgraphics\n");
		      fprintf(fh,"\t\t[\n");
		      fprintf(fh,"\t\t\ttype \"rectangle\"\n");
		      fprintf(fh,"\t\t\tfill ");
		      grlibint_exp_gml_color(node->attr.color,fh);
		      fprintf(fh,"\t\t\toutline \"#000000\"\n");
		      fprintf(fh,"\t\t\tx %i\n",node->attr.x);
		      fprintf(fh,"\t\t\ty %i\n",node->attr.y);
		      fprintf(fh,"\t\t\tw %f\n",node->attr.w);
		      if (node->attr.height==0)
			{
			  if (node->attr.width!=0.0)
			    fprintf(fh,"\t\t\th %f\n",20.0);
			  else
			    fprintf(fh,"\t\t\th %f\n",10.0);
			}
		      else
			fprintf(fh,"\t\t\th %f\n",node->attr.height);

		      fprintf(fh,"\t\t]\n");
		      
		      if ((node->attr.color==GRC_BLACK) ||
			  (node->attr.fontsize!=DEFAULT_FONT_SIZE))
			{
			  fprintf(fh,"\t\tLabelGraphics\n");
			  fprintf(fh,"\t\t[\n");
			  if (node->attr.name[0]==(char)0)
			    {
			      if (node->attr.width!=0.0)
				fprintf(fh,"\t\t\ttext \"%.2f\"\n",node->attr.width*1000.0);
			      else
				fprintf(fh,"\t\t\ttext \"\"\n");
			    }
			  else
			    fprintf(fh,"\t\t\ttext \"%s\"\n",node->attr.name);

			  fprintf(fh,"\t\t\tcolor ");
			  grlibint_exp_gml_fontcolor(node->attr.color,fh);
			  
			  if (node->attr.fontsize!=DEFAULT_FONT_SIZE)
			    {
			      fprintf(fh,"\t\t\tfontSize %i\n",node->attr.fontsize);
			    }
			  fprintf(fh,"\t\t]\n");
			}
		      
		      fprintf(fh,"\t]\n");		  
		    }
		}
	      }
	    nodefrag=nodefrag->next;
	  }

	/* write edges */

	maxw=1;
	edgefrag=graph->edges;
	while (edgefrag!=NULL)
	  {
	    for (i=0; i<edgefrag->count; i++)
	      {
		if (edgefrag->edge[i].full)
		  {
		    edge=&(edgefrag->edge[i].entry.data);
		    if (edge->attr.width>maxw)
		      maxw=edge->attr.width;
		  }	    
	      }
	    edgefrag=edgefrag->next;
	  }
	/* just fro QBox paper to get uniform graphs */
	/*   maxw=76; */

	edgescale=MAXEDGE_GML/maxw;

	edgefrag=graph->edges;
	while (edgefrag!=NULL)
	  {
	    for (i=0; i<edgefrag->count; i++)
	      {
		if (edgefrag->edge[i].full)
		  {
		    edge=&(edgefrag->edge[i].entry.data);
		    {
		      /* write one edge */
		      
		      fprintf(fh,"\tedge\n");
		      fprintf(fh,"\t[\n");
		      fprintf(fh,"\t\tsource %i\n",edge->node_from);
		      fprintf(fh,"\t\ttarget %i\n",edge->node_to);
		      if (edge->attr.width>0)
			fprintf(fh,"\t\tlabel \"%d\"\n", (int)edge->attr.width);
		      fprintf(fh,"\t\tgraphics\n");
		      fprintf(fh,"\t\t[\n");
		      switch (edge->attr.arcstyle)
			{
			case GRA_ARC:
			  {
			    fprintf(fh,"\t\t\ttype \"arc\"\n");
			    break;
			  }
			case GRA_SPLINE:
			  {
			    fprintf(fh,"\t\t\ttype \"spline\"\n");
			    break;
			  }
			}
		      if (edge->attr.width>0)
			fprintf(fh,"\t\t\twidth %f\n",edge->attr.width*edgescale);
		      else
			fprintf(fh,"\t\t\twidth 1.0\n");
		      fprintf(fh,"\t\t\ttargetArrow \"standard\"\n");
		      fprintf(fh,"\t\t\tfill ");
		      /* This came from Dorian's code, but I think it's no longer used */
		      /* grlibint_exp_gml_color(grlibint_num_colors,fh); */
		      grlibint_exp_gml_color(edge->attr.color,fh);
		      switch (edge->attr.arcstyle)
			{
			case GRA_ARC:
			  {
			    fprintf(fh,"\t\t\tarcType	\"fixedRatio\"\n");
			    fprintf(fh,"\t\t\tarcRatio	1.0\n");
			    break;
			  }
			case GRA_SPLINE:
			  {
			    fprintf(fh,"\t\t\tarcType	\"fixedRatio\"\n");
			    fprintf(fh,"\t\t\tarcRatio	1.0\n");
			    break;
			  }
			}
		      fprintf(fh,"\t\t]\n");

		      fprintf(fh,"\t\tLabelGraphics\n");
		      fprintf(fh,"\t\t[\n");

		      if (graph->edgeset)
			{
#ifdef STAT_BITVECTOR
//GLL: Note we can generate a c_str from the Integer Set and let graphlib write it to
//     the file, but this string gets too large and fails
                          fprintf(fh,"\t\t\ttext \"%u:", bitvec_size(edge->attr.edgelist));
		          bitvec_c_str_file(fh, edge->attr.edgelist);
                          fprintf(fh,"\"\n");
#else
#ifdef NOSET
			  fprintf(fh,"\t\t\ttext \"%s\"\n",edge->attr.name);
#else
			  IntegerSet rset( edge->attr.name );
			  fprintf(fh,"\t\t\ttext \"%u:%s\"\n",rset.size(), edge->attr.name);
#endif
#endif			  
			}
		      else
			{
#ifdef STAT_BITVECTOR
//GLL: this should output the empty list "[]"
                          fprintf(fh,"\t\t\ttext \"");
                          bitvec_c_str_file(fh, edge->attr.edgelist);
                          fprintf(fh,"\"\n");
#else
			  if (edge->attr.name[0]!=(char)0)
			    fprintf(fh,"\t\t\ttext \"%s\"\n",edge->attr.name);			
#endif			    
			}

		      fprintf(fh,"\t\t\tmodel   \"centered\"\n");
		      fprintf(fh,"\t\t\tposition        \"center\"\n");
		      if (edge->attr.fontsize>0)
			fprintf(fh,"\t\t\tfontSize %i\n",edge->attr.fontsize);
		      if ((edge->attr.block==GRB_BLOCK) || (edge->attr.block==GRB_FULL))
			{
			  fprintf(fh,"\t\t\toutline ");
			  grlibint_exp_gml_color(edge->attr.color,fh);
			}
		      if (edge->attr.block==GRB_FULL)
			{
			  fprintf(fh,"\t\t\tfill ");
			  grlibint_exp_gml_color(edge->attr.color,fh);
			}

		      fprintf(fh,"\t\t]\n");

		      fprintf(fh,"\t]\n");		
		    }  
		  }
	      }
	    edgefrag=edgefrag->next;
	  }
	
	/* write footer */
	
	fprintf(fh,"]\n");

	/* done */

	fclose(fh);

	break;
      }

    /*=====================================================*/

    default:
      {
	return GRL_UNKNOWNFORMAT;
	break;
      }
    }
      
  return GRL_OK;
}


/*............................................................*/
/* extract graphs for all edge set members individually */

#ifndef NOSET

graphlib_error_t graphlib_extractAndExportTemporalGraphs(graphlib_filename_t fn_prefix, 
							 graphlib_format_t format,graphlib_graph_p graph)
{
  graphlib_error_t err;
  graphlib_edgeentry_p edge_entry;
  
  //get sole outgoing edge of root node
  err = grlibint_findNodeEdge( graph, 0, &edge_entry );
  assert( GRL_IS_OK( err ) );
  
  //get total set of ranks based on edge name
#ifdef STAT_BITVECTOR  
  unsigned int i;
  for (i=0; i<bitvec_size(edge_entry->entry.data.attr.edgelist); i++)
#else
  IntegerSet rank_set( edge_entry->entry.data.attr.name );
  
  //for each rank, extract and export its subgraph
  for( unsigned int i=0; i<rank_set.size(); i++ )
#endif
    {
      char fn[1024];
      graphlib_graph_p subgraph;
      err = graphlibint_extractSubGraphByEdgeRank( graph, i, &subgraph );
      assert( GRL_IS_OK( err ) );
      
      snprintf( fn, 1024, "%s-rank%u.gml", fn_prefix, i );
      err = graphlib_exportGraph( fn, format, subgraph );
      assert( GRL_IS_OK( err ) );
    }
  
  return GRL_OK;
}

#endif

/*............................................................*/
/* copy graph into a serialized buffer */
/* The binary serialized format is:
   num_nodes
   num_edges
   
   for i=0:num_nodes
     node_id
     node_name_size
     node_name
     node_width

   for i=0:num_edges
     from_node_id
     to_node_id
     edge_width
*/

#ifndef NOSET

graphlib_error_t graphlib_serializeGraph( graphlib_graph_p igraph,
                                          char ** obyte_array,
                                          unsigned int * obyte_array_len )
{
  graphlib_error_t err;
  char * temp_array=NULL;
  int cur_idx, num_nodes, num_edges, name_len, temp_array_len=0;

  if (igraph->numannotation>0)
    return GRL_NOATTRIBUTE;
  
  graphlib_nodefragment_p  nodefrag;
  graphlib_edgefragment_p  edgefrag;
  graphlib_nodedata_p      node;
  graphlib_edgedata_p      edge;
  int                      i;
  //int node_count=0, edge_count=0;

  err = graphlib_nodeCount( igraph, &num_nodes );
  err = graphlib_edgeCount( igraph, &num_edges);
  
  cur_idx = 0;
  
  /* write header */
  grlibint_copyDataToBuf( &cur_idx, (const char *)&num_nodes, sizeof(int), &temp_array, &temp_array_len );
  grlibint_copyDataToBuf( &cur_idx, (const char *)&num_edges, sizeof(int), &temp_array, &temp_array_len );
  
  
  /* write nodes */
  nodefrag = igraph->nodes;
  while (nodefrag!=NULL) 
    {
      for (i=0; i<nodefrag->count; i++) 
	{
	  if (nodefrag->node[i].full) 
	    {
	      /* write one node */
	      node = &(nodefrag->node[i].entry.data);
	      
	      /* id */
	      grlibint_copyDataToBuf( &cur_idx,(const char *)&(node->id),sizeof(graphlib_node_t),
                                   &temp_array,&temp_array_len );
                    
	      /* name */
	      name_len = strlen( node->attr.name );
	      name_len++; /* for null terminator */
	      grlibint_copyDataToBuf( &cur_idx,(const char *)&(name_len),sizeof(int),
				      &temp_array,&temp_array_len );

#ifdef GRL_DYNAMIC_NODE_NAME
	      grlibint_copyDataToBuf( &cur_idx,(const char *)(node->attr.name),name_len,
				      &temp_array,&temp_array_len );
#else
	      grlibint_copyDataToBuf( &cur_idx,(const char *)&(node->attr.name),name_len,
				      &temp_array,&temp_array_len );
#endif
	      
	      /* width */
	      grlibint_copyDataToBuf( &cur_idx,(const char *)&(node->attr.width),sizeof(graphlib_width_t),
				      &temp_array,&temp_array_len );
            }
        }
      nodefrag=nodefrag->next;
    }
  
  /* write edges */
  edgefrag = igraph->edges;
  while (edgefrag!=NULL) 
    {
      for (i=0; i<edgefrag->count; i++) 
	{
	  if (edgefrag->edge[i].full) 
	    {
	      /* write one edge */
	      edge=&(edgefrag->edge[i].entry.data);
	      
	      /* from_id */
	      grlibint_copyDataToBuf( &cur_idx,(const char *)&(edge->node_from),sizeof(graphlib_node_t),
                                   &temp_array,&temp_array_len );
               
	      /* to_id */
	      grlibint_copyDataToBuf( &cur_idx,(const char *)&(edge->node_to),sizeof(graphlib_node_t),
				      &temp_array,&temp_array_len );

	      /* name */
#ifdef STAT_BITVECTOR
              name_len = grlibint_edgelabelwidth*bv_typesize;
#else
	      name_len = strlen( edge->attr.name );
	      name_len++; /* for null terminator */
#endif	      
	      grlibint_copyDataToBuf( &cur_idx,(const char *)&(name_len),sizeof(int),
				      &temp_array,&temp_array_len );

#ifdef STAT_BITVECTOR
	      grlibint_copyDataToBuf( &cur_idx,(const char *)(edge->attr.edgelist),name_len,
				      &temp_array,&temp_array_len );
#endif

	      /* width */
	      grlibint_copyDataToBuf( &cur_idx,(const char *)&(edge->attr.width),sizeof(graphlib_width_t),
				      &temp_array,&temp_array_len );
	    }
        }
      edgefrag=edgefrag->next;
    }
  
  *obyte_array = temp_array;
  *obyte_array_len = temp_array_len;
  return GRL_OK;
}

#endif

/*............................................................*/
/* copy graph from a serialized buffer */

#ifndef NOSET

graphlib_error_t graphlib_deserializeGraphConn(int conn, 
					       graphlib_graph_p * ograph,
					       char * ibyte_array,
					       unsigned int ibyte_array_len )
{
  graphlib_error_t err;
  graphlib_nodeattr_t node_attr = {0,0,0,0,0,0,"",14};

  int offset;

#ifdef STAT_BITVECTOR  
  graphlib_edgeattr_t edge_attr = {1,0,NULL,0,0,14};
#else
  graphlib_edgeattr_t edge_attr = {1,0,"",0,0,14};
#endif

  int num_nodes, num_edges, i, id=0, from_id=0, to_id=0, name_len, cur_idx;
  graphlib_width_t width;

  void *edgelist;
#ifdef GRL_DYNAMIC_NODE_NAME
  char *nodename;
#else  
  char nodename[GRL_MAX_NAME_LENGTH];
#endif

  offset=0;
  for (i=0; i<conn; i++)
    offset+=grlibint_connwidths[i];
  
  err=graphlib_newGraph(ograph);
  if (GRL_IS_FATALERROR(err))
    return err;

  cur_idx=0;
  /* read header */
  grlibint_copyDataFromBuf( (char *)&num_nodes, &cur_idx, sizeof(int), ibyte_array, ibyte_array_len );
  grlibint_copyDataFromBuf( (char *)&num_edges, &cur_idx, sizeof(int), ibyte_array, ibyte_array_len );


  /* read nodes */
  for( i=0; i<num_nodes; i++ ) 
    {
      /* id */
      grlibint_copyDataFromBuf( (char *)&id, &cur_idx,sizeof(graphlib_node_t),
				ibyte_array, ibyte_array_len );
      
      /* name */
      grlibint_copyDataFromBuf( (char *)&name_len,&cur_idx,sizeof(int),
				ibyte_array,ibyte_array_len );

#ifdef GRL_DYNAMIC_NODE_NAME
      nodename = (char *)malloc(name_len);
#endif
                    
#ifdef GRL_DYNAMIC_NODE_NAME
      grlibint_copyDataFromBuf( (char *)nodename,&cur_idx,name_len,
				ibyte_array,ibyte_array_len );
      node_attr.name = nodename;
#else
      grlibint_copyDataFromBuf( (char *)&nodename,&cur_idx,name_len,
				ibyte_array,ibyte_array_len );
      strncpy( node_attr.name, nodename, name_len );
#endif
                    
      /* width */
      grlibint_copyDataFromBuf( (char *)&width,&cur_idx,sizeof(graphlib_width_t),
                                  ibyte_array,ibyte_array_len );
      node_attr.width = width;
      
      graphlib_addNode( *ograph, id, &node_attr );
    }

  /* read edges */
  for( i=0; i<num_edges; i++ ) 
    {
      /* from_id */
      grlibint_copyDataFromBuf( (char *)&from_id,&cur_idx,sizeof(graphlib_node_t),
				ibyte_array,ibyte_array_len );
      
      /* to_id */
      grlibint_copyDataFromBuf( (char *)&to_id,&cur_idx,sizeof(graphlib_node_t),
                                  ibyte_array,ibyte_array_len );
      
      /* name */
      grlibint_copyDataFromBuf( (char *)&name_len,&cur_idx,sizeof(int),
				ibyte_array,ibyte_array_len );
                    
#ifdef STAT_BITVECTOR
      edgelist=bitvec_allocate();
      if (edgelist==NULL)
        return GRL_NOMEM;
      grlibint_copyDataFromBuf( (char *)&(((bv_type *)edgelist)[offset]),&cur_idx,name_len,
				ibyte_array,ibyte_array_len );
      edge_attr.edgelist=edgelist;
#else
      strncpy( edge_attr.name, name, name_len );
      grlibint_copyDataFromBuf( (char *)&(edgelist[offset]),&cur_idx,name_len,
				ibyte_array,ibyte_array_len );
#endif      
      
      /* width */
      grlibint_copyDataFromBuf( (char *)&width,&cur_idx,sizeof(graphlib_width_t),
                                  ibyte_array,ibyte_array_len );
      
      edge_attr.width = width;
      graphlib_addDirectedEdge( *ograph, from_id, to_id, &edge_attr );
    }

    return GRL_OK;
}


graphlib_error_t graphlib_deserializeGraph(graphlib_graph_p * ograph,
					   char * ibyte_array,
					   unsigned int ibyte_array_len )
{
  return graphlib_deserializeGraphConn(0,ograph,ibyte_array,ibyte_array_len);
}

#endif

/*-----------------------------------------------------------------*/
/* Manipulation routines */

graphlib_error_t graphlib_addNode(graphlib_graph_p graph, graphlib_node_t node,
                                  graphlib_nodeattr_p attr)

{
  graphlib_nodefragment_p  newfrag;
  graphlib_nodeentry_p     entry;
  graphlib_error_t         err;
  int                      newnode;

#ifdef FASTPATH
  err=GRL_NONODE;
#else
 err=grlibint_findNode(graph,node,&entry);
#endif
  if (err==GRL_NONODE)
    {
      if (graph->freenodes!=NULL)
	{
	  /* reuse one node from freelist */
	  
	  entry=graph->freenodes;
	  graph->freenodes=entry->entry.freeptr;
	}
      else
	{
	  /* allocate new node */
      
	  if (graph->nodes==NULL)
	    {
	      err=grlibint_newNodeFragment(&(graph->nodes),graph->numannotation);
	      if (GRL_IS_FATALERROR(err))
		return err;
	    }
	  
	  if ((graph->nodes)->count==NODEFRAGSIZE)
	    {
	      err=grlibint_newNodeFragment(&newfrag,graph->numannotation);
	      if (GRL_IS_FATALERROR(err))
		return err;
	      newfrag->next=graph->nodes;
	      graph->nodes=newfrag;
	    }

	  entry=&((graph->nodes)->node[(graph->nodes)->count]);
	  (graph->nodes)->count += 1;
	}

      entry->entry.data.id=node;
      entry->full=1;
      newnode=1;
    }
  else
    {
      if (GRL_IS_FATALERROR(err))
	return err;
      newnode=0;
    }

  if (attr!=NULL)
    {
      if (newnode)
	{
	  entry->entry.data.attr=*attr;
	  entry->entry.data.attr.width=attr->width;
	  entry->entry.data.attr.w=attr->width;
	  entry->entry.data.attr.height=attr->height;
	}
      else
	{
	  entry->entry.data.attr.color=attr->color;
	  entry->entry.data.attr.x=attr->x;
	  entry->entry.data.attr.y=attr->y;
	  if (attr->w>entry->entry.data.attr.w)
	    entry->entry.data.attr.w=attr->w;
	  if (attr->width>entry->entry.data.attr.width)
	    entry->entry.data.attr.width=attr->width;
	  if (attr->height>entry->entry.data.attr.height)
	    entry->entry.data.attr.height=attr->height;
	}
#ifdef GRL_DYNAMIC_NODE_NAME
      entry->entry.data.attr.name = strdup(attr->name);
#else
      strncpy(entry->entry.data.attr.name,attr->name,GRL_MAX_NAME_LENGTH);
#endif
      err=GRL_OK;
    }
  else
    {
      err=graphlib_setDefNodeAttr(&(entry->entry.data.attr));
    }
  
  return err;
}


/*............................................................*/

graphlib_error_t graphlib_addNodeNoCheck(graphlib_graph_p graph, graphlib_node_t node,
                                  graphlib_nodeattr_p attr)

{
  graphlib_nodefragment_p  newfrag;
  graphlib_nodeentry_p     entry;
  graphlib_error_t         err;
  int                      newnode;

  err=GRL_NONODE;
  if (err==GRL_NONODE)
    {
      if (graph->freenodes!=NULL)
	{
	  /* reuse one node from freelist */
	  
	  entry=graph->freenodes;
	  graph->freenodes=entry->entry.freeptr;
	}
      else
	{
	  /* allocate new node */
      
	  if (graph->nodes==NULL)
	    {
	      err=grlibint_newNodeFragment(&(graph->nodes),graph->numannotation);
	      if (GRL_IS_FATALERROR(err))
		return err;
	    }
	  
	  if ((graph->nodes)->count==NODEFRAGSIZE)
	    {
	      err=grlibint_newNodeFragment(&newfrag,graph->numannotation);
	      if (GRL_IS_FATALERROR(err))
		return err;
	      newfrag->next=graph->nodes;
	      graph->nodes=newfrag;
	    }

	  entry=&((graph->nodes)->node[(graph->nodes)->count]);
	  (graph->nodes)->count += 1;
	}

      entry->entry.data.id=node;
      entry->full=1;
      newnode=1;
    }
  else
    {
      if (GRL_IS_FATALERROR(err))
	return err;
      newnode=0;
    }

  if (attr!=NULL)
    {
      if (newnode)
	{
	  entry->entry.data.attr=*attr;
	  entry->entry.data.attr.width=attr->width;
	  entry->entry.data.attr.w=attr->width;
	  entry->entry.data.attr.height=attr->height;
	}
      else
	{
	  entry->entry.data.attr.color=attr->color;
	  entry->entry.data.attr.x=attr->x;
	  entry->entry.data.attr.y=attr->y;
	  if (attr->w>entry->entry.data.attr.w)
	    entry->entry.data.attr.w=attr->w;
	  if (attr->width>entry->entry.data.attr.width)
	    entry->entry.data.attr.width=attr->width;
	  if (attr->height>entry->entry.data.attr.height)
	    entry->entry.data.attr.height=attr->height;
	}
      strncpy(entry->entry.data.attr.name,attr->name,GRL_MAX_NAME_LENGTH);
      err=GRL_OK;
    }
  else
    {
      err=graphlib_setDefNodeAttr(&(entry->entry.data.attr));
    }
  
  return err;
}


/*............................................................*/

graphlib_error_t graphlib_addUndirectedEdge(graphlib_graph_p graph, 
					    graphlib_node_t node1, 
					    graphlib_node_t node2,
					    graphlib_edgeattr_p attr)
{
  graphlib_error_t err;
  int              directed;
  
  directed=graph->directed;
  err=graphlib_addDirectedEdge(graph,node1,node2,attr);
  if (GRL_IS_FATALERROR(err))
    return err;
  err=graphlib_addDirectedEdge(graph,node2,node1,attr);
  graph->directed=directed;
  return err;
}


/*............................................................*/

graphlib_error_t graphlib_addUndirectedEdgeNoCheck(graphlib_graph_p graph, 
						   graphlib_node_t node1, 
						   graphlib_node_t node2,
						   graphlib_edgeattr_p attr)
{
  graphlib_error_t err;
  int              directed;
  
  directed=graph->directed;
  err=graphlib_addDirectedEdgeNoCheck(graph,node1,node2,attr);
  if (GRL_IS_FATALERROR(err))
    return err;
  err=graphlib_addDirectedEdgeNoCheck(graph,node2,node1,attr);
  graph->directed=directed;
  return err;
}


/*............................................................*/

graphlib_error_t graphlib_addDirectedEdge(graphlib_graph_p graph, 
					  graphlib_node_t node1, 
					  graphlib_node_t node2,
					  graphlib_edgeattr_p attr)
{
  graphlib_edgefragment_p  newfrag;
  graphlib_edgeentry_p     entry;
  graphlib_nodeentry_p     noderef1=NULL;
  graphlib_nodeentry_p     noderef2=NULL;
  graphlib_error_t         err;

#ifdef FASTPATH
  err=GRL_NOEDGE;
#else
  err=grlibint_findEdge(graph,node1,node2,&entry);
#endif
  if (err==GRL_NOEDGE)
    {
      #ifdef FASTPATH
      err=GRL_OK;
      #else
      err=grlibint_findNode(graph,node1,&noderef1);
      if ((GRL_IS_FATALERROR(err))||(err==GRL_NONODE))
	return err;

      err=grlibint_findNode(graph,node2,&noderef2);
      if ((GRL_IS_FATALERROR(err))||(err==GRL_NONODE))
	return err;
      #endif

      if (graph->freeedges!=NULL)
	{
	  /* reuse edge from free list */

	  entry=graph->freeedges;
	  graph->freeedges=entry->entry.freeptr;
	}
      else
	{
	  /* allocate new entry */
	    
	  if (graph->edges==NULL)
	    {
	      err=grlibint_newEdgeFragment(&(graph->edges));
	      if (GRL_IS_FATALERROR(err))
		return err;
	    }
	  
	  if ((graph->edges)->count==EDGEFRAGSIZE)
	    {
	      err=grlibint_newEdgeFragment(&newfrag);
	      if (GRL_IS_FATALERROR(err))
		return err;
	      newfrag->next=graph->edges;
	      graph->edges=newfrag;
	    }
      
	  entry=&((graph->edges)->edge[(graph->edges)->count]);
	  (graph->edges)->count += 1;
	}

      entry->entry.data.node_from=node1;
      entry->entry.data.node_to=node2;
      entry->entry.data.ref_from=noderef1;
      entry->entry.data.ref_to=noderef2;
      entry->full=1;
    }
  else
    {
      if (GRL_IS_FATALERROR(err))
	return err;
    }

  if (attr!=NULL)
    {
#ifdef STAT_BITVECTOR      
      if (entry->entry.data.attr.edgelist != NULL)
        bitvec_delete(&(entry->entry.data.attr.edgelist));
#endif      
      entry->entry.data.attr=*attr;
#ifdef STAT_BITVECTOR      
      entry->entry.data.attr.edgelist=bitvec_allocate();
      if (entry->entry.data.attr.edgelist==NULL)
        return GRL_NOMEM;
      memcpy(entry->entry.data.attr.edgelist, attr->edgelist, grlibint_edgelabelwidth*bv_typesize);
#endif      
      err=GRL_OK;
    }
  else
    {
      err=graphlib_setDefEdgeAttr(&(entry->entry.data.attr));
    }
  
  graph->directed=1;
  
  return err;
}


/*............................................................*/

graphlib_error_t graphlib_addDirectedEdgeNoCheck(graphlib_graph_p graph, 
						 graphlib_node_t node1, 
						 graphlib_node_t node2,
						 graphlib_edgeattr_p attr)
{
  graphlib_edgefragment_p  newfrag;
  graphlib_edgeentry_p     entry;
  graphlib_nodeentry_p     noderef1=NULL;
  graphlib_nodeentry_p     noderef2=NULL;
  graphlib_error_t         err;

  err=GRL_NOEDGE;
  if (err==GRL_NOEDGE)
    {
      err=GRL_OK;

      if (graph->freeedges!=NULL)
	{
	  /* reuse edge from free list */

	  entry=graph->freeedges;
	  graph->freeedges=entry->entry.freeptr;
	}
      else
	{
	  /* allocate new entry */
	    
	  if (graph->edges==NULL)
	    {
	      err=grlibint_newEdgeFragment(&(graph->edges));
	      if (GRL_IS_FATALERROR(err))
		return err;
	    }
	  
	  if ((graph->edges)->count==EDGEFRAGSIZE)
	    {
	      err=grlibint_newEdgeFragment(&newfrag);
	      if (GRL_IS_FATALERROR(err))
		return err;
	      newfrag->next=graph->edges;
	      graph->edges=newfrag;
	    }
      
	  entry=&((graph->edges)->edge[(graph->edges)->count]);
	  (graph->edges)->count += 1;
	}

      entry->entry.data.node_from=node1;
      entry->entry.data.node_to=node2;
      entry->entry.data.ref_from=noderef1;
      entry->entry.data.ref_to=noderef2;
      entry->full=1;
    }
  else
    {
      if (GRL_IS_FATALERROR(err))
	return err;
    }

  if (attr!=NULL)
    {
      entry->entry.data.attr=*attr;
#ifdef STAT_BITVECTOR      
      entry->entry.data.attr.edgelist=bitvec_allocate();
      if (entry->entry.data.attr.edgelist==NULL)
        return GRL_NOMEM;
      memcpy(entry->entry.data.attr.edgelist, attr->edgelist, grlibint_edgelabelwidth*bv_typesize);
#endif      
      err=GRL_OK;
    }
  else
    {
      err=graphlib_setDefEdgeAttr(&(entry->entry.data.attr));
    }
  
  graph->directed=1;
  
  return err;
}


/*............................................................*/
/* delete a node and all its connections */

graphlib_error_t graphlib_deleteConnectedNode(graphlib_graph_p graph, 
					      graphlib_node_t node)
{
  graphlib_error_t     err;
  graphlib_nodeentry_p nodedel;
  graphlib_edgeentry_p edgedel;

  err=grlibint_findNode(graph,node,&nodedel);
  if (err!=GRL_OK)
    return err;

  do
    {
      err=grlibint_findNodeEdge(graph,node,&edgedel);
      if (err==GRL_OK)
	err=grlibint_delEdge(graph,edgedel);
    }
  while (err==GRL_OK);

  if (err==GRL_NOEDGE)
    {
      return grlibint_delNode(graph,nodedel);
    }
  else
    return err;
}


/*............................................................*/
/* standard graph merge - overwrite doubles */

graphlib_error_t graphlib_mergeGraphs(graphlib_graph_p graph1, 
                                      graphlib_graph_p graph2)
{
  int                     err,i,directed;
  graphlib_nodefragment_p runnode;
  graphlib_edgefragment_p runedge;

  runnode=graph2->nodes;
  runedge=graph2->edges;
  
  if ((graph1->directed==0) &&
      (graph2->directed==0))
    directed=0;
  else
    directed=1;

  while (runnode!=NULL)
    {
      for (i=0; i<runnode->count; i++)
	{
	  if (runnode->node[i].full)
	    {
	      err=graphlib_addNode(graph1,runnode->node[i].entry.data.id,
				   &((runnode->node[i]).entry.data.attr));
	      if (GRL_IS_FATALERROR(err))
		return err;
	    }
	}
      runnode=runnode->next;
    }

  while (runedge!=NULL)
    {
      for (i=0; i<runedge->count; i++)
	{
	  if (runedge->edge[i].full)
	    {
	      err=graphlib_addDirectedEdge(graph1,runedge->edge[i].entry.data.node_from,
					   runedge->edge[i].entry.data.node_to,
					   &((runedge->edge[i]).entry.data.attr));
	      if (GRL_IS_FATALERROR(err))
		return err;
	    }
	}
      runedge=runedge->next;
    }  

  graph1->directed=directed;

  return GRL_OK;
}


/*............................................................*/
/* graph merge by adding widths */

graphlib_error_t graphlib_mergeGraphsWeighted(graphlib_graph_p graph1, 
					      graphlib_graph_p graph2)
{
  int  err,i,directed;
  graphlib_nodefragment_p runnode;
  graphlib_edgefragment_p runedge;
  graphlib_edgeentry_p    edgeentry;
  graphlib_nodeentry_p    nodeentry;

  runnode=graph2->nodes;
  runedge=graph2->edges;
  
  if ((graph1->directed==0) &&
      (graph2->directed==0))
    directed=0;
  else
    directed=1;
  
  while (runnode!=NULL) 
    {
      for (i=0; i<runnode->count; i++) 
	{
          if (runnode->node[i].full) 
	    {
              err=grlibint_findNode(graph1,runnode->node[i].entry.data.id, &nodeentry);

              if (err == GRL_OK ) 
		{
		  //widths must be combined
                  runnode->node[i].entry.data.attr.width += nodeentry->entry.data.attr.width;
		}
              err=graphlib_addNode(graph1,runnode->node[i].entry.data.id,
                                   &((runnode->node[i]).entry.data.attr));
              if (GRL_IS_FATALERROR(err))
		return err;
	    }
	}
      runnode=runnode->next;
    }
  
  while (runedge!=NULL) 
    {
      for (i=0; i<runedge->count; i++) 
	{
          if (runedge->edge[i].full) 
	    {
              err=grlibint_findEdge(graph1,runedge->edge[i].entry.data.node_from,
                                    runedge->edge[i].entry.data.node_to,&edgeentry);
              if (err==GRL_OK)
		{
                  //widths must be combined
                  runedge->edge[i].entry.data.attr.width += edgeentry->entry.data.attr.width;
		}
              err=graphlib_addDirectedEdge(graph1,runedge->edge[i].entry.data.node_from,
                                           runedge->edge[i].entry.data.node_to,
                                           &((runedge->edge[i]).entry.data.attr));
              if (GRL_IS_FATALERROR(err))
		return err;
	    }
	}
      runedge=runedge->next;
    }  
  
  graph1->directed=directed;
  
  return GRL_OK;
}


/*............................................................*/
/* graph merge by adding widths and combine edge sets */

#ifndef NOSET

graphlib_error_t graphlib_mergeGraphsRanked(graphlib_graph_p graph1, 
					    graphlib_graph_p graph2)
{
  int  err,i,directed;
  graphlib_nodefragment_p runnode;
  graphlib_edgefragment_p runedge;
  graphlib_edgeentry_p     edgeentry;
  graphlib_nodeentry_p     nodeentry;

  runnode=graph2->nodes;
  runedge=graph2->edges;
  
  if ((graph1->directed==0) &&
      (graph2->directed==0))
    directed=0;
  else
    directed=1;

  graph1->edgeset=1;
  
  while (runnode!=NULL) 
    {
      for (i=0; i<runnode->count; i++) 
	{
	  if (runnode->node[i].full) 
	    {
	      err=grlibint_findNode(graph1,runnode->node[i].entry.data.id, &nodeentry);
              if (err == GRL_OK ) 
		{
		  //widths must be combined
                  runnode->node[i].entry.data.attr.width += nodeentry->entry.data.attr.width;
		}
              err=graphlib_addNode(graph1,runnode->node[i].entry.data.id,
                                   &((runnode->node[i]).entry.data.attr));
              if (GRL_IS_FATALERROR(err))
		return err;
	    }
	}
      runnode=runnode->next;
    }
  
  while (runedge!=NULL) 
    {
      for (i=0; i<runedge->count; i++) 
	{
          if (runedge->edge[i].full) 
	    {
              err=grlibint_findEdge(graph1,runedge->edge[i].entry.data.node_from,
                                    runedge->edge[i].entry.data.node_to,&edgeentry);
              if (err==GRL_OK)
		{
                  //names must be combined
#ifdef STAT_BITVECTOR
		  bitvec_merge(runedge->edge[i].entry.data.attr.edgelist, edgeentry->entry.data.attr.edgelist);
#else
                  IntegerSet rank_set;
                  rank_set.insert(runedge->edge[i].entry.data.attr.name);
                  rank_set.insert(edgeentry->entry.data.attr.name);
                  strncpy(runedge->edge[i].entry.data.attr.name,
                           rank_set.c_str(), GRL_MAX_NAME_LENGTH);
#endif			   
		}
              err=graphlib_addDirectedEdge(graph1,runedge->edge[i].entry.data.node_from,
                                           runedge->edge[i].entry.data.node_to,
                                           &((runedge->edge[i]).entry.data.attr));
              if (GRL_IS_FATALERROR(err))
		return err;
	    }
	}
      runedge=runedge->next;
    }  

  graph1->directed=directed;

  return GRL_OK;
}

#endif

/*............................................................*/
/* graph merge by with empty edge sets */

graphlib_error_t graphlib_mergeGraphsEmptyEdges(graphlib_graph_p graph1, 
					        graphlib_graph_p graph2)
{
  int  err,i,directed;
  graphlib_nodefragment_p runnode;
  graphlib_edgefragment_p runedge;
  graphlib_nodeentry_p     nodeentry;

  runnode=graph2->nodes;
  runedge=graph2->edges;
  
  if ((graph1->directed==0) &&
      (graph2->directed==0))
    directed=0;
  else
    directed=1;

  graph1->edgeset=1;
  
  while (runnode!=NULL) 
    {
      for (i=0; i<runnode->count; i++) 
	{
	  if (runnode->node[i].full) 
	    {
	      err=grlibint_findNode(graph1,runnode->node[i].entry.data.id, &nodeentry);
              if (err == GRL_OK ) 
		{
		  //widths must be combined
                  runnode->node[i].entry.data.attr.width += nodeentry->entry.data.attr.width;
		}
              err=graphlib_addNode(graph1,runnode->node[i].entry.data.id,
                                   &((runnode->node[i]).entry.data.attr));
              if (GRL_IS_FATALERROR(err))
		return err;
	    }
	}
      runnode=runnode->next;
    }
  
  while (runedge!=NULL) 
    {
      for (i=0; i<runedge->count; i++) 
	{
          if (runedge->edge[i].full) 
	    {
              /* Add directed edge with NULL attr. This will get us the default empty edge */
              err=graphlib_addDirectedEdge(graph1,runedge->edge[i].entry.data.node_from,
                                           runedge->edge[i].entry.data.node_to,
                                           NULL);
              if (GRL_IS_FATALERROR(err))
		return err;
	    }
	}
      runedge=runedge->next;
    }  

  graph1->directed=directed;

  return GRL_OK;
}


/*............................................................*/
/* graph merge by adding widths and fill edge sets */

graphlib_error_t graphlib_mergeGraphsFillEdges(graphlib_graph_p graph1, 
					       graphlib_graph_p graph2,
					       int *ranks,
					       int ranks_size,
					       int offset)
{
  int  err,i,j,directed;
#ifdef STAT_BITVECTOR
  int start;
#endif
  graphlib_nodefragment_p runnode;
  graphlib_edgefragment_p runedge;
  graphlib_edgeentry_p     edgeentry;
  graphlib_nodeentry_p     nodeentry;

  runnode=graph2->nodes;
  runedge=graph2->edges;


#ifdef STAT_BITVECTOR
 /* Find the starting bit number */
  start=offset*bv_typesize*8;
#endif
  
  if ((graph1->directed==0) &&
      (graph2->directed==0))
    directed=0;
  else
    directed=1;

  graph1->edgeset=1;
  
  while (runnode!=NULL) 
    {
      for (i=0; i<runnode->count; i++) 
	{
	  if (runnode->node[i].full) 
	    {
	      err=grlibint_findNode(graph1,runnode->node[i].entry.data.id, &nodeentry);
              if (err == GRL_OK ) 
		{
		  //widths must be combined
                  runnode->node[i].entry.data.attr.width += nodeentry->entry.data.attr.width;
		}
              err=graphlib_addNode(graph1,runnode->node[i].entry.data.id,
                                   &((runnode->node[i]).entry.data.attr));
              if (GRL_IS_FATALERROR(err))
		return err;
	    }
	}
      runnode=runnode->next;
    }
  
  while (runedge!=NULL) 
    {
      for (i=0; i<runedge->count; i++) 
	{
          if (runedge->edge[i].full) 
	    {
              err=grlibint_findEdge(graph1,runedge->edge[i].entry.data.node_from,
                                    runedge->edge[i].entry.data.node_to,&edgeentry);
              if (err==GRL_OK)
		{
#ifdef STAT_BITVECTOR
                  /* Check for bits set in graph2 and map by rank into bit vector of graph 1 */
                  for (j=0; j<ranks_size; j++)
    	            {
    	              if (bitvec_contains(runedge->edge[i].entry.data.attr.edgelist, j+start) == 1)
    		        bitvec_insert(edgeentry->entry.data.attr.edgelist, ranks[j]);
    	            }
#endif			   
		}
              if (GRL_IS_FATALERROR(err))
		return err;
	    }
	}
      runedge=runedge->next;
    }  

  graph1->directed=directed;

  return GRL_OK;
}

/*-----------------------------------------------------------------*/
/* Attribute routines */

/*............................................................*/
/* set default node attributes */

graphlib_error_t graphlib_setDefNodeAttr(graphlib_nodeattr_p attr)
{
  attr->width=DEFAULT_NODE_WIDTH;
  attr->height=DEFAULT_NODE_WIDTH;
  attr->w=DEFAULT_NODE_WIDTH;
  attr->color=DEFAULT_NODE_COLOR;
  attr->x=DEFAULT_NODE_COOR;
  attr->y=DEFAULT_NODE_COOR;
#ifdef GRL_DYNAMIC_NODE_NAME
  attr->name = strdup("");
#else
  strcpy(attr->name,"");
#endif
  attr->fontsize=DEFAULT_FONT_SIZE;

  return GRL_OK;
}


/*............................................................*/
/* set default node attributes */

graphlib_error_t graphlib_setDefEdgeAttr(graphlib_edgeattr_p attr)
{
  attr->width=DEFAULT_EDGE_WIDTH;
  attr->color=DEFAULT_EDGE_COLOR;
#ifdef STAT_BITVECTOR
  if (attr->edgelist==NULL)
    {
      attr->edgelist=bitvec_allocate();
      if (attr->edgelist==NULL)
	return GRL_NOMEM;
    }
  else
    {
      bitvec_erase(attr->edgelist);
    }

#else
  strcpy(attr->name,"");
#endif  
  attr->arcstyle=DEFAULT_EDGE_STYLE;
  attr->block=DEFAULT_BLOCK;
  attr->fontsize=DEFAULT_FONT_SIZE;

  return GRL_OK;
}


/*............................................................*/

graphlib_error_t graphlib_scaleNodeWidth(graphlib_graph_p graph, 
					 graphlib_width_t minval,
					 graphlib_width_t maxval)
{
  int                     i,first;
  graphlib_width_t        searchmin=0,searchmax=0;
  graphlib_nodefragment_p nodefrag;

  first=1;

  nodefrag=graph->nodes;
  while (nodefrag!=NULL)
    {
      for (i=0; i<nodefrag->count; i++)
	{
	  if (nodefrag->node[i].full)
	    {
	      if ((nodefrag->node[i].entry.data.attr.width<searchmin)||(first))
		{
		  searchmin=nodefrag->node[i].entry.data.attr.width;
		}
	      if ((nodefrag->node[i].entry.data.attr.width>searchmax)||(first))
		{
		  searchmax=nodefrag->node[i].entry.data.attr.width;
		}
	      first=0;
	    }
	}
      nodefrag=nodefrag->next;
    }
  
  nodefrag=graph->nodes;
  while (nodefrag!=NULL)
    {
      for (i=0; i<nodefrag->count; i++)
	{
	  if (nodefrag->node[i].full)
	    {
	      if (searchmin==searchmax)
		nodefrag->node[i].entry.data.attr.w=(maxval+minval)/2.0;
	      else
		nodefrag->node[i].entry.data.attr.w=
		  ((nodefrag->node[i].entry.data.attr.width-searchmin)/
		   (searchmax-searchmin))*(maxval-minval)+minval;
	    }
	}
      nodefrag=nodefrag->next;
    }
  
  return GRL_OK;
}


/*............................................................*/

graphlib_error_t graphlib_scaleEdgeWidth(graphlib_graph_p graph, 
					 graphlib_width_t minval,
					 graphlib_width_t maxval)
{
  int                     i,first;
  graphlib_width_t        searchmin=0,searchmax=0;
  graphlib_edgefragment_p edgefrag;

  first=1;

  edgefrag=graph->edges;
  while (edgefrag!=NULL)
    {
      for (i=0; i<edgefrag->count; i++)
	{
	  if (edgefrag->edge[i].full)
	    {
	      if ((edgefrag->edge[i].entry.data.attr.width<searchmin)||(first))
		{
		  searchmin=edgefrag->edge[i].entry.data.attr.width;
		}
	      if ((edgefrag->edge[i].entry.data.attr.width>searchmax)||(first))
		{
		  searchmax=edgefrag->edge[i].entry.data.attr.width;
		}
	      first=0;
	    }
	}
      edgefrag=edgefrag->next;
    }
  
  edgefrag=graph->edges;
  while (edgefrag!=NULL)
    {
      for (i=0; i<edgefrag->count; i++)
	{
	  if (edgefrag->edge[i].full)
	    {
	      if (searchmin==searchmax)
		edgefrag->edge[i].entry.data.attr.width=(maxval+minval)/2.0;
	      else
		edgefrag->edge[i].entry.data.attr.width=
		  ((edgefrag->edge[i].entry.data.attr.width-searchmin)/
		   (searchmax-searchmin))*(maxval-minval)+minval;
	    }
	}
      edgefrag=edgefrag->next;
    }
  
  return GRL_OK;
}


/*............................................................*/
/* Set node annotation name */

graphlib_error_t graphlib_AnnotationKey (graphlib_graph_p graph, int num, char *name)
{
  if ((0<=num) && (num<graph->numannotation))
    {
      if (graph->annotations[num]!=NULL)
	free(graph->annotations[num]);
      if (name==NULL)
	graph->annotations[num]=NULL;
      else
	graph->annotations[num]=strdup(name);
      return GRL_OK;
    }
  else
    return GRL_NOATTRIBUTE;
}


/*............................................................*/
/* Set annotation value for a node */

graphlib_error_t graphlib_AnnotationSet(graphlib_graph_p graph, graphlib_node_t node, 
					int num, graphlib_annotation_t val)
{
  graphlib_error_t err;
  graphlib_nodeentry_p noderef;
  graphlib_nodefragment_p nodefrag;
  int index;

  if ((0<=num) && (num<graph->numannotation))
    {
      err=grlibint_findNodeIndex(graph,node,&noderef,&nodefrag,&index);
      if (GRL_IS_NOTOK(err))
	return err;
      nodefrag->grannot[index*graph->numannotation+num]=val;
      return GRL_OK;
    }
  else
    return GRL_NOATTRIBUTE;
}


/*............................................................*/
/* Get annotation name from a node */

graphlib_error_t graphlib_AnnotationGet(graphlib_graph_p graph, graphlib_node_t node, 
					int num, graphlib_annotation_t* val)
{
  graphlib_error_t err;
  graphlib_nodeentry_p noderef;
  graphlib_nodefragment_p nodefrag;
  int index;

  if ((0<=num) && (num<graph->numannotation))
    {
      err=grlibint_findNodeIndex(graph,node,&noderef,&nodefrag,&index);
      if (GRL_IS_NOTOK(err))
	return err;
      *val=nodefrag->grannot[index*graph->numannotation+num];
      return GRL_OK;
    }
  else
    return GRL_NOATTRIBUTE;
}


/*-----------------------------------------------------------------*/
/* Analysis routines */

/*............................................................*/

graphlib_error_t graphlib_colorInvertedPath(graphlib_graph_p gr, graphlib_color_t color, graphlib_node_t node)
{
  graphlib_error_t     err;
  graphlib_nodeentry_p noderef;
  graphlib_edgeentry_p edgeref;

  while (1)
    {
      err=grlibint_findNode(gr,node,&noderef);
      if GRL_IS_NOTOK(err)
	{
	  return err;
	}

      if ((noderef->entry.data.attr.color)==color)
	{
	  /* circle */
	  return GRL_OK;
	}
      
      noderef->entry.data.attr.color=color;

      err=grlibint_findIncomingEdgeColor(gr,node,color,&edgeref);
      if GRL_IS_NOTOK(err)
	{
	  if (err==GRL_NOEDGE)
	    return GRL_OK;
	  else
	    return err;
	}      

      node=edgeref->entry.data.node_from;
    }
}


/*............................................................*/

graphlib_error_t graphlib_colorInvertedPathDeleteRest(graphlib_graph_p gr, graphlib_color_t color, graphlib_color_t color_off, graphlib_node_t node)
{
  graphlib_error_t     err;
  graphlib_nodeentry_p noderef;
  graphlib_edgeentry_p edgeref;

  int                     i,lastnode;
  graphlib_edgefragment_p edgefrag;
	  

  lastnode=-1;
  while (1)
    {
      err=grlibint_findNode(gr,node,&noderef);
      if GRL_IS_NOTOK(err)
	{
	  return err;
	}

      if ((noderef->entry.data.attr.color)==color)
	{
	  /* circle */
	  return GRL_OK;
	}
      
      noderef->entry.data.attr.color=color;

      edgefrag=gr->edges;
      while (edgefrag!=NULL)
	{
	  for (i=0; i<edgefrag->count; i++)
	    {
	      if (edgefrag->edge[i].full)
		{
		  if ((edgefrag->edge[i].entry.data.node_from==node) &&
		      (edgefrag->edge[i].entry.data.node_to!=lastnode))
		    {
		      edgefrag->edge[i].entry.data.attr.color=color_off;
		      err=graphlib_deleteTreeNotRootColor(gr,edgefrag->edge[i].entry.data.node_to,color);
		      if (err!=GRL_OK)
			return err;
		    }
		}
	    }
	  edgefrag=edgefrag->next;
	}

      err=grlibint_findIncomingEdgeColor(gr,node,color,&edgeref);
      if GRL_IS_NOTOK(err)
	{
	  if (err==GRL_NOEDGE)
	    return GRL_OK;
	  else
	    return err;
	}      

      lastnode=node;
      node=edgeref->entry.data.node_from;
    }
}


/*............................................................*/

graphlib_error_t graphlib_deleteInvertedPath(graphlib_graph_p gr,
					     graphlib_node_t node,graphlib_node_t *lastnode)
{
  graphlib_error_t     err,res=GRL_OK;
  graphlib_nodeentry_p noderef;
  graphlib_edgeentry_p edgeref;
  int                  multiple,dummylast,done;

  multiple=0;
  done=0;

  while (!done)
    {
      /* remember last node */

      *lastnode=node;

      /* find reference */

      err=grlibint_findNode(gr,node,&noderef);
      if GRL_IS_NOTOK(err)
	{
	  return err;
	}
      
      /* find first coming in */

      err=grlibint_findIncomingEdge(gr,node,&edgeref);
      if GRL_IS_NOTOK(err)
	{
	  if (err==GRL_NOEDGE)
	    {
	      if (multiple)
		res=GRL_MULTIPLEPATHS;
	      else
		res=GRL_OK;
	      done=1;
	    }
	  else
	    return err;
	}      
      else
	{
	  /* set next step */
	  
	  node=edgeref->entry.data.node_from;

	  /* get rid of edge */
	  
	  grlibint_delEdge(gr,edgeref);
	}

      /* see if there are edges going out */

      do
	{
	  err=grlibint_findOutgoingEdge(gr,*lastnode,&edgeref);
	  if (err==GRL_OK)
	    {
	      err=grlibint_delEdge(gr,edgeref);
	    }
	}
      while (err==GRL_OK);
      if (err!=GRL_NOEDGE)
	return err;
      
      /* see if there are more coming in */

      do
	{
	  err=grlibint_findIncomingEdge(gr,*lastnode,&edgeref);
	  if (err==GRL_OK)
	    {
	      err=graphlib_deleteInvertedPath(gr,edgeref->entry.data.node_from,&dummylast);
	      multiple=1;
	    }
	}
      while (err==GRL_OK);
      if (err!=GRL_NOEDGE)
	return err;

      /* now, that there are no edges, delete point */

      err=grlibint_delNode(gr,noderef);
      if (err!=GRL_OK)
	return err;
    }  

  return res;
}


/*............................................................*/

graphlib_error_t graphlib_deleteInvertedLine(graphlib_graph_p gr,
					     graphlib_node_t node,graphlib_node_t *lastnode)
{
  graphlib_error_t     err,res=GRL_OK;
  graphlib_nodeentry_p noderef;
  graphlib_edgeentry_p edgeref,edgeout;
  int                  multiple,dummylast,done;

  multiple=0;
  done=0;

  while (!done)
    {
      /* remember last node */

      *lastnode=node;

      /* see if there are edges going out */

      err=grlibint_findOutgoingEdge(gr,node,&edgeout);
      if (err!=GRL_NOEDGE)
	return err;
      
      /* find reference */

      err=grlibint_findNode(gr,node,&noderef);
      if GRL_IS_NOTOK(err)
	{
	  return err;
	}
      
      /* find first coming in */

      err=grlibint_findIncomingEdge(gr,node,&edgeref);
      if GRL_IS_NOTOK(err)
	{
	  if (err==GRL_NOEDGE)
	    {
	      if (multiple)
		res=GRL_MULTIPLEPATHS;
	      else
		res=GRL_OK;
	      done=1;
	    }
	  else
	    return err;
	}      
      else
	{
	  /* set next step */
	  
	  node=edgeref->entry.data.node_from;

	  /* get rid of edge */
	  
	  grlibint_delEdge(gr,edgeref);
	}

      /* see if there are mode coming in */

      do
	{
	  err=grlibint_findIncomingEdge(gr,*lastnode,&edgeref);
	  if (err==GRL_OK)
	    {
	      err=graphlib_deleteInvertedPath(gr,edgeref->entry.data.node_from,&dummylast);
	      multiple=1;
	    }
	}
      while (err==GRL_OK);
      if (err!=GRL_NOEDGE)
	return err;

      /* now, that there are no edges, delete point */

      err=grlibint_delNode(gr,noderef);
      if (err!=GRL_OK)
      	return err;
    }  

  return res;
}


/*............................................................*/

graphlib_error_t graphlib_deleteTreeNotRoot(graphlib_graph_p gr, graphlib_node_t node)
{
  int                     i;
  graphlib_edgefragment_p edgefrag;
  graphlib_error_t err;

  edgefrag=gr->edges;
  while (edgefrag!=NULL)
    {
      for (i=0; i<edgefrag->count; i++)
	{
	  if (edgefrag->edge[i].full)
	    {
	      if (edgefrag->edge[i].entry.data.node_from==node)
		{
		  grlibint_delEdge(gr,&(edgefrag->edge[i]));
		  err=graphlib_deleteTree(gr,edgefrag->edge[i].entry.data.node_to);
		  if (err!=GRL_OK)
		    return err;
		}
	    }
	}
      edgefrag=edgefrag->next;
    }

  return GRL_OK;
}


/*............................................................*/

graphlib_error_t graphlib_deleteTreeNotRootColor(graphlib_graph_p gr, graphlib_node_t node, graphlib_color_t color)
{
  int                     i;
  graphlib_edgefragment_p edgefrag;
  graphlib_error_t err;
  graphlib_nodeentry_p     noderef;

  err=grlibint_findNode(gr,node,&noderef);
  if GRL_IS_NOTOK(err)
    {
      return err;
    }
  if (noderef->entry.data.attr.color==color)
    return GRL_OK;
      
  edgefrag=gr->edges;
  while (edgefrag!=NULL)
    {
      for (i=0; i<edgefrag->count; i++)
	{
	  if (edgefrag->edge[i].full)
	    {
	      if (edgefrag->edge[i].entry.data.node_from==node)
		{
		  grlibint_delEdge(gr,&(edgefrag->edge[i]));
		  err=graphlib_deleteTreeColor(gr,edgefrag->edge[i].entry.data.node_to,color);
		  if (err!=GRL_OK)
		    return err;
		}
	    }
	}
      edgefrag=edgefrag->next;
    }

  return GRL_OK;
}


/*............................................................*/

graphlib_error_t graphlib_deleteTree(graphlib_graph_p gr, graphlib_node_t node)
{
  graphlib_error_t err;

  err=graphlib_deleteTreeNotRoot(gr,node);
  if (err!=GRL_OK)
    return err;
  return graphlib_deleteConnectedNode(gr,node);
}


/*............................................................*/

graphlib_error_t graphlib_deleteTreeColor(graphlib_graph_p gr, graphlib_node_t node, graphlib_color_t color)
{
  graphlib_error_t err;
  graphlib_nodeentry_p     noderef;

  err=grlibint_findNode(gr,node,&noderef);
  if GRL_IS_NOTOK(err)
    {
      return err;
    }
  if (noderef->entry.data.attr.color==color)
    return GRL_OK;
      
  err=graphlib_deleteTreeNotRootColor(gr,node,color);
  if (err!=GRL_OK)
    return err;
  return graphlib_deleteConnectedNode(gr,node);
}


/*............................................................*/

graphlib_error_t graphlib_collapseHor(graphlib_graph_p gr)
{
  int                     i,j,c_in,c_out;
  graphlib_edgefragment_p edgefrag;
  graphlib_nodefragment_p nodefrag;
  graphlib_nodeentry_p    n_in,n_out;
  graphlib_edgeentry_p    e_in,e_out;

  n_in=0;
  n_out=0;
  e_in=0;
  e_out=0;

  nodefrag=gr->nodes;
  while (nodefrag!=NULL)
    {
      for (i=0; i<nodefrag->count; i++)
	{
	  if (nodefrag->node[i].full)
	    {
	      c_in=0;
	      c_out=0;
	      
	      edgefrag=gr->edges;
	  
	      while (edgefrag!=NULL)
		{
		  for (j=0; j<edgefrag->count; j++)
		    {
		      if (edgefrag->edge[j].full)
			{
 			  if (edgefrag->edge[j].entry.data.node_to==
			      nodefrag->node[i].entry.data.id)
			    {
			      c_in++;
			      e_in=&(edgefrag->edge[j]);
			      n_in=edgefrag->edge[j].entry.data.ref_from;
			      
			    }
 			  if (edgefrag->edge[j].entry.data.node_from==
			      nodefrag->node[i].entry.data.id)
			    {
			      c_out++;
			      e_out=&(edgefrag->edge[j]);
			      n_out=edgefrag->edge[j].entry.data.ref_to;
			    }
			}
		    }
		  edgefrag=edgefrag->next;
		}

	      if ((c_in==1) && (c_out==1))
		{
		  if ((n_in->entry.data.attr.x==
		       nodefrag->node[i].entry.data.attr.x) &&
		      (n_out->entry.data.attr.x==
		       nodefrag->node[i].entry.data.attr.x))
		    {
		      e_in->entry.data.node_to=e_out->entry.data.node_to;
		      e_in->entry.data.ref_to=e_out->entry.data.ref_to;
		      grlibint_delEdge(gr,e_out);
		      grlibint_delNode(gr,&(nodefrag->node[i]));
		    }
		}
	    }
	}	  
      nodefrag=nodefrag->next;
    }

  return GRL_OK;
}


/*............................................................*/
/* color graphs by edge sets */

#ifndef NOSET

graphlib_error_t graphlib_colorGraphByLeadingEdgeLabel(graphlib_graph_p graph)
{
  graphlib_nodefragment_p  nf;
  graphlib_edgeentry_p  e;
  graphlib_nodedata_p n;
  graphlib_error_t err;
  //color nodes based on name of incoming edge
  grlibint_num_colors=0;
  nf=graph->nodes;
  while (nf!=NULL) 
    {
      int i;
      for (i=0; i<nf->count; i++) 
	{
	  if (nf->node[i].full) 
	    {
	      n=&(nf->node[i].entry.data);
	      err = grlibint_findIncomingEdge( graph, n->id, &e );
	      if( err == GRL_NOEDGE )
		{
		  n->attr.color=0;
		  continue;
                }
	      
	      assert( GRL_IS_OK(err) );
	      n->attr.color = grlibint_getNodeColor( e->entry.data.attr.edgelist );
            }
        }
      nf=nf->next;
    }
  
  return GRL_OK;
}

#endif

#ifdef STAT_BITVECTOR
graphlib_error_t graphlib_setEdgeByTask(void **edgelist, int task)
{
  if (*edgelist==NULL)
    {
      *edgelist=bitvec_allocate();
      if (*edgelist==NULL)
	return GRL_NOMEM;
    }
  else
    {
      bitvec_erase(*edgelist);
    }

  bitvec_insert(*edgelist, task);
  return GRL_OK;
}

int graphlib_getBitVectorSize()
{
    return bv_typesize;
}
#endif


/*-----------------------------------------------------------------*/
/* The End. */
