/*
  mallocMC: Memory Allocator for Many Core Architectures.

  Copyright 2015 Institute of Radiation Physics,
                 Helmholtz-Zentrum Dresden - Rossendorf

  Author(s):  Benjamin Worpitz - HZDR

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#pragma once

#include <string>

#include "NoOp.hpp"

namespace mallocMC{
namespace ReservePoolPolicies{

  struct NoOp{
    MAMC_ACCELERATOR
    static void* setMemPool(size_t memsize){
      return NULL;
    }

    MAMC_ACCELERATOR
    static void resetMemPool(void *p=NULL){
    }

    MAMC_HOST
    static std::string classname(){
      return "NoOp";
    }

  };

} //namespace ReservePoolPolicies
} //namespace mallocMC
