/**
 * Copyright 2015 Kurt Kanzenbach
 * Copyright 2016-2017 Andreas Schäfer
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef FLAT_ARRAY_DETAIL_SHORT_VEC_AVX_INT_16_HPP
#define FLAT_ARRAY_DETAIL_SHORT_VEC_AVX_INT_16_HPP

#if LIBFLATARRAY_WIDEST_VECTOR_ISA == LIBFLATARRAY_AVX2

#include <libflatarray/detail/sqrt_reference.hpp>
#include <libflatarray/detail/short_vec_helpers.hpp>
#include <libflatarray/config.h>
#include <libflatarray/short_vec_base.hpp>

// disable certain warnings from system headers when compiling with
// Microsoft Visual Studio:
#ifdef _MSC_BUILD
#pragma warning( push )
#pragma warning( disable : 4514 )
#endif

#include <immintrin.h>

#ifdef LIBFLATARRAY_WITH_CPP14
#include <initializer_list>
#endif

#ifdef _MSC_BUILD
#pragma warning( pop )
#endif

namespace LibFlatArray {

template<typename CARGO, std::size_t ARITY>
class short_vec;

template<typename CARGO, std::size_t ARITY>
class sqrt_reference;

#ifdef __ICC
// disabling this warning as implicit type conversion is exactly our goal here:
#pragma warning push
#pragma warning (disable: 2304)
#endif

template<>
class short_vec<int, 16> : public short_vec_base<int, 16>
{
public:
    static const std::size_t ARITY = 16;

    typedef short_vec_strategy::avx2 strategy;

    template<typename _CharT, typename _Traits>
    friend std::basic_ostream<_CharT, _Traits>& operator<<(
        std::basic_ostream<_CharT, _Traits>& __os,
        const short_vec<int, 16>& vec);

    inline
    short_vec(const int data = 0) :
        val{_mm256_set1_epi32(data),
            _mm256_set1_epi32(data)}
    {}

    inline
    short_vec(const int *data)
    {
        load(data);
    }

    inline
    short_vec(
        const __m256i& val1,
        const __m256i& val2) :
        val{val1,
            val2}
    {}

#ifdef LIBFLATARRAY_WITH_CPP14
    inline
    short_vec(const std::initializer_list<int>& il)
    {
        const int *ptr = static_cast<const int *>(&(*il.begin()));
        load(ptr);
    }
#endif

    inline
    short_vec(const sqrt_reference<int, 16>& other);

    inline
    void operator-=(const short_vec<int, 16>& other)
    {
        val[ 0] = _mm256_sub_epi32(val[ 0], other.val[ 0]);
        val[ 1] = _mm256_sub_epi32(val[ 1], other.val[ 1]);
    }

    inline
    short_vec<int, 16> operator-(const short_vec<int, 16>& other) const
    {
        return short_vec<int, 16>(
            _mm256_sub_epi32(val[ 0], other.val[ 0]),
            _mm256_sub_epi32(val[ 1], other.val[ 1]));
    }

    inline
    void operator+=(const short_vec<int, 16>& other)
    {
        val[ 0] = _mm256_add_epi32(val[ 0], other.val[ 0]);
        val[ 1] = _mm256_add_epi32(val[ 1], other.val[ 1]);
    }

    inline
    short_vec<int, 16> operator+(const short_vec<int, 16>& other) const
    {
        return short_vec<int, 16>(
            _mm256_add_epi32(val[ 0], other.val[ 0]),
            _mm256_add_epi32(val[ 1], other.val[ 1]));
    }

    inline
    void operator*=(const short_vec<int, 16>& other)
    {
        val[ 0] = _mm256_mullo_epi32(val[ 0], other.val[ 0]);
        val[ 1] = _mm256_mullo_epi32(val[ 1], other.val[ 1]);
    }

    inline
    short_vec<int, 16> operator*(const short_vec<int, 16>& other) const
    {
        return short_vec<int, 16>(
            _mm256_mullo_epi32(val[ 0], other.val[ 0]),
            _mm256_mullo_epi32(val[ 1], other.val[ 1]));
    }

    inline
    void operator/=(const short_vec<int, 16>& other)
    {
        val[ 0] = _mm256_cvtps_epi32(_mm256_div_ps(_mm256_cvtepi32_ps(val[ 0]),
                                                _mm256_cvtepi32_ps(other.val[ 0])));
        val[ 1] = _mm256_cvtps_epi32(_mm256_div_ps(_mm256_cvtepi32_ps(val[ 1]),
                                                _mm256_cvtepi32_ps(other.val[ 1])));
    }

    inline
    void operator/=(const sqrt_reference<int, 16>& other);

    inline
    short_vec<int, 16> operator/(const short_vec<int, 16>& other) const
    {
        return short_vec<int, 16>(
            _mm256_cvttps_epi32(_mm256_div_ps(
                                    _mm256_cvtepi32_ps(val[ 0]),
                                    _mm256_cvtepi32_ps(other.val[ 0]))),
            _mm256_cvttps_epi32(_mm256_div_ps(
                                    _mm256_cvtepi32_ps(val[ 1]),
                                    _mm256_cvtepi32_ps(other.val[ 1]))));
    }

    inline
    short_vec<int, 16> operator/(const sqrt_reference<int, 16>& other) const;

    inline
    short_vec<int, 16> sqrt() const
    {
        return short_vec<int, 16>(
            _mm256_cvtps_epi32(
                _mm256_sqrt_ps(_mm256_cvtepi32_ps(val[ 0]))),
            _mm256_cvtps_epi32(
                _mm256_sqrt_ps(_mm256_cvtepi32_ps(val[ 1]))));
    }

    inline
    void load(const int *data)
    {
        val[ 0] = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(data + 0));
        val[ 1] = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(data + 8));
    }

    inline
    void load_aligned(const int *data)
    {
        SHORTVEC_ASSERT_ALIGNED(data, 32);
        val[ 0] = _mm256_load_si256(reinterpret_cast<const __m256i *>(data + 0));
        val[ 1] = _mm256_load_si256(reinterpret_cast<const __m256i *>(data + 8));
    }

    inline
    void store(int *data) const
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(data + 0), val[ 0]);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(data + 8), val[ 1]);
    }

    inline
    void store_aligned(int *data) const
    {
        SHORTVEC_ASSERT_ALIGNED(data, 32);
        _mm256_store_si256(reinterpret_cast<__m256i *>(data + 0), val[ 0]);
        _mm256_store_si256(reinterpret_cast<__m256i *>(data + 8), val[ 1]);
    }

    inline
    void store_nt(int *data) const
    {
        SHORTVEC_ASSERT_ALIGNED(data, 32);
        _mm256_stream_si256(reinterpret_cast<__m256i *>(data + 0), val[ 0]);
        _mm256_stream_si256(reinterpret_cast<__m256i *>(data + 8), val[ 1]);
    }

    inline
    void gather(const int *ptr, const int *offsets)
    {
        __m256i indices1 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(offsets + 0));
        __m256i indices2 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(offsets + 8));
        val[ 0] = _mm256_i32gather_epi32(ptr, indices1, 4);
        val[ 1] = _mm256_i32gather_epi32(ptr, indices2, 4);
    }

    inline
    void scatter(int *ptr, const int *offsets) const
    {
        ptr[offsets[ 0]] = _mm256_extract_epi32(val[ 0], 0);
        ptr[offsets[ 1]] = _mm256_extract_epi32(val[ 0], 1);
        ptr[offsets[ 2]] = _mm256_extract_epi32(val[ 0], 2);
        ptr[offsets[ 3]] = _mm256_extract_epi32(val[ 0], 3);
        ptr[offsets[ 4]] = _mm256_extract_epi32(val[ 0], 4);
        ptr[offsets[ 5]] = _mm256_extract_epi32(val[ 0], 5);
        ptr[offsets[ 6]] = _mm256_extract_epi32(val[ 0], 6);
        ptr[offsets[ 7]] = _mm256_extract_epi32(val[ 0], 7);
        ptr[offsets[ 8]] = _mm256_extract_epi32(val[ 1], 0);
        ptr[offsets[ 9]] = _mm256_extract_epi32(val[ 1], 1);
        ptr[offsets[10]] = _mm256_extract_epi32(val[ 1], 2);
        ptr[offsets[11]] = _mm256_extract_epi32(val[ 1], 3);
        ptr[offsets[12]] = _mm256_extract_epi32(val[ 1], 4);
        ptr[offsets[13]] = _mm256_extract_epi32(val[ 1], 5);
        ptr[offsets[14]] = _mm256_extract_epi32(val[ 1], 6);
        ptr[offsets[15]] = _mm256_extract_epi32(val[ 1], 7);
    }

private:
    __m256i val[2];
};

inline
void operator<<(int *data, const short_vec<int, 16>& vec)
{
    vec.store(data);
}

template<>
class sqrt_reference<int, 16>
{
public:
    template<typename OTHER_CARGO, std::size_t OTHER_ARITY>
    friend class short_vec;

    sqrt_reference(const short_vec<int, 16>& vec) :
        vec(vec)
    {}

private:
    short_vec<int, 16> vec;
};

#ifdef __ICC
#pragma warning pop
#endif

inline
short_vec<int, 16>::short_vec(const sqrt_reference<int, 16>& other) :
    val{_mm256_cvtps_epi32(
            _mm256_sqrt_ps(_mm256_cvtepi32_ps(other.vec.val[ 0]))),
        _mm256_cvtps_epi32(
            _mm256_sqrt_ps(_mm256_cvtepi32_ps(other.vec.val[ 1])))}
{}

inline
void short_vec<int, 16>::operator/=(const sqrt_reference<int, 16>& other)
{
    val[ 0] = _mm256_cvtps_epi32(
        _mm256_mul_ps(_mm256_cvtepi32_ps(val[ 0]),
                   _mm256_rsqrt_ps(_mm256_cvtepi32_ps(other.vec.val[ 0]))));
    val[ 1] = _mm256_cvtps_epi32(
        _mm256_mul_ps(_mm256_cvtepi32_ps(val[ 1]),
                   _mm256_rsqrt_ps(_mm256_cvtepi32_ps(other.vec.val[ 1]))));
}

inline
short_vec<int, 16> short_vec<int, 16>::operator/(const sqrt_reference<int, 16>& other) const
{
    return short_vec<int, 16>(
        _mm256_cvtps_epi32(
            _mm256_mul_ps(_mm256_cvtepi32_ps(val[ 0]),
                          _mm256_rsqrt_ps(_mm256_cvtepi32_ps(other.vec.val[ 0])))),
        _mm256_cvtps_epi32(
            _mm256_mul_ps(_mm256_cvtepi32_ps(val[ 1]),
                          _mm256_rsqrt_ps(_mm256_cvtepi32_ps(other.vec.val[ 1])))));
}

inline
sqrt_reference<int, 16> sqrt(const short_vec<int, 16>& vec)
{
    return sqrt_reference<int, 16>(vec);
}

template<typename _CharT, typename _Traits>
std::basic_ostream<_CharT, _Traits>&
operator<<(std::basic_ostream<_CharT, _Traits>& __os,
           const short_vec<int, 16>& vec)
{
    const int *data1 = reinterpret_cast<const int *>(&vec.val[ 0]);
    const int *data2 = reinterpret_cast<const int *>(&vec.val[ 1]);
    __os << "["
         << data1[0] << ", " << data1[1]  << ", " << data1[2]  << ", " << data1[3] << ", "
         << data1[4] << ", " << data1[5]  << ", " << data1[6]  << ", " << data1[7] << ", "
         << data2[0] << ", " << data2[1]  << ", " << data2[2]  << ", " << data2[3] << ", "
         << data2[4] << ", " << data2[5]  << ", " << data2[6]  << ", " << data2[7]
         << "]";
    return __os;
}

}

#endif

#endif
