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


#if !defined (__integer_set_h)
#define __integer_set_h 1

#ifdef STAT_BITVECTOR

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GRL_BV_CHAR
typedef char bv_type;
#elif GRL_BV_INT
typedef int bv_type;
#elif GRL_BV_SHORT
typedef short bv_type;
#else
typedef long bv_type;
#endif

const int bv_typesize = sizeof (bv_type);

long bithash(const void *bit_vector, int width);
void* bitvec_allocate();
void bitvec_delete(void** edgelist);
void bitvec_erase(void* edgelist);
int bitvec_initialize(int longsize,int edgelabelwidth);
void bitvec_finalize();
void bitvec_insert(void *vec, int val);
void bitvec_merge(void *inout, void *in);
int bitvec_contains(void *vec, int val);
unsigned int bitvec_size(void *vec);

/*GLL: We can remove this function... it failed at 128K tasks, presumably because the string
     got too large.  The bitvec_c_str_file() function replaces this one.
char *bitvec_c_str(void *vec, int size);*/

void bitvec_c_str_file(FILE* f, void *vec);

#ifdef __cplusplus
}
#endif

#else /*!STAT_BITVECTOR*/

#ifndef __cplusplus
struct IntegerSet;
#else  // __cplusplus

#include <set>
#include <string>
using namespace std;

class IntegerSet{
 private:
    set< unsigned int > _set;
    string _str_set;

 public:
    IntegerSet(){};
    IntegerSet( const char * );
    void insert( int );
    void insert( const char *  );
    bool contains( int );
    unsigned int size( ){ return _set.size(); };
    const char * c_str( );
    void print( );
};

#endif // __cplusplus
#endif /*STAT_BIT_VECTOR*/

#endif /* __integer_set_h */
