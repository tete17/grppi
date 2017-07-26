/**
* @version		GrPPI v0.2
* @copyright		Copyright (C) 2017 Universidad Carlos III de Madrid. All rights reserved.
* @license		GNU/GPL, see LICENSE.txt
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You have received a copy of the GNU General Public License in LICENSE.txt
* also available in <http://www.gnu.org/licenses/gpl.html>.
*
* See COPYRIGHT.txt for copyright notices and details.
*/

#ifndef GRPPI_TBB_STENCIL_H
#define GRPPI_TBB_STENCIL_H

#ifdef GRPPI_TBB

#include <tbb/tbb.h>

#include "parallel_execution_tbb.h"

namespace grppi{
template <typename InputIt, typename OutputIt, typename Operation, typename NFunc>
 void stencil(parallel_execution_tbb &p, InputIt first, InputIt last, 
  OutputIt firstOut, Operation && op, NFunc && neighbor ) {

  int numElements = last - first;
  int elemperthr = numElements/p.concurrency_degree();
  tbb::task_group g;

  for(int i=1;i<p.concurrency_degree();i++){

     g.run( [&neighbor, &op, first, firstOut, elemperthr, i, last, p ]() {
        auto begin = first + (elemperthr * i);
        auto end = first + (elemperthr * (i+1));
        if( i == p.concurrency_degree()-1) end = last;
        auto out = firstOut + (elemperthr * i);
        while(begin!=end){
          auto neighbors = neighbor(begin);
          *out = op(begin, neighbors);
          begin++;
          out++;
        }
      }
    );
   }
     //MAIN 
   auto end = first + elemperthr;
   while(first!=end){
    auto neighbors = neighbor(first);
    *firstOut = op(first, neighbors);
    first++;
    firstOut++;
  }

  g.wait();

}

template <typename InputIt, typename OutputIt, typename ... MoreIn, typename Operation, typename NFunc>
void internal_stencil(parallel_execution_tbb &p, int elemperthr, int index, InputIt first,
            InputIt last, OutputIt firstOut, Operation op, NFunc neighbor, MoreIn ... inputs){
  auto begin = first + (elemperthr * index);
  auto end = first + (elemperthr * (index+1));
  if (index==p.concurrency_degree()-1) end = last;
  auto out = firstOut + (elemperthr * index);
  advance_iterators((elemperthr* index), inputs ...);
  while(begin!=end){
    auto neighbors = neighbor(begin, inputs ...);
    *out = op(begin, neighbors);
    begin++;
    advance_iterators( inputs ... );
    out++;
  }
}


template <typename InputIt, typename OutputIt, typename ... MoreIn, typename Operation, typename NFunc>
void stencil(parallel_execution_tbb & p, InputIt first, InputIt last, OutputIt firstOut, Operation op, NFunc neighbor, MoreIn ... inputs ) {
  int numElements = last - first;
  int elemperthr = numElements/p.concurrency_degree();
  tbb::task_group g;
  for(int index=1;index<p.concurrency_degree();index++){
    g.run([neighbor, op, first, firstOut, elemperthr, index, last, &p, inputs...](){
      internal_stencil(p,elemperthr,index,first, last, firstOut, op, neighbor, inputs ...);
    });
  }
  //MAIN 
  auto end = first + elemperthr;
  while(first!=end){
    auto neighbors = neighbor(first,inputs ...);
    *firstOut = op(first, neighbors);
    first++;
    advance_iterators( inputs ... );
    firstOut++;
  }

  //Join threads
  g.wait();
}



}
#endif

#endif
