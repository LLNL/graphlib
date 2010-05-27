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
/* Martin Schulz, LLNL, 2005-2007                                  */
/* Additions made by Dorian Arnold (summer 2006)                   */
/* Modifications made by Gregory L. Lee 2006-2007                  */
/* Version 1.0                                                     */
/*-----------------------------------------------------------------*/

#if !defined( __graphlib_h )
#define __graphlib_h 1

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */


/*-----------------------------------------------------------------*/
/* Constants */

/*.......................................................*/
/* limits of text strings used in attributes */

#define GRL_MAX_FN_LENGTH   200

/* only used for node attributes when using STAT_BITVECTOR*/
#define GRL_MAX_NAME_LENGTH 64

/* only used when using STAT_BITVECTOR*/
#define GRL_DEFAULT_EDGELABELWIDTH 1024


/*.......................................................*/
/* Attributes */

/* predefined color codes */

#define GRC_GRAY      0
#define GRC_GREY      0
#define GRC_BLUE      1
#define GRC_YELLOW    2
#define GRC_GREEN     3
#define GRC_ORANGE    4
#define GRC_TAN       5
#define GRC_FIREBRICK 6
#define GRC_RED       7
#define GRC_DARKGREEN 8
#define GRC_LIGHTGRAY 9
#define GRC_LIGHTGREY 9
#define GRC_PURPLE    10 
#define GRC_GOLDENROD 11
#define GRC_OLIVE     12
#define GRC_WHITE     13  
#define GRC_BLACK     14
#define GRC_RANGE1    15
#define GRC_RANGE2    16
#define GRC_RANGE3    17
#define GRC_RANGE4    18
#define GRL_NUM_COLORS 19


/* special colors to identify different shadings
   use these instead of color IDs (in combination
   with setting node values) */

#define GRC_REDSPEC   10000
#define GRC_GREENSPEC 20000
#define GRC_RAINBOW   30000


/* size of color shadings */

#define GRC_SPECTRUMRANGE 256
#define GRC_RAINBOWCOLORS 1024


/*.......................................................*/
/* Edge styles (not supported by all export formats) */

#define GRA_LINE    0
#define GRA_ARC     1
#define GRA_SPLINE  2


/*.......................................................*/
/* Edge label styles (not supported by all export formats) */

#define GRB_NONE  0
#define GRB_BLOCK 1
#define GRB_FULL  2


/*.......................................................*/
/* Export Formats */

#define GRF_NOEXPORT -1
#define GRF_DOT       0   /* use AT&T DOT format with hex. colors */
#define GRF_GML       1   /* use GraphML (GML) */
#define GRF_PLAINDOT  2   /* use AT&T DOT format with color names */


/*.......................................................*/
/* Macros to check error codes */

#define GRL_IS_OK(err)         (err==GRL_OK)
#define GRL_IS_NOTOK(err)      (err!=GRL_OK)
#define GRL_IS_FATALERROR(err) (err<GRL_OK)
#define GRL_IS_WARNING(err)    (err>GRL_OK)


/*.......................................................*/
/* Error codes */
/*   > 0 : warning only */
/*   = 0 : no error     */
/*   < 0 : fatal error  */
/* All routines return an error code */

#define GRL_MULTIPLEPATHS  5
#define GRL_NOEDGE         4
#define GRL_NONODE         3
#define GRL_NODEALREADY    2
#define GRL_EDGEALREADY    1
#define GRL_OK             0
#define GRL_NOMEM         -1
#define GRL_FILEERROR     -2
#define GRL_UNKNOWNFORMAT -3
#define GRL_MEMORYERROR   -4
#define GRL_NOATTRIBUTE   -5


/*-----------------------------------------------------------------*/
/* Types */

/*.......................................................*/
/* Basic Attribute Types */

typedef int    graphlib_color_t;      /* uses GRC_ constants */
typedef double graphlib_width_t;
typedef int    graphlib_coor_t;
typedef int    graphlib_arc_t;        /* uses GRA_ constants */
typedef int    graphlib_block_t;      /* uses GRB_ constants */
typedef int    graphlib_fontsize_t;


/*.......................................................*/
/* Node Attribute Structure */

typedef struct graphlib_nodeattr_d *graphlib_nodeattr_p;
typedef struct graphlib_nodeattr_d
{
  graphlib_width_t width,w,height;
  graphlib_color_t color; 
  graphlib_coor_t  x,y;
#ifdef GRL_DYNAMIC_NODE_NAME
  char *name;
#else
  char             name[GRL_MAX_NAME_LENGTH];
#endif
  graphlib_fontsize_t fontsize;
} graphlib_nodeattr_t;


/*.......................................................*/
/* Edge Attribute Structure */

typedef struct graphlib_edgeattr_d *graphlib_edgeattr_p;
typedef struct graphlib_edgeattr_d
{
  graphlib_width_t    width;
  graphlib_color_t    color; 
#ifdef STAT_BITVECTOR
  void                *edgelist;
#else
  char                name[GRL_MAX_NAME_LENGTH];
#endif
  graphlib_arc_t      arcstyle;
  graphlib_block_t    block;
  graphlib_fontsize_t fontsize;
} graphlib_edgeattr_t;


/*.......................................................*/
/* Node annotations */
/* Users can define additional annotations for each node.
   All annotations are of this type and the number of 
   annotations per node is defined at graph creation time */

typedef double graphlib_annotation_t;


/*.......................................................*/
/* Transparent types */
/* Never use the native types, since these can change */

/* Export format */
typedef int  graphlib_format_t;

/* Error codes */
typedef int  graphlib_error_t;

/* Filename for load/save/export functions */
typedef char graphlib_filename_t[GRL_MAX_FN_LENGTH];

/* IDs for individual nodes */
typedef int  graphlib_node_t;


/*.......................................................*/
/* Transparent pointer to one graph, used as a graph handle */

typedef struct graphlib_graph_d *graphlib_graph_p;


/*-----------------------------------------------------------------*/
/* Management routines */

/*.......................................................*/
/* Initialize GraphLib
   One of the Init functions must be called before any other call */

graphlib_error_t graphlib_Init(void);


/*.......................................................*/
/* Initialize GraphLib (variable edge label width)
   One of the Init functions must be called before any other call */
/* IN: width of edgelabels in number of bits */

graphlib_error_t graphlib_InitVarEdgeLabels(int edgelabelwidth);


/*.......................................................*/
/* Initialize GraphLib (variable edge label width and
   using an array to specify incoming bit widths
   One of the Init functions must be called before any other call */
/* IN: number of incoming connections
       array of widths of edgelabels in number of bits
   OUT: final edge label width in number of bits */

graphlib_error_t graphlib_InitVarEdgeLabelsConn(int numconn,
                                                int *edgelabelwidth,
                                                int *finalwidth);


/*.......................................................*/
/* Cleanup GraphLib
   No Graphlib routine can be called after this routine */

graphlib_error_t graphlib_Finish(void);


/*.......................................................*/
/* Create a new graph without node annotations */
/* IN: pointer to storage for graph handle */

graphlib_error_t graphlib_newGraph(graphlib_graph_p *newgraph);


/*.......................................................*/
/* Create a new graph without node annotations */
/* IN: pointer to storage for graph handle
       number of annotations per node */

graphlib_error_t graphlib_newAnnotatedGraph(graphlib_graph_p *newgraph,
                                            int numattr);


/*.......................................................*/
/* Delete an edge attribute */
/* IN: edge attribute handle */

graphlib_error_t graphlib_delEdgeAttr(graphlib_edgeattr_t deledgeattr);

/*............................................................*/
/* get the attributes of a node */
/* IN: graph handle
       node ID who's attribute is to be returned
       pointer to return value */
/* Added by Bob Munch in support of STAT, Cray */

graphlib_error_t graphlib_getNodeAttr(graphlib_graph_p graph,
                                      graphlib_node_t node,
                                      graphlib_nodeattr_t **attr);

/*.......................................................*/
/* Delete a graph */
/* IN: graph handle */

graphlib_error_t graphlib_delGraph(graphlib_graph_p delgraph);


/*.......................................................*/
/* Delete all graphs */

graphlib_error_t graphlib_delAll(void);


/*-----------------------------------------------------------------*/
/* Basic functions */

/*.......................................................*/
/* Count the number of nodes in a graph */
/* IN: graph handle
       pointer to return value */

graphlib_error_t graphlib_nodeCount(graphlib_graph_p igraph, int * num_nodes);


/*.......................................................*/
/* Count the number of edges in a graph */
/* IN: graph handle
       pointer to return value */

graphlib_error_t graphlib_edgeCount(graphlib_graph_p igraph, int * num_edges);


/*............................................................*/
/* find a node by edge rank, starting at inode */
/* Added by Bob Munch in support of STAT, Cray */

graphlib_error_t graphlib_findNextNodeByEdgeRank(graphlib_graph_p graph,
                                                 graphlib_node_t inode,
                                                 int rank,
                                                 graphlib_node_t *onode);


/*-----------------------------------------------------------------*/
/* Basic Manipulation routines */

/*.......................................................*/
/* add a node to a graph */
/* IN: graph handle
       node ID to be added
       node attributes (if NULL, default attributes are used) */

graphlib_error_t graphlib_addNode(graphlib_graph_p graph,
                                  graphlib_node_t node, 
                                  graphlib_nodeattr_p attr);


/*.......................................................*/
/* add a node to a graph */
/* don't check if operation is legal */
/* IN: graph handle
       node ID to be added
       node attributes (if NULL, default attributes are used) */

graphlib_error_t graphlib_addNodeNoCheck(graphlib_graph_p graph,
                                         graphlib_node_t node, 
                                         graphlib_nodeattr_p attr);


/*.......................................................*/
/* add an undirected edge to a graph */
/* IN: graph handle
       node ID for starting node
       node ID for ending node
       edge attributes (if NULL, default attributes are used)
   Comment: in case the graph is directed, this routine will add
   two directional edges, one in each direction */

graphlib_error_t graphlib_addUndirectedEdge(graphlib_graph_p graph,
                                            graphlib_node_t node1, 
                                            graphlib_node_t node2,
                                            graphlib_edgeattr_p attr);


/*.......................................................*/
/* add an undirected edge to a graph */
/* don't check if operation is legal */
/* IN: graph handle
       node ID for starting node
       node ID for ending node
       edge attributes (if NULL, default attributes are used)
   Comment: in case the graph is directed, this routine will add
   two directional edges, one in each direction */

graphlib_error_t graphlib_addUndirectedEdgeNoCheck(graphlib_graph_p graph,
                                                   graphlib_node_t node1, 
                                                   graphlib_node_t node2,
                                                   graphlib_edgeattr_p attr);


/*.......................................................*/
/* add a directed edge to a graph */
/* IN: graph handle
       node ID for starting node
       node ID for ending node
       edge attributes (if NULL, default attributes are used)
   Comment: in case the graph is undirected, this routine will change
   the graph into a directed graph */

graphlib_error_t graphlib_addDirectedEdge(graphlib_graph_p graph,
                                          graphlib_node_t node1, 
                                          graphlib_node_t node2,
                                          graphlib_edgeattr_p attr);


/*.......................................................*/
/* add a directed edge to a graph */
/* don't check if operation is legal */
/* IN: graph handle
       node ID for starting node
       node ID for ending node
       edge attributes (if NULL, default attributes are used)
   Comment: in case the graph is undirected, this routine will change
   the graph into a directed graph */

graphlib_error_t graphlib_addDirectedEdgeNoCheck(graphlib_graph_p graph,
                                                 graphlib_node_t node1, 
                                                 graphlib_node_t node2,
                                                 graphlib_edgeattr_p attr);


/*.......................................................*/
/* delete a node and all edges leading to and from it */
/* IN: graph handle
       node ID */

graphlib_error_t graphlib_deleteConnectedNode(graphlib_graph_p graph,
graphlib_node_t node); 


/*-----------------------------------------------------------------*/
/* Attribute Routines */

/*.......................................................*/
/* set a node attribute field to the default attributes */
/* IN: pointer to node attribute field */

graphlib_error_t graphlib_setDefNodeAttr(graphlib_nodeattr_p attr);


/*.......................................................*/
/* set an edge attribute field to the default attributes */
/* IN: pointer to edge attribute field */

graphlib_error_t graphlib_setDefEdgeAttr(graphlib_edgeattr_p attr);


/*.......................................................*/
/* Scale all node widths to fit into intervals */
/* IN: graph handle
       minimal value of all widths after the transformation
       maximal value of all widths after the transformation */

graphlib_error_t graphlib_scaleNodeWidth(graphlib_graph_p graph,                                                         graphlib_width_t minval,
                                         graphlib_width_t maxval);


/*.......................................................*/
/* Scale all edge widths to fit into intervals */
/* IN: graph handle
       minimal value of all widths after the transformation
       maximal value of all widths after the transformation */

graphlib_error_t graphlib_scaleEdgeWidth(graphlib_graph_p graph,
                                         graphlib_width_t minval,
                                         graphlib_width_t maxval);


/*-----------------------------------------------------------------*/
/* Annotation Routines */

/* Comment: these routines can only be used on graphs created
   with graphlib_newAnnotatedGraph */

/*.......................................................*/
/* set the name of an annotation - this will be used in GML exports
   to identify the annotation values */
/* IN: graph handle 
       annotation number to set name for
       name of annotation */

graphlib_error_t graphlib_AnnotationKey(graphlib_graph_p graph,
                                        int num,
                                        char *name);


/*.......................................................*/
/* Set an annotation value on a node */
/* IN: graph handle
       node ID
       annotation number to set value for
       value to be set */

graphlib_error_t graphlib_AnnotationSet(graphlib_graph_p graph,
                                        graphlib_node_t node, 
                                        int num, graphlib_annotation_t val);


/*.......................................................*/
/* Get an annotation value from a node */
/* IN: graph handle
       node ID
       annotation number to get value from
       pointer to value storage */

graphlib_error_t graphlib_AnnotationGet(graphlib_graph_p graph,
                                        graphlib_node_t node, 
                                        int num, graphlib_annotation_t* val);


/*-----------------------------------------------------------------*/
/* I/O routines */

/*.......................................................*/
/* load a graph stored in GraphLib's internal format */
/* IN: filename
       pointer to graph handle storage
   Comment: this routine will allocate a new graph */

graphlib_error_t graphlib_loadGraph(graphlib_filename_t fn,
                                        graphlib_graph_p *newgraph);


/*.......................................................*/
/* store a graph in GraphLib's internal format */
/* IN: filename
       graph handle */

graphlib_error_t graphlib_saveGraph(graphlib_filename_t fn,
                                    graphlib_graph_p graph);


/*.......................................................*/
/* export a graph in external format */
/* IN: filename
       format (use GRF_ constants)
       graph handle
   Comment: exported graphs can not be loaded again */

graphlib_error_t graphlib_exportGraph(graphlib_filename_t fn,
                                      graphlib_format_t format,
                                      graphlib_graph_p graph);


/*.......................................................*/
/* export a series graphs based on rank sets */
/* IN: filename
       format (use GRF_ constants)
       graph handle
   Comment: exported graphs can not be loaded again */

graphlib_error_t graphlib_extractAndExportTemporalGraphs(graphlib_filename_t fn,
                                                         graphlib_format_t format, 
                                                         graphlib_graph_p graph);


/*.......................................................*/
/* serialize a graph into a byte array for transfer */
/* IN: graph handle
       pointer to byte array
       pointer to return value (length of serialized graph) */

graphlib_error_t graphlib_serializeGraph(graphlib_graph_p igraph,
                                         char **obyte_array,
                                         unsigned int *obyte_array_len );


/*.......................................................*/
/* deserialize a graph from a byte array for transfer */
/* Assumes equal edge label width */
/* IN: graph handle
       pointer to byte array
       length of serialized graph */

graphlib_error_t graphlib_deserializeGraph(graphlib_graph_p *ograph,
                                           char *ibyte_array,
                                           unsigned int ibyte_array_len );


/*.......................................................*/
/* deserialize a graph from a byte array for transfer */
/* Assumes that graphlib was initialized using graphlib_InitVarEdgeLabelsConn
   and that the edge label width of the input graph matches the values
   passed at initialization (for the specified connection number) */
/* IN: graph handle
       pointer to byte array
       length of serialized graph */

graphlib_error_t graphlib_deserializeGraphConn(int connection,
                                               graphlib_graph_p *ograph,
                                               char *ibyte_array,
                                               unsigned int ibyte_array_len );


/*-----------------------------------------------------------------*/
/* Graph Merge Routines */

/*.......................................................*/
/* Merge two graphs, attributes of nodes and edges in frist graph dominate,
   resulting graph will be stored in first graph handle */
/* IN: graph handle to first graph
       graph handle to second graph
   Comment: this routine modifies the first graph */

graphlib_error_t graphlib_mergeGraphs(graphlib_graph_p graph1,
                                      graphlib_graph_p graph2);


/*.......................................................*/
/* Merge two graphs, average node and edge widths
   resulting graph will be stored in first graph handle */
/* IN: graph handle to first graph
       graph handle to second graph
   Comment: this routine modifies the first graph */

graphlib_error_t graphlib_mergeGraphsWeighted(graphlib_graph_p graph1,
                                      graphlib_graph_p graph2);


/*.......................................................*/
/* Merge two graphs, keep ranks of graphs in node names
   resulting graph will be stored in first graph handle */
/* IN: graph handle to first graph
       graph handle to second graph
   Comment: this routine modifies the first graph */

graphlib_error_t graphlib_mergeGraphsRanked(graphlib_graph_p graph1,
                                            graphlib_graph_p graph2);


/*.......................................................*/
/* Merge two graphs, but set empty edge labels 
   resulting graph will be stored in first graph handle */
/* IN: graph handle to first graph
       graph handle to second graph
   Comment: this routine modifies the first graph */

graphlib_error_t graphlib_mergeGraphsEmptyEdges(graphlib_graph_p graph1,
                                                graphlib_graph_p graph2);


/*.......................................................*/
/* Merge two graphs, and fill in the edge labels 
   resulting graph will be stored in first graph handle */
/* IN: graph handle to first graph
       graph handle to second graph
   Comment: this routine modifies the first graph */

graphlib_error_t graphlib_mergeGraphsFillEdges(graphlib_graph_p graph1,
                                               graphlib_graph_p graph2, int *ranks, int ranks_size, int offset);


/*-----------------------------------------------------------------*/
/* Analysis routines */

/* Warning: Some of these routines are very application specific */

/*.......................................................*/
/* start at a node and traverse a directed graph backwards
   during traversal color graph in given color */
/* IN: graph handle
       color to set
       node to start at
   Comment: this routine modifies the specified graph */

graphlib_error_t graphlib_colorInvertedPath(graphlib_graph_p gr,
                                            graphlib_color_t color,
                                            graphlib_node_t node);


/*.......................................................*/
/* start at a node and traverse a directed graph backwards
   during traversal color graph in given color and delete nodes
   not connected to this path */
/* IN: graph handle
       color to set
       node to start at
   Comment: this routine modifies the specified graph */

graphlib_error_t graphlib_colorInvertedPathDeleteRest(graphlib_graph_p gr,
                                                      graphlib_color_t color,
                                                      graphlib_color_t color_off,
                                                      graphlib_node_t node);


/*.......................................................*/
/* currently undocumented */

graphlib_error_t graphlib_deleteInvertedPath(graphlib_graph_p gr,
                                             graphlib_node_t node,
                                             graphlib_node_t *lastnode);


/*.......................................................*/
/* currently undocumented */

graphlib_error_t graphlib_deleteInvertedLine(graphlib_graph_p gr,
                                             graphlib_node_t node,
                                             graphlib_node_t *lastnode);


/*.......................................................*/
/* delete a tree from a given node, but leave the node */
/* IN: graph handle
       root node
   Comment: this routine modifies the specified graph */

graphlib_error_t graphlib_deleteTreeNotRoot(graphlib_graph_p gr,
                                            graphlib_node_t node);


/*.......................................................*/
/* delete a tree from a given node, including the node */
/* IN: graph handle
       root node
   Comment: this routine modifies the specified graph */

graphlib_error_t graphlib_deleteTree(graphlib_graph_p gr,
                                     graphlib_node_t node);


/*.......................................................*/
/* delete a tree from a given node, but leave the node */
/* stop when a node has the given color */
/* IN: graph handle
       root node
       color node color to stop deleting at
   Comment: this routine modifies the specified graph */

graphlib_error_t graphlib_deleteTreeNotRootColor(graphlib_graph_p gr,
                                                 graphlib_node_t node,
                                                 graphlib_color_t color);


/*.......................................................*/
/* delete a tree from a given node, including the node */
/* stop when a node has the given color */
/* IN: graph handle
       root node
       color node color to stop deleting at
   Comment: this routine modifies the specified graph */

graphlib_error_t graphlib_deleteTreeColor(graphlib_graph_p gr,
                                          graphlib_node_t node,
                                          graphlib_color_t color);


/*.......................................................*/
/* collapse chains of nodes with identical x coordinates */
/* IN: graph handle
   Comment: this routine modifies the specified graph */

graphlib_error_t graphlib_collapseHor(graphlib_graph_p gr);


/*.......................................................*/
/* auto-color subtrees according to edge labels */
/* IN: graph handle
   Comment: this routine modifies the specified graph */

graphlib_error_t graphlib_colorGraphByLeadingEdgeLabel(graphlib_graph_p gr);


/*-----------------------------------------------------------------*/

#ifdef STAT_BITVECTOR
/*.......................................................*/
/* add a task to a bit vector by its rank */
/* IN: the bit vector and the integer task rank
   Comment: this routine modifies the input bit vector */
graphlib_error_t graphlib_setEdgeByTask(void **edgelist, int task);

/*.......................................................*/
/* modify all edge attributes in a graph */
/* IN: the graph and the new edge attribute
   Comment: this routine modifies the input graph */
graphlib_error_t graphlib_modifyEdgeAttr(graphlib_graph_p graph,
                                         graphlib_edgeattr_p attr);

int graphlib_getBitVectorSize();
#endif /*ifdef STAT_BITVECTOR*/

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */


#endif /* defined( __graphlib_h ) */


/*-----------------------------------------------------------------*/
/* The End. */

