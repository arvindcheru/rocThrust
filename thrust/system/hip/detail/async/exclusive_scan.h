/******************************************************************************
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the NVIDIA CORPORATION nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#pragma once

#include <thrust/detail/config.h>
#include <thrust/detail/cpp14_required.h>

#if THRUST_CPP_DIALECT >= 2014

#if THRUST_DEVICE_COMPILER == THRUST_DEVICE_COMPILER_HIP

#include <thrust/iterator/iterator_traits.h>

#include <thrust/system/hip/config.h>
#include <thrust/system/hip/detail/async/customization.h>
#include <thrust/system/hip/detail/util.h>
#include <thrust/system/hip/future.h>

#include <thrust/type_traits/remove_cvref.h>

#include <thrust/distance.h>

#include <type_traits>

// TODO specialize for thrust::plus to use e.g. ExclusiveSum instead of ExcScan
//  - Note that thrust::plus<> is transparent, cub::Sum is not. This should be
//    fixed in CUB first).
//  - Need to check if CUB actually optimizes for sums before putting in effort

THRUST_NAMESPACE_BEGIN
namespace system
{
namespace hip
{
namespace detail
{

template <typename DerivedPolicy,
          typename ForwardIt,
          typename Size,
          typename OutputIt,
          typename InitialValueType,
          typename BinaryOp>
unique_eager_event
async_exclusive_scan_n(execution_policy<DerivedPolicy>& policy,
                       ForwardIt first,
                       Size n,
                       OutputIt out,
                       InitialValueType init,
                       BinaryOp op)
{
  auto const device_alloc = get_async_device_allocator(policy);
  unique_eager_event ev;

  // Determine temporary device storage requirements.
  size_t tmp_size = 0;
  {
    thrust::hip_rocprim::throw_on_error(
        rocprim::exclusive_scan(nullptr,
                                tmp_size,
                                first,
                                out,
                                init,
                                n,
                                op,
                                nullptr,
                                THRUST_HIP_DEBUG_SYNC_FLAG),
        "after determining tmp storage "
        "requirements for exclusive_scan"
    );
  }

  // Allocate temporary storage.
  auto content = uninitialized_allocate_unique_n<thrust::detail::uint8_t>(
    device_alloc, tmp_size
  );
  void* const tmp_ptr = raw_pointer_cast(content.get());

  // Set up stream with dependencies.
  hipStream_t const user_raw_stream = thrust::hip_rocprim::stream(policy);

  if (thrust::hip_rocprim::default_stream() != user_raw_stream)
  {
    ev = make_dependent_event(
      std::tuple_cat(
        std::make_tuple(
          std::move(content),
          unique_stream(nonowning, user_raw_stream)
        ),
        extract_dependencies(std::move(thrust::detail::derived_cast(policy)))));
  }
  else
  {
    ev = make_dependent_event(
      std::tuple_cat(
        std::make_tuple(std::move(content)),
        extract_dependencies(std::move(thrust::detail::derived_cast(policy)))));
  }

  // Run scan.
  {
    thrust::hip_rocprim::throw_on_error(
        rocprim::exclusive_scan(tmp_ptr,
                                tmp_size,
                                first,
                                out,
                                init,
                                n,
                                op,
                                user_raw_stream,
                                THRUST_HIP_DEBUG_SYNC_FLAG),
        "after dispatching exclusive_scan kernel"
    );
  }

  return ev;
}

}}} // namespace system::hip::detail

namespace hip_rocprim
{

// ADL entry point.
template <typename DerivedPolicy,
          typename ForwardIt,
          typename Sentinel,
          typename OutputIt,
          typename InitialValueType,
          typename BinaryOp>
auto async_exclusive_scan(execution_policy<DerivedPolicy>& policy,
                          ForwardIt first,
                          Sentinel&& last,
                          OutputIt&& out,
                          InitialValueType &&init,
                          BinaryOp&& op)
THRUST_RETURNS(
  thrust::system::hip::detail::async_exclusive_scan_n(
    policy,
    first,
    distance(first, THRUST_FWD(last)),
    THRUST_FWD(out),
    THRUST_FWD(init),
    THRUST_FWD(op)
  )
)

} // namespace hip_rocprim

THRUST_NAMESPACE_END

#endif // THRUST_DEVICE_COMPILER == THRUST_DEVICE_COMPILER_HIP

#endif // C++14
