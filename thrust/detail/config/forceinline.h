/*
 *  Copyright 2008-2013 NVIDIA Corporation
 *  Modifications Copyright© 2019 Advanced Micro Devices, Inc. All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*! \file forceinline.h
 *  \brief Defines __thrust_forceinline__
 */

#pragma once

#include <thrust/detail/config.h>

#if defined(__CUDACC__) || defined(__NVCOMPILER_CUDA__)

#define __thrust_forceinline__ __forceinline__

#elif THRUST_DEVICE_COMPILER == THRUST_DEVICE_COMPILER_HIP

#define __thrust_forceinline__ __forceinline__

#else

// TODO add

#define __thrust_forceinline__

#endif
