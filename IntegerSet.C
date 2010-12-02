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


#include "IntegerSet.h"

#include <errno.h>

#ifdef STAT_BITVECTOR
static const int bv_typebits = bv_typesize * 8;
static int bv_edgelabelwidth = 1;
static int bv_edgelabelbits  = 8;
static int bv_init = 0;

/* This is the length of the bit vector in longs (i.e., length of void *edgelist array) */

/* bitshifting is MUCH faster than ANY memory access */
#define BIT(i) (1 << (i))

/* Hash the bit vector and create a (fairly) unique long integer id */
/* There may be collisions, but they should be rare */
long bithash(const void *bit_vector, int width)
{
  int i;
  long ret = 0;
  
  for ( i=0; i<width; i=i+1 )
    {
      ret = ret + ((bv_type *)bit_vector)[i] * (width - i + 1);
    }
  return ret;
}

int bitvec_initialize(int longsize, int edgelabelwidth)
{
  bv_edgelabelwidth=edgelabelwidth;
  bv_edgelabelbits=edgelabelwidth*bv_typebits;

  bv_init++;
  return 0;
}

void bitvec_finalize()
{
}

void* bitvec_allocate()
{
  void *newlabel = calloc(bv_edgelabelwidth, bv_typesize);
  if (!newlabel)
    fprintf (stderr, "%s(%i): calloc failed to allocate %d bytes. error = %s\n",
	     __func__, __LINE__ - 1, bv_edgelabelwidth *bv_typesize, strerror (errno));

  return newlabel;
}

void bitvec_delete(void** edgelist)
{
  free(*edgelist);
  *edgelist=NULL;
}

void bitvec_erase(void* edgelist)
{
  memset(edgelist,0,bv_edgelabelwidth*bv_typesize);
}


void bitvec_insert(void *vec, int val)
{
  int byte = val / bv_typebits;
  int bit = val % bv_typebits;

  ((bv_type *)vec)[byte] |= BIT(bit);
}

void bitvec_merge(void *inout, void *in)
{
  int i;

  for (i=0; i<bv_edgelabelwidth; i++)
    {
      ((bv_type *)inout)[i] |= ((bv_type *)in)[i];
    }
}

int bitvec_contains(void *vec, int val)
{
  int byte = val / bv_typebits;
  int bit = val % bv_typebits;

  return !!(((bv_type *)vec)[byte] & BIT(bit));
}

unsigned int bitvec_size(void *vec)
{
  int i, j, ret = 0;

  for (i=0; i<bv_edgelabelwidth; i++)
    {
      for (j=0; j<bv_typebits; j++)
        {
	  ret += !!(((bv_type *)vec)[i] & BIT(j));
        }
    }
  return ret;
}


void bitvec_c_str_file(FILE *f, void *vec)
{
  char val[128];
  int i, j;
  int in_range=0, first_iteration=1, range_start, range_end, cur_val, last_val=0;

  fprintf(f, "[");
  for (i=0; i<bv_edgelabelwidth; i++)
    {
      for (j=0; j<bv_typebits; j++)
        {
          if (((bv_type *)vec)[i] & BIT(j))
            {
              cur_val = i*bv_typebits+j;
              if (in_range == 0)
                {
                  snprintf(val, 128, "%d", cur_val);
                  if (first_iteration == 0)
                    {
                      if (cur_val == last_val + 1)
                        {
                          in_range = 1;
                          range_start = last_val;
                          range_end = cur_val;
                          fprintf(f, "-");
                        }        
                      else
                        {
                          fprintf(f, ",");
                          fprintf(f, "%s", val);
                        }
                    }
                  else
                    {
                      fprintf(f, "%s", val);
                    }
                }
              else
                {
                  if (cur_val == last_val + 1)
                    {
                      range_end = cur_val;
                    }
                  else
                    {
                      snprintf(val, 128, "%d,%d", last_val, cur_val);
                      fprintf(f, "%s", val);
                      in_range = 0;
                    }
                }
              first_iteration = 0;
              last_val = cur_val;
            }
        }
    }
  if (in_range == 1)
    {
      snprintf(val, 128, "%d", last_val);
      fprintf(f, "%s", val);
    }
  fprintf(f, "]");
}

#else /*!STAT_BITVECTOR*/

IntegerSet::IntegerSet( const char * istr )
{
    insert( istr );
}

void IntegerSet::insert( int ival )
{
    /*fprintf( stderr, "%d: Merging %d into \"%s\"...\n", getpid(), ival, c_str() );*/
    _set.insert( ival );
}

void IntegerSet::insert( const char * istr  )
{
    unsigned int start_pos, end_pos;
    char int_str[256];
    unsigned int cur_int, range_start=0;
    bool in_range=false;

    /*fprintf( stderr, "%d: Merging \"%s\" into \"%s\"...\n", getpid(), istr, c_str() );*/
    for( unsigned int i=0; istr[i] != ']'; ) {
        if( istr[i] == '[' ){
            i++;
            continue;
        }

        if( isdigit( istr[i] ) ){
            start_pos = i;
            while( isdigit( istr[i] ) ){
                i++;
            }
            end_pos = i;

            memcpy( int_str, (const void *)(istr+start_pos), end_pos-start_pos);
            /*fprintf( stderr, "\t%d:%d: end: %d, start: %d, end-start:%d",*/
                     /*getpid(), __LINE__, end_pos, start_pos, end_pos-start_pos );*/
            int_str[end_pos-start_pos]='\0';
            cur_int = atoi( int_str );
            /*fprintf( stderr, "\t%d:%d: converted: \"%s\" to %d\n",*/
                     /*getpid(), __LINE__, int_str, cur_int );*/
        }

        if( in_range ){
            for( unsigned int j=range_start; j <= cur_int; j++ ){
                /*fprintf( stderr, "\t%d:%d: inserting: %d, istr_pos: %d\n", getpid(), __LINE__, j, i);*/
                _set.insert( j );
                in_range = false;
            }
        }
        else{
            if( istr[i] == ',' ){
                /*fprintf( stderr, "\t%d:%d: inserting: %d, istr_pos: %d\n", getpid(), __LINE__, cur_int, i);*/
                _set.insert( cur_int );
            }
            else if( istr[i] == ']' ){
                /*fprintf( stderr, "\t%d:%d: inserting: %d, istr_pos: %d\n", getpid(), __LINE__, cur_int, i);*/
                _set.insert( cur_int );
                break;
            }
            else if( istr[i] == '-' ){
                in_range = true;
                range_start = cur_int;
            }
            i++;
        }
    }
}

bool IntegerSet::contains( int i )
{
    if( _set.find( i ) == _set.end() )
        return false;

    return true;
}

const char * IntegerSet::c_str( )
{
    char int_str[256];
    unsigned int range_start, range_end, cur_val, last_val=0;
    bool in_range=false, first_iteration=true;

    _str_set = '[';

    set< unsigned int >::iterator set_iter;

    for( set_iter=_set.begin(); set_iter!=_set.end(); set_iter++ ){
        cur_val = *set_iter;

        if( !in_range ){
            snprintf( int_str, 256, "%u", cur_val );
            if( !first_iteration ){
                if( cur_val == last_val+1 ){
                    in_range = true;
                    range_start = last_val;
                    range_end = cur_val;
                    _str_set += '-';
                }
                else{
                    _str_set += ',';
                    _str_set += int_str;
                }
            }
            else{
                _str_set += int_str;
            }
        }
        else{
            if( cur_val == last_val+1 ){
                range_end = cur_val;
            }
            else{
                /*range has ended; dump last_val (range_end) and cur_val*/
                snprintf( int_str, 256, "%u,%u", last_val, cur_val );
                _str_set += int_str; 
                in_range=false;
            }
        }

        first_iteration=false;
        last_val = cur_val;
    }

    if( in_range ) {
        snprintf( int_str, 256, "%u", last_val );
        _str_set += int_str; 
    }

    _str_set += ']';

    return _str_set.c_str();
}

void IntegerSet::print()
{
    set< unsigned int> ::iterator set_iter;

    for( set_iter = _set.begin(); set_iter != _set.end(); set_iter++ ){
        fprintf(stderr, "%u ", *set_iter );
    }
}

#if defined (TEST_INTEGERSET)
int main ()
{
    IntegerSet a,b,c;

    a.insert( 0 );
    a.insert( 2 );
    for( unsigned int i=4; i<13; i++ )
        a.insert( i );

    a.insert( 0 );
    a.insert( 2 );

    a.insert( 60 );
    a.insert( 27 );
    a.insert( 13 );
    a.insert( 59 );

    fprintf( stderr, "a: %s\n", a.c_str() );

    b.insert( a.c_str() );

    fprintf( stderr, "b: ");
    b.print();

    fprintf( stderr, "b: %s\n", b.c_str() );
}
#endif 

#endif /*STAT_BIT_VECTOR*/
