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


/*-----------------------------------------------------------------*/
/* Library to create, manipulate, and export graphs                */
/* Martin Schulz, LLNL, 2005-2006                                  */
/* Additions by Dorian Arnold, LLNL, 2006                          */
/* Modifications by Gregory L. Lee, LLNL, 2006-2011                */
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
  graphlib_node_t      node_from;
  graphlib_node_t      node_to;
  graphlib_edgeattr_t  attr;
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
  int                      num_node_attrs;
  int                      num_edge_attrs;
  char                     **annotations;
  char                     **node_attr_keys;
  char                     **edge_attr_keys;
  graphlib_nodefragment_t  *nodes;
  graphlib_edgefragment_t  *edges;
  graphlib_nodeentry_p     freenodes;
  graphlib_edgeentry_p     freeedges;
  graphlib_functiontable_p functions;
} graphlib_graph_t;

typedef struct graphlib_graphlist_d *graphlib_graphlist_p;
typedef struct graphlib_graphlist_d
{
  graphlib_graph_p      graph;
  graphlib_graphlist_p  next;
} graphlib_graphlist_t;


/*-----------------------------------------------------------------*/
/* Variables */

graphlib_graphlist_t *allgraphs=NULL;

static unsigned int grlibint_num_colors=0;
static long node_clusters[GRC_RAINBOWCOLORS];

/* Default function table */
static graphlib_functiontable_p default_functions=NULL;



/*-----------------------------------------------------------------*/
/* Simple support routines */

/*............................................................*/
/* find a node in the node table with indices */

graphlib_error_t grlibint_findNodeIndex(graphlib_graph_p graph,
                                        graphlib_node_t node,
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
                                   graphlib_node_t node,
                                   graphlib_nodeentry_p *entry)
{
  int                     i;
  graphlib_nodefragment_p nodefrag;

  return grlibint_findNodeIndex(graph,node,entry,&nodefrag,&i);
}


/*............................................................*/
/* find an edge in the edge table */

graphlib_error_t grlibint_findEdge(graphlib_graph_p graph,
                                   graphlib_node_t node1,
                                   graphlib_node_t node2,
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
                                  graphlib_nodeentry_p node)
{
  int i;
  if (node->full==0)
    {
      return GRL_NONODE;
    }

  graph->functions->free_node(node->entry.data.attr.label);

  for (i=0; i<graph->num_node_attrs; i++)
    {
      graph->functions->free_node_attr(graph->node_attr_keys[i], node->entry.data.attr.attr_values[i]);
    }
  free(node->entry.data.attr.attr_values);

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
  int i;
  if (edge->full==0)
    {
      return GRL_NOEDGE;
    }

  for (i=0; i<graph->num_edge_attrs; i++)
    {
      graph->functions->free_edge_attr(graph->edge_attr_keys[i], edge->entry.data.attr.attr_values[i]);
    }
  free(edge->entry.data.attr.attr_values);

  edge->full=0;
  edge->entry.freeptr=graph->freeedges;
  graph->functions->free_edge(edge->entry.data.attr.label);
  graph->freeedges=edge;

  return GRL_OK;
}


/*............................................................*/
/* create and register a new graph */

graphlib_error_t grlibint_addGraph(graphlib_graph_p *newgraph)
{
  graphlib_graphlist_p newitem;

  newitem=(graphlib_graphlist_t*)malloc(sizeof(graphlib_graphlist_t));
  if (newitem==NULL)
    return GRL_NOMEM;

  *newgraph=(graphlib_graph_p)calloc(1, sizeof(graphlib_graph_t));
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

graphlib_error_t grlibint_newNodeFragment(graphlib_nodefragment_p *newnodefrag,
                                          int numannotation)
{
  int i;

  *newnodefrag=(graphlib_nodefragment_t*)
                 calloc(1, sizeof(graphlib_nodefragment_t));
  if (*newnodefrag==NULL)
      return GRL_NOMEM;

  (*newnodefrag)->next=NULL;
  (*newnodefrag)->count=0;

  if (numannotation==0)
    (*newnodefrag)->grannot=NULL;
  else
    {
      (*newnodefrag)->grannot=
        (graphlib_annotation_t*)malloc(sizeof(graphlib_annotation_t)
                                         *NODEFRAGSIZE*numannotation);
      if ((*newnodefrag)->grannot==NULL)
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
/* create and initialize new edge segment */

graphlib_error_t grlibint_newEdgeFragment(graphlib_edgefragment_p *newedgefrag)
{
  *newedgefrag=(graphlib_edgefragment_t*)
                  calloc(1, sizeof(graphlib_edgefragment_t));
  if (*newedgefrag==NULL)
      return GRL_NOMEM;

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

graphlib_error_t grlibint_copyDataToBuf(int *idx, const char *src, int len,
                                        char **dest_array, int *dest_array_len)
{
  unsigned int alloc_size;
  char         *dst;
  if ((((*idx)+len)>(*dest_array_len)))
    {
      alloc_size =  8192*(1+((*idx)+len)/8192);
      *dest_array = (char*)realloc(*dest_array, alloc_size);
      *dest_array_len = alloc_size;
    }

  dst=(*dest_array)+(*idx);
  memcpy(dst,src,len);
  (*idx)+=len;

#ifdef DEBUG
  fprintf(stderr,"[%s:%s():%d]: ", __FILE__,__FUNCTION__,__LINE__);
  if (len == sizeof(int))
    fprintf(stderr,"\tint: %d (%p) => %d (%p)\n",
            *((int*)src),src,*((int*)dst),dst);
  else if (len==sizeof(graphlib_width_t))
    fprintf(stderr,"\tdouble: %.2lf (%p) => %.2lf (%p)\n",
            *((double*)src),src,*((double*)dst),dst);
  else
    fprintf(stderr, "\tstring: \"%s\" (%p) => \"%s\" (%p)\n",src,src,dst,dst);
#endif

  return GRL_OK;
}


/*............................................................*/
/* read graph from a buffer */

graphlib_error_t grlibint_copyDataFromBuf(char *dst, int *idx, int len,
                                          char *src_array, int src_array_len)
{
  char *src=src_array+*idx;

  /* check for array bounds read error */
  if ((((*idx)+len)>(src_array_len)))
    {
      return GRL_MEMORYERROR;
    }

  memcpy(dst,src,len);
  (*idx)+=len;

#ifdef DEBUG
  fprintf(stderr,"[%s:%s():%d]: ",__FILE__,__FUNCTION__,__LINE__);
  if (len==sizeof(int))
    fprintf(stderr,"int: %d (%p) <= %d (%p)\n",*((int*)dst),dst,*((int*)src),
            src);
  else if (len == sizeof(graphlib_width_t))
    fprintf(stderr, "double: %.2lf (%p) <= %.2lf (%p)\n",
            *((double*)dst),dst,*((double*)src),src);
  else
    fprintf(stderr, "string: \"%s\" (%p) <= \"%s\" (%p)\n",dst,dst,src,src);
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
      /*GLL comment: added the +.1 b/c when the num_colors was a nice power of 2 (i.e., 2,4,8) the color_vals were all coming out as shades of one color
            float normalized_color_val=1.0-(((float)color-GRC_RAINBOW)/((float)grlibint_num_colors+.1));
            color_val=(unsigned int)(normalized_color_val*((float)0xFFFFFF)); */

            /*GLL comment: if there are a small number of colors, use a larger
              normalizing value to get a better color range*/
            if (grlibint_num_colors<18)
              color_val=16777215-(unsigned int)(((color-GRC_RAINBOW)/18.0)*16777215);
            /* Powers of 2 number of colors create red-only colors */
            else if (grlibint_num_colors%32==0)
              color_val=16777215-(unsigned int)(((color-GRC_RAINBOW)/(float)(grlibint_num_colors+1))*16777215);
            else
              color_val=16777215-(unsigned int)(((color-GRC_RAINBOW)/(float)grlibint_num_colors)*16777215);
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
        else if ((color>=GRC_GREENSPEC) &&
                 (color<GRC_GREENSPEC+GRC_SPECTRUMRANGE))
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
        else if ((color>=GRC_GREENSPEC) && (color<GRC_GREENSPEC+
                                                    GRC_SPECTRUMRANGE))
          {
            if ((color-GRC_GREENSPEC)<(GRC_SPECTRUMRANGE/2))
              fprintf(fh,"\"#000000\"");
            else
              fprintf(fh,"\"#FFFFFF\"");
          }
        else if ((color>=GRC_RAINBOW) && (color<GRC_RAINBOW+GRC_RAINBOWCOLORS))
          {
            unsigned int color_val;
            float normalized_color_val=1.0-(((float)color-GRC_RAINBOW)/
                                            ((float)grlibint_num_colors));
            color_val=(unsigned int)(normalized_color_val*((float)0xFFFFFF));
            if (((color_val & 0xFF0000)+(color_val && 0xFF00)+(color_val &&
                                                               0xFF))>0xFF)
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
        else if ((color>=GRC_GREENSPEC) && (color<GRC_GREENSPEC+
                                                  GRC_SPECTRUMRANGE))
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

int grlibint_getNodeColor(const void *label, long (*edge_checksum)(const char *, const void *), void *(*copy_edge)(const void *))
{
  unsigned int i=0;

  for (i=0;(i<grlibint_num_colors) && (i<GRC_RAINBOWCOLORS); i++)
    {
      if (node_clusters[i]==edge_checksum("NULL", label))
        {
          return GRC_RAINBOW+i+1;
        }
    }

  if (i<GRC_RAINBOWCOLORS)
    {
      /*need to add new node_cluster*/
      node_clusters[i]=edge_checksum("NULL", label);
      grlibint_num_colors++;
      return GRC_RAINBOW+i+1;
    }

  return GRC_RAINBOW;
}



/*............................................................*/
/* dynamic color distribution */
/* THIS IS A HACK RIGHT NOW USING A GLOBAL TABLE */
/* WORKS ONLY FOR ONE GRAPH! */

int grlibint_getNodeAttrColor(const char *key, const void *label, long (*edge_checksum)(const char *, const void *), void *(*copy_edge)(const char *, const void *))
{
  unsigned int i=0;

  for (i=0;(i<grlibint_num_colors) && (i<GRC_RAINBOWCOLORS); i++)
    {
      if (node_clusters[i]==edge_checksum(key, label))
        {
          return GRC_RAINBOW+i+1;
        }
    }

  if (i<GRC_RAINBOWCOLORS)
    {
      /*need to add new node_cluster*/
      node_clusters[i]=edge_checksum(key, label);
      grlibint_num_colors++;
      return GRC_RAINBOW+i+1;
    }

  return GRC_RAINBOW;
}

#ifdef DEBUG
void grlibint_print_color_assignment()
{
    for (unsigned int i=0;(i<grlibint_num_colors) && (i<1024);i++)
      {
        fprintf(stderr,"Color[%d]: val: ",i+1);
        grlibint_exp_dot_color(i+1,stderr);
        fprintf(stderr,"\n");
      }
}
#endif


/*-----------------------------------------------------------------*/
/* Default functions */

void grlibint_serialize_node(char *buf, const void *label)
{
  if (label!= NULL)
    strcpy(buf,label);
}
unsigned int grlibint_serialize_node_length(const void *label)
{
  if (label!=NULL)
    return strlen(label)+1;
  else
    return 0;
}
void grlibint_deserialize_node(void **label, const char *buf,
                               unsigned int label_len)
{
  *label = malloc(strlen(buf) + 1);
  strcpy((char *)*label,buf);
}
char *grlibint_node_to_text(const void *label)
{
  if (label != NULL)
    return strdup((char*)label);
  else
    return NULL;
}
void *grlibint_merge_node(void *label1, const void *label2)
{
  if (label1==NULL || label2==NULL)
    return NULL;
  if (strcmp((char*)label1,(char*)label2) == 0)
    return label1;
  label1 = realloc(label1,strlen((char *)label1)+strlen((char *)label2)+1);
  if (label1==NULL)
    return NULL;
  strcat((char*)label1,(char*)label2);
  return label1;
}
void *grlibint_copy_node(const void *label)
{
  if (label!=NULL)
    return (void*)strdup(label);
  else
    return NULL;
}
void grlibint_free_node(void *label)
{
  free(label);
}

void grlibint_serialize_node_attr(const char *key, char *buf, const void *label)
{
  if (label!= NULL)
    strcpy(buf,label);
}
unsigned int grlibint_serialize_node_attr_length(const char *key, const void *label)
{
  if (label!=NULL)
    return strlen(label)+1;
  else
    return 0;
}
void grlibint_deserialize_node_attr(const char *key, void **label,
                                    const char *buf, unsigned int label_len)
{
  *label = malloc(strlen(buf) + 1);
  strcpy((char *)*label,buf);
}
char *grlibint_node_attr_to_text(const char *key, const void *label)
{
  if (label != NULL)
    return strdup((char*)label);
  else
    return NULL;
}
void *grlibint_merge_node_attr(const char *key, void *label1, const void *label2)
{
  if (label1==NULL || label2==NULL)
    return NULL;
  if (strcmp((char*)label1,(char*)label2) == 0)
    return label1;
  label1 = realloc(label1,strlen((char *)label1)+strlen((char *)label2)+1);
  if (label1==NULL)
    return NULL;
  strcat((char*)label1,(char*)label2);
  return label1;
}
void *grlibint_copy_node_attr(const char *key, const void *label)
{
  if (label!=NULL)
    return (void*)strdup(label);
  else
    return NULL;
}
void grlibint_free_node_attr(const char *key, void *label)
{
  free(label);
}

/* These are the same as the node, so we'll just point to the node versions
void grlibint_serialize_edge(char *, void *);
unsigned int grlibint_serialize_edge_length(const void *);
void grlibint_deserialize_edge(char *, const void *);
char *grlibint_edge_to_text(const void *);
void grlibint_merge_edge(void *, const void*);
void *grlibint_copy_edge(const void *);
void grlibint_free_edge(void *);
void grlibint_serialize_edge_attr(char *, void *);
unsigned int grlibint_serialize_edge_attr_length(const void *);
void grlibint_deserialize_edge_attr(char *, const void *);
char *grlibint_edge_attr_to_text(const void *);
void grlibint_merge_edge_attr(void *, const void*);
void *grlibint_copy_edge_attr(const void *);
void grlibint_free_edge_attr(void *);
 */

long grlibint_edge_checksum(const char *key, const void * label)
{
  long sum=0;
  int  i;
  for (i=0;i<strlen((char*)label);i++)
    sum+=(int)((char*)label)[i];
}

graphlib_error_t graphlib_Init()
{
  if (default_functions==NULL)
  {
    default_functions=(graphlib_functiontable_p)
                         calloc(1, sizeof(graphlib_functiontable_t));
    if (default_functions==NULL)
      return GRL_NOMEM;
    default_functions->serialize_node = grlibint_serialize_node;
    default_functions->serialize_node_length = grlibint_serialize_node_length;
    default_functions->deserialize_node = grlibint_deserialize_node;
    default_functions->node_to_text = grlibint_node_to_text;
    default_functions->merge_node = grlibint_merge_node;
    default_functions->copy_node = grlibint_copy_node;
    default_functions->free_node = grlibint_free_node;
    default_functions->serialize_node_attr = grlibint_serialize_node_attr;
    default_functions->serialize_node_attr_length = grlibint_serialize_node_attr_length;
    default_functions->deserialize_node_attr = grlibint_deserialize_node_attr;
    default_functions->node_attr_to_text = grlibint_node_attr_to_text;
    default_functions->merge_node_attr = grlibint_merge_node_attr;
    default_functions->copy_node_attr = grlibint_copy_node_attr;
    default_functions->free_node_attr = grlibint_free_node_attr;
    default_functions->serialize_edge = grlibint_serialize_node;
    default_functions->serialize_edge_length = grlibint_serialize_node_length;
    default_functions->deserialize_edge = grlibint_deserialize_node;
    default_functions->edge_to_text = grlibint_node_to_text;
    default_functions->merge_edge = grlibint_merge_node;
    default_functions->copy_edge = grlibint_copy_node;
    default_functions->free_edge = grlibint_free_node;
    default_functions->serialize_edge_attr = grlibint_serialize_node_attr;
    default_functions->serialize_edge_attr_length = grlibint_serialize_node_attr_length;
    default_functions->deserialize_edge_attr = grlibint_deserialize_node_attr;
    default_functions->edge_attr_to_text = grlibint_node_attr_to_text;
    default_functions->merge_edge_attr = grlibint_merge_node_attr;
    default_functions->copy_edge_attr = grlibint_copy_node_attr;
    default_functions->free_edge_attr = grlibint_free_node_attr;
    default_functions->edge_checksum = grlibint_edge_checksum;
  }
  return GRL_OK;
}

/*............................................................*/
/* cleanup */

graphlib_error_t graphlib_Finish()
{
  graphlib_error_t ret = graphlib_delAll();
  if (default_functions != NULL)
    free(default_functions);
  default_functions = NULL;
  return ret;
}


/*............................................................*/
/* add graph without annotations */

graphlib_error_t graphlib_newGraph(graphlib_graph_p *newgraph,
                                   graphlib_functiontable_p functions)
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
  (*newgraph)->num_node_attrs=0;
  (*newgraph)->num_edge_attrs=0;
  (*newgraph)->annotations=NULL;
  (*newgraph)->node_attr_keys=NULL;
  (*newgraph)->edge_attr_keys=NULL;

  (*newgraph)->freenodes=NULL;
  (*newgraph)->freeedges=NULL;
  if (functions != NULL)
    (*newgraph)->functions=functions;
  else
    (*newgraph)->functions=default_functions;

  return GRL_OK;
}


/*............................................................*/
/* add graph with annotations */

graphlib_error_t graphlib_newAnnotatedGraph(graphlib_graph_p *newgraph,
                                            graphlib_functiontable_p functions,
                                            int numannotation)
{
  graphlib_error_t err;
  int              i;

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
  (*newgraph)->num_node_attrs=0;
  (*newgraph)->num_edge_attrs=0;
  (*newgraph)->node_attr_keys=NULL;
  (*newgraph)->edge_attr_keys=NULL;

  (*newgraph)->freenodes=NULL;
  (*newgraph)->freeedges=NULL;
  if (functions != NULL)
    (*newgraph)->functions=functions;
  else
    (*newgraph)->functions=default_functions;

  return GRL_OK;
}


/*.......................................................*/
/* Add a node attribute key */
graphlib_error_t graphlib_addNodeAttrKey(graphlib_graph_p graph,
                                         const char *key,
                                         int *index)
{
  graph->num_node_attrs+=1;
  graph->node_attr_keys=(char **)realloc(graph->node_attr_keys,graph->num_node_attrs*sizeof(char *));
  if (graph->node_attr_keys==NULL)
    return GRL_NOMEM;
  *index=graph->num_node_attrs-1;
  graph->node_attr_keys[*index]=strdup(key);
  if (graph->node_attr_keys[*index]==NULL)
    return GRL_NOMEM;
  return GRL_OK;
}


/*.......................................................*/
/* Get number of node attributes */
graphlib_error_t graphlib_getNumNodeAttrs(graphlib_graph_p graph,
                                          int *num_attrs)
{
  *num_attrs=graph->num_node_attrs;
  return GRL_OK;
}


/*.......................................................*/
/* Get a node attribute key */
graphlib_error_t graphlib_getNodeAttrKey(graphlib_graph_p graph,
                                         int index,
                                         char **key)
{
  if (index<0 || index>=graph->num_node_attrs)
    return GRL_NOATTRIBUTE;
  *key=graph->node_attr_keys[index];
  return GRL_OK;
}


/*.......................................................*/
/* Get a node attribute index */
graphlib_error_t graphlib_getNodeAttrIndex(graphlib_graph_p graph,
                                         const char *key,
                                         int *index)
{
  int i;
  for (i=0;i<graph->num_node_attrs;i++)
    {
      if (strcmp(key,graph->node_attr_keys[i])==0)
        {
          *index=i;
          return GRL_OK;
        }
    }
  return GRL_NOATTRIBUTE;
}


/*.......................................................*/
/* Add an edge attribute key */
graphlib_error_t graphlib_addEdgeAttrKey(graphlib_graph_p graph,
                                         const char *key,
                                         int *index)
{
  graph->num_edge_attrs+=1;
  graph->edge_attr_keys=(char **)realloc(graph->edge_attr_keys,graph->num_edge_attrs*sizeof(char *));
  if (graph->edge_attr_keys==NULL)
    return GRL_NOMEM;
  *index=graph->num_edge_attrs-1;
  graph->edge_attr_keys[*index]=strdup(key);
  if (graph->edge_attr_keys[*index]==NULL)
    return GRL_NOMEM;
  return GRL_OK;
}


/*.......................................................*/
/* Get number of edge attributes */
graphlib_error_t graphlib_getNumEdgeAttrs(graphlib_graph_p graph,
                                          int *num_attrs)
{
  *num_attrs=graph->num_edge_attrs;
  return GRL_OK;
}


/*.......................................................*/
/* Get a edge attribute key */
graphlib_error_t graphlib_getEdgeAttrKey(graphlib_graph_p graph,
                                         int index,
                                         char **key)
{
  if (index<0 || index>=graph->num_edge_attrs)
    return GRL_NOATTRIBUTE;
  *key=graph->edge_attr_keys[index];
  return GRL_OK;
}


/*.......................................................*/
/* Get a edge attribute index */
graphlib_error_t graphlib_getEdgeAttrIndex(graphlib_graph_p graph,
                                         const char *key,
                                         int *index)
{
  int i;
  for (i=0;i<graph->num_edge_attrs;i++)
    {
      if (strcmp(key,graph->edge_attr_keys[i])==0)
        {
          *index=i;
          return GRL_OK;
        }
    }
  return GRL_NOATTRIBUTE;
}



/*............................................................*/
/* delete an edge attribute */

graphlib_error_t graphlib_delEdgeAttr(graphlib_edgeattr_t deledgeattr,
                                      void (*free_edge)(void *))
{
  if (free_edge == NULL)
    grlibint_free_node(deledgeattr.label);
  else
    free_edge(deledgeattr.label);
  return GRL_OK;
}

/*............................................................*/
/* delete a graph */

graphlib_error_t graphlib_delGraph(graphlib_graph_p delgraph)
{
  int i,j;
  graphlib_edgefragment_p deledge;
  graphlib_nodefragment_p delnode;
  graphlib_graphlist_p    graphs,oldgraphs;
  graphlib_nodefragment_p  nodefrag;
  graphlib_edgefragment_p  edgefrag;

  nodefrag = delgraph->nodes;
  while (nodefrag!=NULL)
    {
      for (i=0;i<nodefrag->count;i++)
        {
          if (nodefrag->node[i].full)
            {
              if (nodefrag->node[i].entry.data.attr.label != NULL)
                delgraph->functions->free_node(nodefrag->node[i].entry.data.attr.label);
              if (nodefrag->node[i].entry.data.attr.attr_values != NULL)
                {
                  for (j=0; j<delgraph->num_node_attrs; j++)
                    {
                      delgraph->functions->free_node_attr(delgraph->node_attr_keys[j], nodefrag->node[i].entry.data.attr.attr_values[j]);
                    }
                  free(nodefrag->node[i].entry.data.attr.attr_values);
                }
            }
        }
      nodefrag=nodefrag->next;
    }
  edgefrag=delgraph->edges;
  while (edgefrag!=NULL)
    {
      for (i=0;i<edgefrag->count;i++)
        {
          if (edgefrag->edge[i].full)
            {
              if (edgefrag->edge[i].entry.data.attr.label != NULL)
                delgraph->functions->free_edge(edgefrag->edge[i].entry.data.attr.label);
              if (edgefrag->edge[i].entry.data.attr.attr_values != NULL)
                {
                  for (j=0; j<delgraph->num_edge_attrs; j++)
                    {
                      delgraph->functions->free_edge_attr(delgraph->edge_attr_keys[j], edgefrag->edge[i].entry.data.attr.attr_values[j]);
                    }
                  free(edgefrag->edge[i].entry.data.attr.attr_values);
                }
            }
        }
      edgefrag=edgefrag->next;
    }

  for (i=0;i<delgraph->numannotation;i++)
    {
      if (delgraph->annotations[i]!=NULL)
        free(delgraph->annotations[i]);
    }
  if (delgraph->annotations!=NULL)
    free(delgraph->annotations);

  if (delgraph->node_attr_keys != NULL)
    {
      for (i=0;i<delgraph->num_node_attrs;i++)
        {
          if (delgraph->node_attr_keys[i]!=NULL)
            free(delgraph->node_attr_keys[i]);
        }
      free(delgraph->node_attr_keys);
    }
  delgraph->num_node_attrs=0;
  if (delgraph->edge_attr_keys != NULL)
    {
      for (i=0;i<delgraph->num_edge_attrs;i++)
        {
          if (delgraph->edge_attr_keys[i]!=NULL)
            free(delgraph->edge_attr_keys[i]);
        }
      free(delgraph->edge_attr_keys);
    }
  delgraph->num_edge_attrs=0;

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

graphlib_error_t graphlib_nodeCount(graphlib_graph_p igraph, int *num_nodes)
{
    graphlib_nodefragment_p nodefrag=igraph->nodes;
    int                     i=0;

    *num_nodes=0;
    while (nodefrag!=NULL)
      {
        for (i=0;i<nodefrag->count;i++)
          {
            if (nodefrag->node[i].full)
              {
                (*num_nodes)++;
              }
          }
        nodefrag=nodefrag->next;
      }

    return GRL_OK;
}


/*............................................................*/
/* count edges */

graphlib_error_t graphlib_edgeCount(graphlib_graph_p igraph, int *num_edges)
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

graphlib_error_t graphlib_loadGraph(graphlib_filename_t fn,
                                    graphlib_graph_p *newgraph,
                                    graphlib_functiontable_p functions)
{
  int              fh;
  graphlib_error_t err;
  char             *serialized_graph;
  uint64_t         size;

  fh=open(fn,O_RDONLY);
  if (fh<0)
    return GRL_FILEERROR;

  err=grlibint_addGraph(newgraph);
  if (GRL_IS_FATALERROR(err))
    return err;

  err=grlibint_read(fh,(char*)(&size), sizeof(uint64_t));
  if (GRL_IS_FATALERROR(err))
    return err;

  serialized_graph=(char*)malloc(size);
  if (serialized_graph==NULL)
    return GRL_NOMEM;

  err=grlibint_read(fh,serialized_graph,size);
  if (GRL_IS_FATALERROR(err))
    return err;

  err=graphlib_deserializeGraph(newgraph,functions,serialized_graph,size);
  if (GRL_IS_FATALERROR(err))
    return err;

  if (serialized_graph!=NULL)
    free(serialized_graph);

  return GRL_OK;
}


/*............................................................*/
/* save a complete graph / internal format */

graphlib_error_t graphlib_saveGraph(graphlib_filename_t fn,
                                    graphlib_graph_p graph)
{
  int              fh,i;
  char             *serialized_graph;
  uint64_t         size;
  graphlib_error_t err;

  fh=open(fn,O_WRONLY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE|S_IRGRP|S_IROTH);
  if (fh<0)
    return GRL_FILEERROR;

  err = graphlib_serializeGraph(graph,&serialized_graph,&size);
  if (GRL_IS_FATALERROR(err))
    return err;
  err=grlibint_write(fh,(char*) &size, sizeof(uint64_t));
  if (GRL_IS_FATALERROR(err))
    return err;
  err=grlibint_write(fh, serialized_graph,size);
  if (serialized_graph!=NULL)
    free(serialized_graph);
  if (GRL_IS_FATALERROR(err))
    return err;

  return GRL_OK;
}


/*............................................................*/
/* export graph to new format */

graphlib_error_t graphlib_exportGraph(graphlib_filename_t fn,
                                      graphlib_format_t format,
                                      graphlib_graph_p graph)
{
  return graphlib_exportAttributedGraph(fn, format, graph, 0, NULL, NULL);
}

/*............................................................*/
/* export graph to new format */

graphlib_error_t graphlib_exportAttributedGraph(graphlib_filename_t fn,
                                      graphlib_format_t format,
                                      graphlib_graph_p graph,
                                      int num_attrs,
                                      char **attr_keys,
                                      char **attr_values)
{
  char *tmp;
  switch (format)
    {

    /*=====================================================*/

    case GRF_DOT:
    case GRF_PLAINDOT:
      {
        FILE                     *fh;
        graphlib_nodefragment_p  nodefrag;
        graphlib_edgefragment_p  edgefrag;
        graphlib_nodedata_p      node;
        graphlib_edgedata_p      edge;
        int                      i, j;

        /* open file */

        fh=fopen(fn,"w");
        if (fh==NULL)
          return GRL_FILEERROR;

        /* write header */

        fprintf(fh,"digraph G {\n");
        for (i=0;i<num_attrs;i++)
          {
            if (i==0)
              fprintf(fh,"\tgraph [");
            else
              fprintf(fh,",");
            fprintf(fh,"%s=\"%s\"",attr_keys[i],attr_values[i]);
            if (i==num_attrs-1)
              fprintf(fh,"];\n");
          }
        fprintf(fh,"\tnode [shape=record,style=filled,labeljust=c,height=0.2];\n");

        /* write nodes */

        nodefrag=graph->nodes;
        while (nodefrag!=NULL)
          {
            for (i=0;i<nodefrag->count;i++)
              {
                if (nodefrag->node[i].full)
                  {
                    node=&(nodefrag->node[i].entry.data);
                    {
                      /* write one node */

                      fprintf(fh,"\t%i [",node->id);

                      fprintf(fh,"pos=\"%i,%i\", ",node->attr.x,node->attr.y);
                      tmp = graph->functions->node_to_text(node->attr.label);
                      fprintf(fh,"label=\"%s\", ", tmp);
                      free(tmp);
                      fprintf(fh,"fillcolor=");
                      if (format==GRF_PLAINDOT)
                        grlibint_exp_plaindot_color(node->attr.color,fh);
                      else
                        grlibint_exp_dot_color(node->attr.color,fh);
                      fprintf(fh,", fontcolor=");
                      if (format==GRF_PLAINDOT)
                        grlibint_exp_plaindot_fontcolor(node->attr.color,fh);
                      else
                        grlibint_exp_dot_fontcolor(node->attr.color,fh);
                      for (j=0; j<graph->num_node_attrs; j++)
                        {
                          fprintf(fh,", %s=",graph->node_attr_keys[j]);
                          tmp = graph->functions->node_attr_to_text(graph->node_attr_keys[j], node->attr.attr_values[j]);
                          fprintf(fh,"\"%s\"", tmp);
                          free(tmp);
                        }
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
            for (i=0;i<edgefrag->count;i++)
              {
                if (edgefrag->edge[i].full)
                  {
                    edge=&(edgefrag->edge[i].entry.data);
                    {
                      /* write one edge */
                      fprintf(fh,"\t%i -> %i [",edge->node_from,edge->node_to);
                      tmp = graph->functions->edge_to_text(edge->attr.label);
                      fprintf(fh,"label=\"%s\"", tmp);
                      free(tmp);
                      for (j=0; j<graph->num_edge_attrs; j++)
                        {
                          fprintf(fh,", %s=",graph->edge_attr_keys[j]);
                          tmp = graph->functions->edge_attr_to_text(graph->edge_attr_keys[j], edge->attr.attr_values[j]);
                          fprintf(fh,"\"%s\"", tmp);
                          free(tmp);
                        }
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
        FILE                     *fh;
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
            for (i=0;i<nodefrag->count;i++)
              {
                if (nodefrag->node[i].full)
                  {
                    node=&(nodefrag->node[i].entry.data);
                    {
                      /* write one node */

                      fprintf(fh,"\tnode\n");
                      fprintf(fh,"\t[\n");
                      fprintf(fh,"\t\tid %i\n",node->id);
                      if (node->attr.label==NULL)
                        {
                          if (node->attr.width!=0.0)
                            fprintf(fh,"\t\tlabel \"%.2f\"\n",
                                    node->attr.width*1000.0);
                          else
                            fprintf(fh,"\t\tlabel \"\"\n");
                        }
                      else
                        {
                          tmp = graph->functions->node_to_text(node->attr.label);
                          fprintf(fh,"\t\tlabel \"%s\"\n", tmp);
                          free(tmp);
                        }

                      for (j=0;j<graph->numannotation;j++)
                        {
                          if (graph->annotations[j]!=NULL)
                            {
                              fprintf(fh,"\t\t%s \"%f\"\n",
                                      graph->annotations[j],
                                      nodefrag->grannot[i*graph->numannotation
                                                          +j]);
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
                          if (node->attr.label==NULL)
                            {
                              if (node->attr.width!=0.0)
                                fprintf(fh,"\t\t\ttext \"%.2f\"\n",
                                        node->attr.width*1000.0);
                              else
                                fprintf(fh,"\t\t\ttext \"\"\n");
                            }
                          else
                            {
                              tmp = graph->functions->node_to_text(node->attr.label);
                              fprintf(fh,"\t\t\ttext \"%s\"\n", tmp);
                              free(tmp);
                            }

                          fprintf(fh,"\t\t\tcolor ");
                          grlibint_exp_gml_fontcolor(node->attr.color,fh);

                          if (node->attr.fontsize!=DEFAULT_FONT_SIZE)
                            {
                              fprintf(fh,"\t\t\tfontSize %i\n",
                                      node->attr.fontsize);
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
            for (i=0;i<edgefrag->count;i++)
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
            for (i=0;i<edgefrag->count;i++)
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
                        fprintf(fh,"\t\t\twidth %f\n",
                                edge->attr.width*edgescale);
                      else
                        fprintf(fh,"\t\t\twidth 1.0\n");
                      fprintf(fh,"\t\t\ttargetArrow \"standard\"\n");
                      fprintf(fh,"\t\t\tfill ");
                      grlibint_exp_gml_color(edge->attr.color,fh);
                      switch (edge->attr.arcstyle)
                        {
                        case GRA_ARC:
                          {
                            fprintf(fh,"\t\t\tarcType        \"fixedRatio\"\n");
                            fprintf(fh,"\t\t\tarcRatio        1.0\n");
                            break;
                          }
                        case GRA_SPLINE:
                          {
                            fprintf(fh,"\t\t\tarcType        \"fixedRatio\"\n");
                            fprintf(fh,"\t\t\tarcRatio        1.0\n");
                            break;
                          }
                        }
                      fprintf(fh,"\t\t]\n");

                      fprintf(fh,"\t\tLabelGraphics\n");
                      fprintf(fh,"\t\t[\n");

                      if (graph->edgeset)
                        {
                          tmp = graph->functions->edge_to_text(edge->attr.label);
                          fprintf(fh,"\t\t\ttext \"%s\"\n", tmp);
                          free(tmp);
                        }
                      else
                        {
                          if (edge->attr.label!=NULL)
                            {
                              tmp = graph->functions->edge_to_text(edge->attr.label);
                              fprintf(fh,"\t\t\ttext \"%s\"\n", tmp);
                              free(tmp);
                            }
                        }

                      fprintf(fh,"\t\t\tmodel   \"centered\"\n");
                      fprintf(fh,"\t\t\tposition        \"center\"\n");
                      if (edge->attr.fontsize>0)
                        fprintf(fh,"\t\t\tfontSize %i\n",edge->attr.fontsize);
                      if ((edge->attr.block==GRB_BLOCK) ||
                          (edge->attr.block==GRB_FULL))
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

graphlib_error_t grlibint_serializeGraph(graphlib_graph_p igraph,
                                         char **obyte_array,
                                         uint64_t *obyte_array_len,
                                         int full_graph)
{
  graphlib_error_t err;
  char                    *temp_array=NULL,*temp=NULL,term='\0';
  int                     cur_idx,num_nodes,num_edges;
  int                     temp_array_len=0,i,j;
  unsigned int            label_len;
  graphlib_nodefragment_p nodefrag;
  graphlib_edgefragment_p edgefrag;
  graphlib_nodedata_p     node;
  graphlib_edgedata_p     edge;

  err = graphlib_nodeCount(igraph,&num_nodes);
  err = graphlib_edgeCount(igraph,&num_edges);

  cur_idx = 0;
  *obyte_array_len = 0;

  /* write header */
  grlibint_copyDataToBuf(&cur_idx,(const char*)&num_nodes,sizeof(int),
                         &temp_array,&temp_array_len);
  *obyte_array_len+=sizeof(int);
  grlibint_copyDataToBuf(&cur_idx,(const char *)&num_edges,sizeof(int),
                         &temp_array,&temp_array_len);
  *obyte_array_len+=sizeof(int);

  if (full_graph==1)
    {
      /* write annotations */
      grlibint_copyDataToBuf(&cur_idx,(const char *)&igraph->numannotation,
                             sizeof(int),&temp_array,&temp_array_len);
      *obyte_array_len+=sizeof(int);
      for (i=0;i<igraph->numannotation;i++)
        {
          if (igraph->annotations[i]!=NULL)
            label_len=strlen(igraph->annotations[i])+1;
          else
            label_len=0;
          grlibint_copyDataToBuf(&cur_idx,(const char *)&label_len,
                                 sizeof(unsigned int),&temp_array,
                                 &temp_array_len);
          *obyte_array_len+=sizeof(int);
          if (label_len>0)
            {
              grlibint_copyDataToBuf(&cur_idx,igraph->annotations[i],label_len,
                                     &temp_array,&temp_array_len);
              *obyte_array_len+=label_len;
            }
        }
    }

  /* write attr keys */
  grlibint_copyDataToBuf(&cur_idx,(const char *)&igraph->num_node_attrs,
                         sizeof(int),&temp_array,&temp_array_len);
  *obyte_array_len+=sizeof(int);
  for (i=0;i<igraph->num_node_attrs;i++)
    {
      if (igraph->node_attr_keys[i]!=NULL)
        label_len=strlen(igraph->node_attr_keys[i])+1;
      else
        label_len=0;
      grlibint_copyDataToBuf(&cur_idx,(const char *)&label_len,
                             sizeof(unsigned int),&temp_array,
                             &temp_array_len);
      *obyte_array_len+=sizeof(int);
      if (label_len>0)
        {
          grlibint_copyDataToBuf(&cur_idx,igraph->node_attr_keys[i],label_len,
                                 &temp_array,&temp_array_len);
          *obyte_array_len+=label_len;
        }
    }
  grlibint_copyDataToBuf(&cur_idx,(const char *)&igraph->num_edge_attrs,
                         sizeof(int),&temp_array,&temp_array_len);
  *obyte_array_len+=sizeof(int);
  for (i=0;i<igraph->num_edge_attrs;i++)
    {
      if (igraph->edge_attr_keys[i]!=NULL)
        label_len=strlen(igraph->edge_attr_keys[i])+1;
      else
        label_len=0;
      grlibint_copyDataToBuf(&cur_idx,(const char *)&label_len,
                             sizeof(unsigned int),&temp_array,
                             &temp_array_len);
      *obyte_array_len+=sizeof(int);
      if (label_len>0)
        {
          grlibint_copyDataToBuf(&cur_idx,igraph->edge_attr_keys[i],label_len,
                                 &temp_array,&temp_array_len);
          *obyte_array_len+=label_len;
        }
    }

  /* write nodes */
  nodefrag=igraph->nodes;
  while (nodefrag!=NULL)
    {
      for (i=0;i<nodefrag->count;i++)
        {
          if (nodefrag->node[i].full)
            {
              /* write one node */
              node=&(nodefrag->node[i].entry.data);

              /* id */
              grlibint_copyDataToBuf(&cur_idx,(const char *)&(node->id),
                                     sizeof(graphlib_node_t), &temp_array,
                                     &temp_array_len);
              *obyte_array_len+=sizeof(graphlib_node_t);

              /* label */
              label_len=igraph->functions->
                serialize_node_length(node->attr.label);
              grlibint_copyDataToBuf(&cur_idx,(const char *)&(label_len),
                                     sizeof(unsigned int),&temp_array,
                                     &temp_array_len);
              *obyte_array_len+=sizeof(unsigned int);
              if (label_len!=0)
                {
                  temp=(char*)malloc(label_len);
                  if (temp==NULL)
                    return GRL_NOMEM;
                  igraph->functions->serialize_node(temp, node->attr.label);
                  grlibint_copyDataToBuf(&cur_idx,temp,label_len,&temp_array,
                                         &temp_array_len);
                  free(temp);
                  *obyte_array_len+=label_len;
                }

              /* attrs */
              for (j=0;j<igraph->num_node_attrs;j++)
                {
                  label_len=igraph->functions->
                    serialize_node_attr_length(igraph->node_attr_keys[j],
                                               node->attr.attr_values[j]);
                  grlibint_copyDataToBuf(&cur_idx,(const char *)&(label_len),
                                         sizeof(unsigned int),&temp_array,
                                         &temp_array_len);
                  *obyte_array_len+=sizeof(unsigned int);
                  if (label_len!=0)
                    {
                      temp=(char*)malloc(label_len);
                      if (temp==NULL)
                        return GRL_NOMEM;
                      igraph->functions->serialize_node_attr(igraph->node_attr_keys[j],
                                                             temp,
                                                             node->attr.attr_values[j]);
                      grlibint_copyDataToBuf(&cur_idx,temp,label_len,&temp_array,
                                             &temp_array_len);
                      free(temp);
                      *obyte_array_len+=label_len;
                    }
                }

              if (full_graph == 1)
                {
                  /* width */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(node->attr.width),
                                         sizeof(graphlib_width_t),&temp_array,
                                         &temp_array_len);
                  *obyte_array_len+=sizeof(graphlib_width_t);
                  /* w */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(node->attr.w),
                                         sizeof(graphlib_width_t),&temp_array,
                                         &temp_array_len);
                  *obyte_array_len+=sizeof(graphlib_width_t);
                  /* height */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(node->attr.height),
                                         sizeof(graphlib_width_t),&temp_array,
                                         &temp_array_len);
                  *obyte_array_len+=sizeof(graphlib_width_t);
                  /* color */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(node->attr.color),
                                         sizeof(graphlib_color_t),&temp_array,
                                         &temp_array_len);
                  *obyte_array_len+=sizeof(graphlib_color_t);
                  /* x */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(node->attr.x),
                                         sizeof(graphlib_coor_t),&temp_array,
                                         &temp_array_len);
                  *obyte_array_len+=sizeof(graphlib_coor_t);
                  /* y */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(node->attr.y),
                                         sizeof(graphlib_coor_t),&temp_array,
                                         &temp_array_len);
                  *obyte_array_len+=sizeof(graphlib_coor_t);
                  /* fontsize */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(node->attr.fontsize),
                                         sizeof(graphlib_fontsize_t),&temp_array,
                                         &temp_array_len);
                  *obyte_array_len+=sizeof(graphlib_fontsize_t);
                }
            }
        }
      nodefrag=nodefrag->next;
    }

  /* write edges */
  edgefrag=igraph->edges;
  while (edgefrag!=NULL)
    {
      for (i=0;i<edgefrag->count;i++)
        {
          if (edgefrag->edge[i].full)
            {
              /* write one edge */
              edge=&(edgefrag->edge[i].entry.data);

              /* from_id */
              grlibint_copyDataToBuf(&cur_idx,(const char *)&(edge->node_from),
                                     sizeof(graphlib_node_t),&temp_array,
                                     &temp_array_len);
              *obyte_array_len += sizeof(graphlib_node_t);

              /* to_id */
              grlibint_copyDataToBuf(&cur_idx,(const char *)&(edge->node_to),
                                     sizeof(graphlib_node_t),&temp_array,
                                     &temp_array_len);
              *obyte_array_len+=sizeof(graphlib_node_t);

              /* name */
              label_len=igraph->functions->serialize_edge_length(edge->attr.
                                                                   label);
              grlibint_copyDataToBuf(&cur_idx,(const char *)&(label_len),
                                     sizeof(unsigned int),&temp_array,
                                     &temp_array_len);
              *obyte_array_len+=sizeof(unsigned int);
              if (label_len!=0)
                {
                  temp=(char*)malloc(label_len);
                  if (temp==NULL)
                    return GRL_NOMEM;
                  igraph->functions->serialize_edge(temp,edge->attr.label);
                  grlibint_copyDataToBuf(&cur_idx,temp,label_len,&temp_array,
                                         &temp_array_len);
                  free(temp);
                  *obyte_array_len+=label_len;
                }

              /* attrs */
              for (j=0;j<igraph->num_edge_attrs;j++)
                {
                  label_len=igraph->functions->
                    serialize_edge_attr_length(igraph->edge_attr_keys[j],
                                               edge->attr.attr_values[j]);
                  grlibint_copyDataToBuf(&cur_idx,(const char *)&(label_len),
                                         sizeof(unsigned int),&temp_array,
                                         &temp_array_len);
                  *obyte_array_len+=sizeof(unsigned int);
                  if (label_len!=0)
                    {
                      temp=(char*)malloc(label_len);
                      if (temp==NULL)
                        return GRL_NOMEM;
                      igraph->functions->
                        serialize_edge_attr(igraph->edge_attr_keys[j],
                                              temp,
                                              edge->attr.attr_values[j]);
                      grlibint_copyDataToBuf(&cur_idx,temp,label_len,&temp_array,
                                             &temp_array_len);
                      free(temp);
                      *obyte_array_len+=label_len;
                    }
                }

              if (full_graph==1)
                {
                  /* width */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(edge->attr.width),
                                         sizeof(graphlib_width_t),&temp_array,
                                         &temp_array_len );
                  *obyte_array_len+=sizeof(graphlib_width_t);
                  /* color */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(edge->attr.color),
                                         sizeof(graphlib_color_t),&temp_array,
                                         &temp_array_len );
                  *obyte_array_len+=sizeof(graphlib_color_t);
                  /* arcstyle */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(edge->attr.arcstyle),
                                         sizeof(graphlib_arc_t),&temp_array,
                                         &temp_array_len );
                  *obyte_array_len+=sizeof(graphlib_arc_t);
                  /* block */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(edge->attr.block),
                                         sizeof(graphlib_block_t),&temp_array,
                                         &temp_array_len );
                  *obyte_array_len+=sizeof(graphlib_block_t);
                  /* fontsize */
                  grlibint_copyDataToBuf(&cur_idx,
                                         (const char *)&(edge->attr.fontsize),
                                         sizeof(graphlib_fontsize_t),
                                         &temp_array,
                                         &temp_array_len );
                  *obyte_array_len+=sizeof(graphlib_fontsize_t);
                }
            }
        }
      edgefrag=edgefrag->next;
    }

  *obyte_array = temp_array;
  return GRL_OK;
}

graphlib_error_t graphlib_serializeGraph(graphlib_graph_p igraph,
                                         char **obyte_array,
                                         uint64_t *obyte_array_len)
{
  return grlibint_serializeGraph(igraph, obyte_array, obyte_array_len, 1);
}

graphlib_error_t graphlib_serializeBasicGraph(graphlib_graph_p igraph,
                                              char **obyte_array,
                                              uint64_t *obyte_array_len)
{
  return grlibint_serializeGraph(igraph, obyte_array, obyte_array_len, 0);
}

/*............................................................*/
/* copy graph from a serialized buffer */

graphlib_error_t grlibint_deserializeGraph(graphlib_graph_p *ograph,
                                           graphlib_functiontable_p functions,
                                           char *ibyte_array,
                                           uint64_t ibyte_array_len,
                                           int full_graph)
{
  graphlib_error_t    err;
  graphlib_nodeattr_t node_attr = {0,0,0,0,0,0,NULL,14,NULL};
  graphlib_edgeattr_t edge_attr = {1,0,NULL,0,0,14,NULL};
  int                 num_nodes,num_edges,i,j,id=0,from_id=0,to_id=0;
  int                 cur_idx;
  unsigned int        label_len;

  err=graphlib_newGraph(ograph,functions);
  if (GRL_IS_FATALERROR(err))
    return err;

  cur_idx=0;
  /* read header */
  grlibint_copyDataFromBuf((char*)&num_nodes,&cur_idx,sizeof(int),ibyte_array,
                           ibyte_array_len);
  grlibint_copyDataFromBuf((char*)&num_edges,&cur_idx,sizeof(int),ibyte_array,
                           ibyte_array_len);

  if (full_graph==1)
    {
      /* read annotations */
      grlibint_copyDataFromBuf((char*)&((*ograph)->numannotation),&cur_idx,
                               sizeof(int),ibyte_array,ibyte_array_len);
      if ((*ograph)->numannotation>0)
        {
          (*ograph)->annotations=(char**)malloc((*ograph)->numannotation*
                                 sizeof(char*));
          if ((*ograph)->annotations==NULL)
            return GRL_NOMEM;
        }
      for (i=0;i<(*ograph)->numannotation;i++)
        {
          grlibint_copyDataFromBuf((char*)&(label_len),&cur_idx,
                                   sizeof(unsigned int),ibyte_array,
                                   ibyte_array_len);
          if (label_len>0)
            {
              (*ograph)->annotations[i]=(char*)malloc(label_len);
              if ((*ograph)->annotations[i]==NULL)
                return GRL_NOMEM;
              grlibint_copyDataFromBuf((*ograph)->annotations[i],&cur_idx,label_len,
                                       ibyte_array,ibyte_array_len);
            }
          else
            (*ograph)->annotations[i]=NULL;
        }
    }

  /* read attrs */
  grlibint_copyDataFromBuf((char*)&((*ograph)->num_node_attrs),&cur_idx,
                           sizeof(int),ibyte_array,ibyte_array_len);
  if ((*ograph)->num_node_attrs>0)
    {
      (*ograph)->node_attr_keys=(char**)malloc((*ograph)->num_node_attrs*
                             sizeof(char*));
      if ((*ograph)->node_attr_keys==NULL)
        return GRL_NOMEM;
    }
  for (i=0;i<(*ograph)->num_node_attrs;i++)
    {
      grlibint_copyDataFromBuf((char*)&(label_len),&cur_idx,
                               sizeof(unsigned int),ibyte_array,
                               ibyte_array_len);
      if (label_len>0)
        {
          (*ograph)->node_attr_keys[i]=(char*)malloc(label_len);
          if ((*ograph)->node_attr_keys[i]==NULL)
            return GRL_NOMEM;
          grlibint_copyDataFromBuf((*ograph)->node_attr_keys[i],&cur_idx,label_len,
                                   ibyte_array,ibyte_array_len);
        }
      else
        (*ograph)->node_attr_keys[i]=NULL;
    }
  grlibint_copyDataFromBuf((char*)&((*ograph)->num_edge_attrs),&cur_idx,
                           sizeof(int),ibyte_array,ibyte_array_len);
  if ((*ograph)->num_edge_attrs>0)
    {
      (*ograph)->edge_attr_keys=(char**)malloc((*ograph)->num_edge_attrs*
                             sizeof(char*));
      if ((*ograph)->edge_attr_keys==NULL)
        return GRL_NOMEM;
    }
  for (i=0;i<(*ograph)->num_edge_attrs;i++)
    {
      grlibint_copyDataFromBuf((char*)&(label_len),&cur_idx,
                               sizeof(unsigned int),ibyte_array,
                               ibyte_array_len);
      if (label_len>0)
        {
          (*ograph)->edge_attr_keys[i]=(char*)malloc(label_len);
          if ((*ograph)->edge_attr_keys[i]==NULL)
            return GRL_NOMEM;
          grlibint_copyDataFromBuf((*ograph)->edge_attr_keys[i],&cur_idx,label_len,
                                   ibyte_array,ibyte_array_len);
        }
      else
        (*ograph)->edge_attr_keys[i]=NULL;
    }

  /* read nodes */
  for(i=0;i<num_nodes;i++)
    {
      /* id */
      grlibint_copyDataFromBuf((char*)&id,&cur_idx,sizeof(graphlib_node_t),
                               ibyte_array,ibyte_array_len);

      /* name */
      grlibint_copyDataFromBuf((char*)&label_len,&cur_idx,sizeof(unsigned int),
                               ibyte_array,ibyte_array_len);
      if (label_len!=0)
        {
          (*ograph)->functions->deserialize_node(&(node_attr.label),
                                                 ibyte_array+cur_idx,label_len);
          cur_idx+=label_len;
        }
      else
        node_attr.label=NULL;

      /* attrs */
      node_attr.attr_values=(void **)calloc(1,(*ograph)->num_node_attrs*sizeof(void *));
      if (node_attr.attr_values==NULL)
        return GRL_NOMEM;
      for (j=0;j<(*ograph)->num_node_attrs;j++)
        {
          grlibint_copyDataFromBuf((char*)&label_len,&cur_idx,sizeof(unsigned int),
                                   ibyte_array,ibyte_array_len);
          if (label_len!=0)
            {
              (*ograph)->functions->
                deserialize_node_attr((*ograph)->node_attr_keys[j],
                                      &(node_attr.attr_values[j]),
                                      ibyte_array+cur_idx,label_len);
              cur_idx+=label_len;
            }
          else
            node_attr.attr_values[j]=NULL;
        }

      if (full_graph==1)
        {
          /* width */
          grlibint_copyDataFromBuf((char*)&node_attr.width,
                                   &cur_idx,sizeof(graphlib_width_t),
                                   ibyte_array,ibyte_array_len);
          /* w */
          grlibint_copyDataFromBuf((char*)&node_attr.w,
                                   &cur_idx,sizeof(graphlib_width_t),
                                   ibyte_array,ibyte_array_len);
          /* height */
          grlibint_copyDataFromBuf((char*)&node_attr.height,
                                   &cur_idx,sizeof(graphlib_width_t),
                                   ibyte_array,ibyte_array_len);
          /* color */
          grlibint_copyDataFromBuf((char*)&node_attr.color,
                                   &cur_idx,sizeof(graphlib_color_t),
                                   ibyte_array,ibyte_array_len);
          /* x */
          grlibint_copyDataFromBuf((char*)&node_attr.x,
                                   &cur_idx,sizeof(graphlib_coor_t),
                                   ibyte_array,ibyte_array_len);
          /* y */
          grlibint_copyDataFromBuf((char*)&node_attr.y,
                                   &cur_idx,sizeof(graphlib_coor_t),
                                   ibyte_array,ibyte_array_len);
          /* fontsize */
          grlibint_copyDataFromBuf((char*)&node_attr.fontsize,
                                   &cur_idx,sizeof(graphlib_fontsize_t),
                                   ibyte_array,ibyte_array_len);
        }
      graphlib_addNode(*ograph,id,&node_attr);
      if (node_attr.label != NULL)
        (*ograph)->functions->free_node(node_attr.label);
      if (node_attr.attr_values != NULL)
        {
          for (j=0;j<(*ograph)->num_node_attrs;j++)
            {
              (*ograph)->functions->free_node_attr((*ograph)->node_attr_keys[j],
                                                   node_attr.attr_values[j]);
            }
          free(node_attr.attr_values);
        }
    }

  /* read edges */
  for (i=0;i<num_edges;i++)
    {
      /* from_id */
      grlibint_copyDataFromBuf((char*)&from_id,&cur_idx,sizeof(graphlib_node_t),                               ibyte_array,ibyte_array_len );

      /* to_id */
      grlibint_copyDataFromBuf((char*)&to_id,&cur_idx,sizeof(graphlib_node_t),
                               ibyte_array,ibyte_array_len );

      /* name */
      grlibint_copyDataFromBuf((char*)&label_len,&cur_idx,sizeof(unsigned int),
                               ibyte_array,ibyte_array_len);
      if (label_len!=0)
        {
          (*ograph)->functions->deserialize_edge(&(edge_attr.label),
                                                 ibyte_array+cur_idx,label_len);
          cur_idx+=label_len;
        }
      else
        edge_attr.label=NULL;

      /* attrs */
      edge_attr.attr_values=(void **)calloc(1,(*ograph)->num_edge_attrs*sizeof(void *));
      if (edge_attr.attr_values==NULL)
        return GRL_NOMEM;
      for (j=0;j<(*ograph)->num_edge_attrs;j++)
        {
          grlibint_copyDataFromBuf((char*)&label_len,&cur_idx,sizeof(unsigned int),
                                   ibyte_array,ibyte_array_len);
          if (label_len!=0)
            {
              (*ograph)->functions->
                deserialize_edge_attr((*ograph)->edge_attr_keys[j],
                                 &(edge_attr.attr_values[j]),
                                 ibyte_array+cur_idx,label_len);
              cur_idx+=label_len;
            }
          else
            edge_attr.attr_values[j]=NULL;
        }


      if (full_graph==1)
        {
          /* width */
          grlibint_copyDataFromBuf((char*)&edge_attr.width,&cur_idx,
                                   sizeof(graphlib_width_t),ibyte_array,
                                   ibyte_array_len );
          /* color */
          grlibint_copyDataFromBuf((char*)&edge_attr.color,&cur_idx,
                                   sizeof(graphlib_color_t),ibyte_array,
                                   ibyte_array_len );
          /* arcstyle */
          grlibint_copyDataFromBuf((char*)&edge_attr.arcstyle,&cur_idx,
                                   sizeof(graphlib_arc_t),ibyte_array,
                                   ibyte_array_len );
          /* block */
          grlibint_copyDataFromBuf((char*)&edge_attr.block,&cur_idx,
                                   sizeof(graphlib_block_t),ibyte_array,
                                   ibyte_array_len );
          /* fontsize */
          grlibint_copyDataFromBuf((char*)&edge_attr.fontsize,&cur_idx,
                                   sizeof(graphlib_fontsize_t),ibyte_array,
                                   ibyte_array_len );
        }
      graphlib_addDirectedEdge(*ograph,from_id,to_id,&edge_attr);
      if (edge_attr.label != NULL)
        (*ograph)->functions->free_edge(edge_attr.label);
      if (edge_attr.attr_values != NULL)
        {
          for (j=0;j<(*ograph)->num_edge_attrs;j++)
            {
              (*ograph)->functions->free_edge_attr((*ograph)->edge_attr_keys[j],
                                                   edge_attr.attr_values[j]);
            }
          free(edge_attr.attr_values);
        }
    }

    return GRL_OK;
}


graphlib_error_t graphlib_deserializeGraph(graphlib_graph_p *ograph,
                                           graphlib_functiontable_p functions,
                                           char *ibyte_array,
                                           uint64_t ibyte_array_len)
{
  return grlibint_deserializeGraph(ograph,functions,ibyte_array,
                                   ibyte_array_len,1);
}

graphlib_error_t graphlib_deserializeBasicGraph(graphlib_graph_p *ograph,
                                                graphlib_functiontable_p
                                                  functions,
                                                char *ibyte_array,
                                                uint64_t ibyte_array_len)
{
  return grlibint_deserializeGraph(ograph,functions,ibyte_array,
                                   ibyte_array_len,0);
}

/*-----------------------------------------------------------------*/
/* Manipulation routines */

graphlib_error_t graphlib_addNode(graphlib_graph_p graph,graphlib_node_t node,
                                  graphlib_nodeattr_p attr)

{
  graphlib_nodefragment_p newfrag;
  graphlib_nodeentry_p    entry;
  graphlib_error_t        err;
  int                     newnode,i;

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
              err=grlibint_newNodeFragment(&(graph->nodes),
                                           graph->numannotation);
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
      entry->entry.data.attr.label=graph->functions->copy_node(attr->label);
      entry->entry.data.attr.attr_values=(void **)calloc(1,graph->num_node_attrs*sizeof(void *));
      if (entry->entry.data.attr.attr_values==NULL)
        return GRL_NOMEM;
      for(i=0;i<graph->num_node_attrs;i++)
        {
          entry->entry.data.attr.attr_values[i]=graph->functions->copy_node_attr(graph->node_attr_keys[i],
                                                                        attr->attr_values[i]);
        }
      err=GRL_OK;
    }
  else
    {
      err=graphlib_setDefNodeAttr(&(entry->entry.data.attr));
    }

  return err;
}


/*............................................................*/

graphlib_error_t graphlib_addNodeNoCheck(graphlib_graph_p graph,
                                         graphlib_node_t node,
                                         graphlib_nodeattr_p attr)

{
  graphlib_nodefragment_p newfrag;
  graphlib_nodeentry_p    entry;
  graphlib_error_t        err;
  int                     newnode,i;

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
              err=grlibint_newNodeFragment(&(graph->nodes),
                                           graph->numannotation);
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
      entry->entry.data.attr.label=graph->functions->copy_node(attr->label);
      entry->entry.data.attr.attr_values=(void **)calloc(1,graph->num_node_attrs*sizeof(void *));
      if (entry->entry.data.attr.attr_values==NULL)
        return GRL_NOMEM;
      for(i=0;i<graph->num_node_attrs;i++)
        {
          entry->entry.data.attr.attr_values[i]=graph->functions->copy_node_attr(graph->node_attr_keys[i],
                                                                        attr->attr_values[i]);
        }
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
  graphlib_edgefragment_p newfrag;
  graphlib_edgeentry_p    entry;
  graphlib_nodeentry_p    noderef1=NULL;
  graphlib_nodeentry_p    noderef2=NULL;
  graphlib_error_t        err;
  int                     i;

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
      entry->entry.data.attr.label = graph->functions->merge_edge(attr->label, entry->entry.data.attr.label);
//      graph->functions->free_edge(entry->entry.data.attr.label);
      for (i=0;i<graph->num_edge_attrs;i++)
        {
          entry->entry.data.attr.attr_values[i] = graph->functions->merge_edge_attr(graph->edge_attr_keys[i],attr->attr_values[i], entry->entry.data.attr.attr_values[i]);
        }
    }

  if (attr!=NULL)
    {
      entry->entry.data.attr=*attr;
      entry->entry.data.attr.label=graph->functions->copy_edge(attr->label);
      entry->entry.data.attr.attr_values=(void **)calloc(1,graph->num_edge_attrs*sizeof(void *));
      if (entry->entry.data.attr.attr_values==NULL)
        return GRL_NOMEM;
      for(i=0;i<graph->num_edge_attrs;i++)
        {
          entry->entry.data.attr.attr_values[i]=graph->functions->copy_edge_attr(graph->edge_attr_keys[i],
                                                                        attr->attr_values[i]);
        }
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
  graphlib_edgefragment_p newfrag;
  graphlib_edgeentry_p    entry;
  graphlib_nodeentry_p    noderef1=NULL;
  graphlib_nodeentry_p    noderef2=NULL;
  graphlib_error_t        err;
  int                     i;

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
      entry->entry.data.attr.label=graph->functions->copy_edge(attr->label);
      entry->entry.data.attr.attr_values=(void **)calloc(1,graph->num_edge_attrs*sizeof(void *));
      for(i=0;i<graph->num_edge_attrs;i++)
        {
          entry->entry.data.attr.attr_values[i]=graph->functions->copy_edge_attr(graph->edge_attr_keys[i],
                                                                        attr->attr_values[i]);
        }
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
  int                     err,i,j,directed;
  graphlib_nodefragment_p runnode;
  graphlib_edgefragment_p runedge;
  graphlib_nodeentry_p    nodeentry;
  graphlib_edgeentry_p    edgeentry;

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
              err=grlibint_findNode(graph1,runnode->node[i].entry.data.id,
                                    &nodeentry);
              if (err == GRL_OK )
                {
                  if (graph1->functions->merge_node != NULL)
                    nodeentry->entry.data.attr.label =
                               graph1->functions->merge_node(
                                       nodeentry->entry.data.attr.label,
                                       runnode->node[i].entry.data.attr.label);
                  if (graph1->functions->merge_node_attr != NULL)
                    for (j=0;j<graph1->num_node_attrs;j++)
                      nodeentry->entry.data.attr.attr_values[j] =
                                 graph1->functions->merge_node_attr(
                                         graph1->node_attr_keys[j],
                                         nodeentry->entry.data.attr.attr_values[j],
                                         runnode->node[i].entry.data.attr.attr_values[j]);
                }
              else
                {
                    err=graphlib_addNode(graph1,runnode->node[i].entry.data.id,
                                         &((runnode->node[i]).entry.data.attr));
                    if (GRL_IS_FATALERROR(err))
                      return err;
                }
            }
        }
      runnode=runnode->next;
    }

  while (runedge!=NULL)
    {
      for (i=0;i<runedge->count;i++)
        {
          if (runedge->edge[i].full)
            {
              err=grlibint_findEdge(graph1,
                                    runedge->edge[i].entry.data.node_from,
                                    runedge->edge[i].entry.data.node_to,
                                    &edgeentry);
              if (err == GRL_OK ) /*merge the edge labels*/
                {
                  if (graph1->functions->merge_edge != NULL)
                    edgeentry->entry.data.attr.label =
                               graph1->functions->merge_edge(
                                       edgeentry->entry.data.attr.label,
                                       runedge->edge[i].entry.data.attr.label);
                  if (graph1->functions->merge_edge_attr != NULL)
                    for (j=0;j<graph1->num_edge_attrs;j++)
                      edgeentry->entry.data.attr.attr_values[j] =
                                 graph1->functions->merge_edge_attr(
                                         graph1->edge_attr_keys[j],
                                         edgeentry->entry.data.attr.attr_values[j],
                                         runedge->edge[i].entry.data.attr.attr_values[j]);
                }
              else /*add the edge from graph2 into graph1*/
                {
                  err=graphlib_addDirectedEdge(graph1,
                                              runedge->edge[i].entry.data.
                                                node_from,
                                              runedge->edge[i].entry.data.
                                                node_to,
                                              &((runedge->edge[i]).entry.data.
                                                attr));
                  if (GRL_IS_FATALERROR(err))
                    return err;
                }
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
  int  err,i,j,directed;
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
      for (i=0;i<runnode->count;i++)
        {
          if (runnode->node[i].full)
            {
              err=grlibint_findNode(graph1,runnode->node[i].entry.data.id,
                                    &nodeentry);

              if (err == GRL_OK )
                {
                  /*widths must be combined*/
                  runnode->node[i].entry.data.attr.width+=nodeentry->entry.data.
                                                            attr.width;
                  if (graph1->functions->merge_node != NULL)
                    nodeentry->entry.data.attr.label =
                               graph1->functions->merge_node(
                                       nodeentry->entry.data.attr.label,
                                       runnode->node[i].entry.data.attr.label);
                  if (graph1->functions->merge_node_attr != NULL)
                    for (j=0;j<graph1->num_node_attrs;j++)
                      nodeentry->entry.data.attr.attr_values[j] =
                                 graph1->functions->merge_node_attr(
                                         graph1->node_attr_keys[j],
                                         nodeentry->entry.data.attr.attr_values[j],
                                         runnode->node[i].entry.data.attr.attr_values[j]);
                }
              else
                {
                  err=graphlib_addNode(graph1,runnode->node[i].entry.data.id,
                                       &((runnode->node[i]).entry.data.attr));
                  if (GRL_IS_FATALERROR(err))
                    return err;
                }
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
              err=grlibint_findEdge(graph1,
                                    runedge->edge[i].entry.data.node_from,
                                    runedge->edge[i].entry.data.node_to,
                                    &edgeentry);
              if (err==GRL_OK)
                {
      /*widths must be combined*/
                  runedge->edge[i].entry.data.attr.width+=edgeentry->entry.data.
                                                            attr.width;
                  if (graph1->functions->merge_edge!=NULL)
                    edgeentry->entry.data.attr.label =
                               graph1->functions->merge_edge(
                                       edgeentry->entry.data.attr.label,
                                       runedge->edge[i].entry.data.attr.label);
                  if (graph1->functions->merge_edge_attr!=NULL)
                    for (j=0;j<graph1->num_edge_attrs;j++)
                      edgeentry->entry.data.attr.attr_values[j] =
                                 graph1->functions->merge_edge_attr(
                                         graph1->edge_attr_keys[j],
                                         edgeentry->entry.data.attr.attr_values[j],
                                         runedge->edge[i].entry.data.attr.attr_values[j]);
                }
              else
                {
                  err=graphlib_addDirectedEdge(graph1,
                                               runedge->edge[i].entry.data.
                                                 node_from,
                                               runedge->edge[i].entry.data.
                                                 node_to,
                                               &((runedge->edge[i]).entry.data.
                                                 attr));
                  if (GRL_IS_FATALERROR(err))
                    return err;
                }
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
  attr->label=NULL;
  attr->fontsize=DEFAULT_FONT_SIZE;
  attr->attr_values=NULL;

  return GRL_OK;
}


/*............................................................*/
/* get the attributes of a node */
/* Added by Bob Munch in support of STAT, Cray */

graphlib_error_t graphlib_getNodeAttr(graphlib_graph_p graph,
                                      graphlib_node_t node,
                                      graphlib_nodeattr_t **attr)
{
  graphlib_error_t     err;
  graphlib_nodeentry_p entry;

  err=grlibint_findNode(graph,node,&entry);
  if (GRL_IS_NOTOK(err))
    return err;
  *attr=&(entry->entry.data.attr);

  return GRL_OK;
}


/*............................................................*/
/* set default node attributes */

graphlib_error_t graphlib_setDefEdgeAttr(graphlib_edgeattr_p attr)
{
  attr->width=DEFAULT_EDGE_WIDTH;
  attr->color=DEFAULT_EDGE_COLOR;
  attr->label=NULL;
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
      for (i=0;i<nodefrag->count;i++)
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
      for (i=0;i<edgefrag->count;i++)
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

graphlib_error_t graphlib_AnnotationKey (graphlib_graph_p graph,int num,
                                         char *name)
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

graphlib_error_t graphlib_AnnotationSet(graphlib_graph_p graph,
                                        graphlib_node_t node,
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

graphlib_error_t graphlib_AnnotationGet(graphlib_graph_p graph,
                                        graphlib_node_t node,
                                        int num, graphlib_annotation_t* val)
{
  graphlib_error_t        err;
  graphlib_nodeentry_p    noderef;
  graphlib_nodefragment_p nodefrag;
  int                     index;

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

graphlib_error_t graphlib_colorInvertedPath(graphlib_graph_p gr,
                                            graphlib_color_t color,
                                            graphlib_node_t node)
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

graphlib_error_t graphlib_colorInvertedPathDeleteRest(graphlib_graph_p gr,
                                                      graphlib_color_t color,
                                                      graphlib_color_t color_off,
                                                      graphlib_node_t node)
{
  graphlib_error_t        err;
  graphlib_nodeentry_p    noderef;
  graphlib_edgeentry_p    edgeref;
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
                      err=graphlib_deleteTreeNotRootColor(gr,
                                                          edgefrag->edge[i].
                                                            entry.data.node_to,
                                                          color);
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
                                             graphlib_node_t node,
                                             graphlib_node_t *lastnode)
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
              err=graphlib_deleteInvertedPath(gr,edgeref->entry.data.node_from,
                                              &dummylast);
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
                                             graphlib_node_t node,
                                             graphlib_node_t *lastnode)
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

graphlib_error_t graphlib_deleteTreeNotRoot(graphlib_graph_p gr,
                                            graphlib_node_t node)
{
  int                     i;
  graphlib_edgefragment_p edgefrag;
  graphlib_error_t        err;

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

graphlib_error_t graphlib_deleteTreeNotRootColor(graphlib_graph_p gr,
                                                  graphlib_node_t node,
                                                  graphlib_color_t color)
{
  int                     i;
  graphlib_edgefragment_p edgefrag;
  graphlib_error_t        err;
  graphlib_nodeentry_p    noderef;

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
                  err=graphlib_deleteTreeColor(gr,
                                               edgefrag->edge[i].entry.data.
                                                 node_to,
                                               color);
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

graphlib_error_t graphlib_deleteTree(graphlib_graph_p gr,graphlib_node_t node)
{
  graphlib_error_t err;

  err=graphlib_deleteTreeNotRoot(gr,node);
  if (err!=GRL_OK)
    return err;
  return graphlib_deleteConnectedNode(gr,node);
}


/*............................................................*/

graphlib_error_t graphlib_deleteTreeColor(graphlib_graph_p gr,
                                          graphlib_node_t node,
                                          graphlib_color_t color)
{
  graphlib_error_t     err;
  graphlib_nodeentry_p noderef;

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

graphlib_error_t graphlib_colorGraphByLeadingEdgeLabel(graphlib_graph_p graph)
{
  int i;
  graphlib_nodefragment_p  nf;
  graphlib_edgeentry_p  e;
  graphlib_nodedata_p n;
  graphlib_error_t err;

  /*color nodes based on name of incoming edge*/
  grlibint_num_colors=0;
  nf=graph->nodes;
  while (nf!=NULL)
    {
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
              n->attr.color = grlibint_getNodeColor( e->entry.data.attr.label, graph->functions->edge_checksum, graph->functions->copy_edge );
            }
        }
      nf=nf->next;
    }

  return GRL_OK;
}


/*............................................................*/
/* color graphs by edge sets */

graphlib_error_t graphlib_colorGraphByLeadingEdgeAttr(graphlib_graph_p graph, const char *key)
{
  int i, index;
  graphlib_nodefragment_p  nf;
  graphlib_edgeentry_p  e;
  graphlib_nodedata_p n;
  graphlib_error_t err;

  /*color nodes based on name of incoming edge*/
  grlibint_num_colors=0;
  nf=graph->nodes;
  while (nf!=NULL)
    {
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
              err = graphlib_getEdgeAttrIndex(graph, key, &index);
              assert( GRL_IS_OK(err) );
              n->attr.color = grlibint_getNodeAttrColor( key, e->entry.data.attr.attr_values[index], graph->functions->edge_checksum, graph->functions->copy_edge_attr );
            }
        }
      nf=nf->next;
    }

  return GRL_OK;
}

/*-----------------------------------------------------------------*/
/* The End. */
