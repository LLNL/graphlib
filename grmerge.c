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
#include <string.h>
#include "graphlib.h"

#define CHECKERROR(err,s) \
{ \
  if (GRL_IS_NOTOK(err)) \
  { \
    printf("Error %i at %s\n",err,s); \
    if (GRL_IS_FATALERROR(err)) \
    { \
      printf("Fatal error - exiting ...\n"); \
      exit(1); \
    } \
  } \
}

/*-----------------------------------------------------*/

void usageerror()
{
  printf("Usage: grmerge [-e <format>] [-cp <stage flags>] infile1 [infile2 ... ] outfile\n");
  printf("Format can be: 0 = DOT\n");
  printf("               1 = GML\n");
  printf("Stage flags can be:  1 = Prune tree\n");
  printf("                     2 = Scale nodes\n");
  printf("                     4 = Collapse chains\n");
  exit(0);
}

/*-----------------------------------------------------*/
/* MAIN */

int main(int argc, char **argv)
{
  int               ac,i;
  graphlib_graph_p  gr,gradd;
  graphlib_format_t format = GRF_NOEXPORT;
  graphlib_error_t  err;
  int critpath=0;

  printf("Graphlib-Merge Utility, Martin Schulz, LLNL, 2005\n");

  if (argc<3)
    usageerror();

  if (strcmp(argv[1],"-e")==0)
    {
      if (argc<5)
	usageerror();
      format=atoi(argv[2]);
      ac=3;
    }
  else
    {
      ac=1;
    }

  if (strcmp(argv[ac],"-cp")==0)
    {
      if (argc<4+ac)
	usageerror();
      critpath=atoi(argv[ac+1]);
      ac++;
      ac++;
    }

  printf("Loading %s\n",argv[ac]);
  err=graphlib_loadGraph(argv[ac],&gr,NULL); 
  CHECKERROR(err,"Load initial graph");  

  for (i=ac+1; i<argc-1; i++)
    {
      printf("Loading %s\n",argv[i]);
      err=graphlib_loadGraph(argv[i],&gradd,NULL); 
      CHECKERROR(err,"Load additional graph");

      err=graphlib_mergeGraphs(gr,gradd);
      CHECKERROR(err,"Merging Graph");  

      err=graphlib_delGraph(gradd);
      CHECKERROR(err,"Deleting additional Graph");  
    }

  
  if (critpath & 1)
    {
      printf("Creating path\n");
      err=graphlib_colorInvertedPathDeleteRest(gr,GRC_RED,GRC_GREEN,0);
      CHECKERROR(err,"Coloring Graph");  
    }
      
  if (critpath & 2)
    {
      printf("Scaling node\n");
      err=graphlib_scaleNodeWidth(gr,10.0,100.0);
      CHECKERROR(err,"Scaling Graph");  
    }
      
  if (critpath & 4)
    {
      printf("Collapsing chains\n");
      err=graphlib_collapseHor(gr);
      CHECKERROR(err,"Collapsing Graph");  
    }

  printf("Exporting graph\n");

  if (format==GRF_NOEXPORT)
    {
      err=graphlib_saveGraph(argv[argc-1],gr);
      CHECKERROR(err,"Saving Graph");  
    }
  else
    {
      err=graphlib_exportGraph(argv[argc-1],format,gr);
      CHECKERROR(err,"Exporting Graph");  
    }

  printf("Cleaning up\n");

  err=graphlib_delGraph(gr);
  CHECKERROR(err,"Deleting initial Graph");  

  return 0;
}
