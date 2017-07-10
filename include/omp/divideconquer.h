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

#ifndef GRPPI_DIVIDECONQUER_OMP_H
#define GRPPI_DIVIDECONQUER_OMP_H

#ifdef GRPPI_OMP

#include <thread>

namespace grppi{

template <typename Input, typename DivFunc, typename Operation, typename MergeFunc>
typename std::result_of<Operation(Input)>::type internal_divide_conquer(parallel_execution_omp &p, Input & problem, 
            DivFunc && divide, Operation && op, MergeFunc && merge, std::atomic<int> & num_threads) {
   
    // Sequential execution fo internal implementation
    using  Output = typename std::result_of<Operation(Input)>::type;
    sequential_execution seq;
    Output out;
    if(num_threads.load()>0){
       auto subproblems = divide(problem);


    if(subproblems.size()>1){
         std::vector<Output> partials(subproblems.size()-1);
         int division = 0;
         auto i = subproblems.begin()+1;
         for(i; i != subproblems.end()&& num_threads.load()>0; i++, division++){
            //THREAD
             #pragma omp task firstprivate(i, division) shared(divide, op, merge, partials, num_threads)
             {
                 partials[division] = internal_divide_conquer(p, *i, std::forward<DivFunc>(divide), std::forward<Operation>(op), std::forward<MergeFunc>(merge), num_threads);
              //END TRHEAD
             }
             num_threads --;
             
          }
          for(i; i != subproblems.end(); i++){
              partials[division] = divide_conquer(seq,*i, divide, op, merge);
          }
          
          //Main thread works on the first subproblem.
          out = internal_divide_conquer(p, *subproblems.begin(), std::forward<DivFunc>(divide), std::forward<Operation>(op), std::forward<MergeFunc>(merge), num_threads);

          #pragma omp taskwait

          for(int i = 0; i<partials.size();i++){ 
              merge(partials[i], out);
          }
        }else{
          out = op(problem);
        }
     }else{
        return divide_conquer(seq, problem, std::forward<DivFunc>(divide), std::forward<Operation>(op), std::forward<MergeFunc>(merge));
     }
     return out;



}

template <typename Input, typename DivFunc, typename Operation, typename MergeFunc>
typename std::result_of<Operation(Input)>::type divide_conquer(parallel_execution_omp &p, Input & problem, 
            DivFunc && divide, Operation && op, MergeFunc && merge) {

    // Sequential execution fo internal implementation
    sequential_execution seq;
    using Output = typename std::result_of<Operation(Input)>::type;
    std::atomic<int> num_threads( p.num_threads -1 );
    Output out;
    if(num_threads.load()>0){
       auto subproblems = divide(problem);

      if(subproblems.size()>1){
          std::vector<Output> partials(subproblems.size()-1);
    	  int division = 0;
  
          #pragma omp parallel
          {
          #pragma omp single nowait
          { 
          auto i = subproblems.begin()+1;
          for(i; i != subproblems.end()&& num_threads.load()>0; i++, division++){
              //THREAD
              #pragma omp task firstprivate(i,division) shared(partials,divide,op,merge, num_threads)
              {
                  partials[division] = internal_divide_conquer(p, *i,  std::forward<DivFunc>(divide), std::forward<Operation>(op), std::forward<MergeFunc>(merge), num_threads);
              //END TRHEAD
              }
                  num_threads --;

          }
          for(i; i != subproblems.end(); i++){
              partials[division] = divide_conquer(seq,*i, std::forward<DivFunc>(divide), std::forward<Operation>(op), std::forward<MergeFunc>(merge));
          }

    	  //Main thread works on the first subproblem.
        
         if(num_threads.load()>0){
            out = internal_divide_conquer(p, *subproblems.begin(),  std::forward<DivFunc>(divide), std::forward<Operation>(op), std::forward<MergeFunc>(merge), num_threads);
        }else{
            out = divide_conquer(seq, *subproblems.begin(),  std::forward<DivFunc>(divide), std::forward<Operation>(op), std::forward<MergeFunc>(merge));
        }
          #pragma omp taskwait
          }
          }
          for(int i = 0; i<partials.size();i++){ 
              merge(partials[i], out);
          }
        }else{
          out = op(problem);
        }
    }else{
        return divide_conquer(seq, problem, std::forward<DivFunc>(divide), std::forward<Operation>(op), std::forward<MergeFunc>(merge));
    }
    return out;
}


}
#endif

#endif