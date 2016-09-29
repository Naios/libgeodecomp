/**
 * Copyright 2014-2016 Andreas Schäfer
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef FLAT_ARRAY_MACROS_HPP
#define FLAT_ARRAY_MACROS_HPP

#include <libflatarray/coord.hpp>
#include <libflatarray/number_of_members.hpp>
#include <libflatarray/detail/macros.hpp>
#include <libflatarray/detail/offset.hpp>

/**
 * This macro is convenient when you need to return instances of the
 * soa_accessor from your own functions.
 */
#define LIBFLATARRAY_PARAMS						\
    LIBFLATARRAY_PARAMS_FULL(X, Y, Z, DIM_X, DIM_Y, DIM_Z, INDEX)

/**
 * Use this macro to give LibFlatArray access to your class' private
 * members.
 */
#define LIBFLATARRAY_ACCESS                                             \
    template<typename CELL_TYPE,                                        \
             long MY_DIM_X, long MY_DIM_Y, long MY_DIM_Z, long INDEX>   \
    friend class LibFlatArray::soa_accessor;                            \
                                                                        \
    template<typename CELL_TYPE,                                        \
             long MY_DIM_X, long MY_DIM_Y, long MY_DIM_Z, long INDEX>   \
    friend class LibFlatArray::const_soa_accessor;                      \
                                                                        \
    template<typename CELL_TYPE,                                        \
             long MY_DIM_X, long MY_DIM_Y, long MY_DIM_Z, long INDEX>   \
    friend class LibFlatArray::soa_accessor_light;                      \
                                                                        \
    template<typename CELL_TYPE,                                        \
             long MY_DIM_X, long MY_DIM_Y, long MY_DIM_Z, long INDEX>   \
    friend class LibFlatArray::const_soa_accessor_light;                \
                                                                        \
    template<typename CELL_TYPE, long R>                                \
    friend class LibFlatArray::detail::flat_array::offset;

/**
 * This macros registers a type with LibFlatArray so that it can be
 * used with soa_grid, soa_array and friends. It will instantiate all
 * templates required for the "Struct of Arrays" (SoA) storage and
 * adds utilities so that user code can also discover properties of
 * the SoA layout.
 */
#define LIBFLATARRAY_REGISTER_SOA(CELL_TYPE, CELL_MEMBERS)              \
    namespace LibFlatArray {                                            \
                                                                        \
    BOOST_PP_SEQ_FOR_EACH(                                              \
        LIBFLATARRAY_DEFINE_FIELD_OFFSET,                               \
        CELL_TYPE,                                                      \
        CELL_MEMBERS)                                                   \
                                                                        \
    template<>                                                          \
    class number_of_members<CELL_TYPE>                                  \
    {                                                                   \
    public:                                                             \
        static const std::size_t VALUE =                                \
            BOOST_PP_SEQ_SIZE(CELL_MEMBERS);                            \
    };                                                                  \
                                                                        \
    template<>                                                          \
    class aggregated_member_size<CELL_TYPE>                             \
    {                                                                   \
    private:                                                            \
        static const std::size_t INDEX =                                \
            number_of_members<CELL_TYPE>::VALUE;                        \
                                                                        \
    public:                                                             \
        static const std::size_t VALUE =                                \
            detail::flat_array::offset<CELL_TYPE, INDEX>::OFFSET;       \
    };                                                                  \
                                                                        \
    template<long MY_DIM_X, long MY_DIM_Y, long MY_DIM_Z, long INDEX>   \
    class soa_accessor<CELL_TYPE, MY_DIM_X, MY_DIM_Y, MY_DIM_Z, INDEX>  \
    {                                                                   \
    public:                                                             \
        typedef CELL_TYPE MyCell;                                       \
                                                                        \
        static const long DIM_X = MY_DIM_X;                             \
        static const long DIM_Y = MY_DIM_Y;                             \
        static const long DIM_Z = MY_DIM_Z;                             \
        static const long DIM_PROD = DIM_X * DIM_Y * DIM_Z;             \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        static                                                          \
        long gen_index(const long x, const long y, const long z)        \
        {                                                               \
            return z * DIM_X * DIM_Y + y * DIM_X + x;                   \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        explicit soa_accessor(char *data, const long index = 0) :       \
            data(data),                                                 \
            index(index)                                                \
        {}                                                              \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        operator CELL_TYPE() const                                      \
        {                                                               \
            CELL_TYPE ret;                                              \
            *this >> ret;                                               \
            return ret;                                                 \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        void operator+=(const long offset)                              \
        {                                                               \
            index += offset;                                            \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        void operator++()                                               \
        {                                                               \
            ++index;                                                    \
        }                                                               \
                                                                        \
        template<long X, long Y, long Z>                                \
        inline                                                          \
        __host__ __device__                                             \
        soa_accessor_light<CELL_TYPE, LIBFLATARRAY_PARAMS> operator[](  \
            coord<X, Y, Z>)                                             \
        {                                                               \
            return soa_accessor_light<CELL_TYPE, LIBFLATARRAY_PARAMS>(  \
                data, index);                                           \
        }                                                               \
                                                                        \
        template<long X, long Y, long Z>                                \
        inline                                                          \
        __host__ __device__                                             \
        const_soa_accessor_light<CELL_TYPE, LIBFLATARRAY_PARAMS> operator[]( \
            coord<X, Y, Z>) const                                       \
        {                                                               \
            return const_soa_accessor_light<CELL_TYPE, LIBFLATARRAY_PARAMS>( \
                data, index);                                           \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void operator=(const CELL_TYPE& cell)                           \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_IN,                \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void operator<<(const CELL_TYPE& cell)                          \
        {                                                               \
            (*this) = cell;                                             \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void operator>>(CELL_TYPE& cell) const                          \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_OUT,               \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void load(const char *source, std::size_t count)                \
        {                                                               \
            load(source, count, 0, count);                              \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void load(                                                      \
            const char *source,                                         \
            std::size_t count,                                          \
            std::size_t offset,                                         \
            std::size_t stride)                                         \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_ARRAY_IN,          \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void save(char *target, std::size_t count) const                \
        {                                                               \
            save(target, count, 0, count);                              \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void save(                                                      \
            char *target,                                               \
            std::size_t count,                                          \
            std::size_t offset,                                         \
            std::size_t stride) const                                   \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_ARRAY_OUT,         \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void construct_members()                                        \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_INIT_SOA_GENERIC_MEMBER,                   \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void destroy_members()                                          \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_DESTROY_SOA_GENERIC_MEMBER,                \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        template<typename OTHER_ACCESSOR>                               \
        __host__ __device__                                             \
        inline                                                          \
        void copy_members(const OTHER_ACCESSOR& other, std::size_t count) \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER,                   \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        template<typename MEMBER_TYPE, long OFFSET>                     \
        inline                                                          \
        __host__ __device__                                             \
        MEMBER_TYPE& access_member()                                    \
        {                                                               \
            return *(MEMBER_TYPE*)(                                     \
                data +                                                  \
                DIM_PROD *                                              \
                detail::flat_array::offset<CELL_TYPE, OFFSET>::OFFSET + \
                index * sizeof(MEMBER_TYPE) +                           \
                INDEX * sizeof(MEMBER_TYPE));                           \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        char *access_member(const long size_of_member, const long offset) \
        {                                                               \
            return                                                      \
                data +                                                  \
                DIM_PROD * offset +                                     \
                index * size_of_member +                                \
                INDEX * size_of_member;                                 \
        }                                                               \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_DECLARE_SOA_MEMBER_NORMAL,                     \
            CELL_TYPE,                                                  \
            CELL_MEMBERS);                                              \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_DECLARE_SOA_MEMBER_CONST,                      \
            CELL_TYPE,                                                  \
            CELL_MEMBERS);                                              \
                                                                        \
        __host__ __device__                                             \
        const char *get_data() const                                    \
        {                                                               \
            return data;                                                \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        char *get_data()                                                \
        {                                                               \
            return data;                                                \
        }                                                               \
                                                                        \
    private:                                                            \
        char *data;                                                     \
    public:                                                             \
        long index;                                                     \
    };                                                                  \
                                                                        \
    template<long MY_DIM_X, long MY_DIM_Y, long MY_DIM_Z, long INDEX>   \
    class const_soa_accessor<                                           \
        CELL_TYPE, MY_DIM_X, MY_DIM_Y, MY_DIM_Z, INDEX>                 \
    {                                                                   \
    public:                                                             \
        typedef CELL_TYPE MyCell;                                       \
                                                                        \
        static const long DIM_X = MY_DIM_X;                             \
        static const long DIM_Y = MY_DIM_Y;                             \
        static const long DIM_Z = MY_DIM_Z;                             \
        static const long DIM_PROD = DIM_X * DIM_Y * DIM_Z;             \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        static                                                          \
        long gen_index(const long x, const long y, const long z)        \
        {                                                               \
            return z * DIM_X * DIM_Y + y * DIM_X + x;                   \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        const_soa_accessor(const char *data, long index) :              \
            data(data),                                                 \
            index(index)                                                \
        {}                                                              \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        operator CELL_TYPE() const                                      \
        {                                                               \
            CELL_TYPE ret;                                              \
            *this >> ret;                                               \
            return ret;                                                 \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        void operator+=(const long offset)                              \
        {                                                               \
            index += offset;                                            \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        void operator++()                                               \
        {                                                               \
            ++index;                                                    \
        }                                                               \
                                                                        \
        template<long X, long Y, long Z>                                \
        inline                                                          \
        __host__ __device__                                             \
        const_soa_accessor<CELL_TYPE, LIBFLATARRAY_PARAMS> operator[](  \
            coord<X, Y, Z>) const                                       \
        {                                                               \
            return const_soa_accessor<CELL_TYPE, LIBFLATARRAY_PARAMS>(  \
                data, index);                                           \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void operator>>(CELL_TYPE& cell) const                          \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_OUT,               \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void save(char *target, std::size_t count) const                \
        {                                                               \
            save(target, count, 0, count);                              \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void save(                                                      \
            char *target,                                               \
            std::size_t count,                                          \
            std::size_t offset,                                         \
            std::size_t stride) const                                   \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_ARRAY_OUT,         \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_DECLARE_SOA_MEMBER_CONST,                      \
            CELL_TYPE,                                                  \
            CELL_MEMBERS);                                              \
                                                                        \
        __host__ __device__                                             \
        const char *get_data() const                                    \
        {                                                               \
            return data;                                                \
        }                                                               \
                                                                        \
    private:                                                            \
        const char *data;                                               \
    public:                                                             \
        long index;                                                     \
    };                                                                  \
                                                                        \
    template<long MY_DIM_X, long MY_DIM_Y, long MY_DIM_Z, long INDEX>   \
    class soa_accessor_light<CELL_TYPE, MY_DIM_X, MY_DIM_Y, MY_DIM_Z, INDEX> \
    {                                                                   \
    public:                                                             \
        typedef CELL_TYPE MyCell;                                       \
                                                                        \
        static const long DIM_X = MY_DIM_X;                             \
        static const long DIM_Y = MY_DIM_Y;                             \
        static const long DIM_Z = MY_DIM_Z;                             \
        static const long DIM_PROD = DIM_X * DIM_Y * DIM_Z;             \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        static                                                          \
        long gen_index(const long x, const long y, const long z)        \
        {                                                               \
            return z * DIM_X * DIM_Y + y * DIM_X + x;                   \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        soa_accessor_light(char *data, long& index) :                   \
            data(data),                                                 \
            index(&index)                                               \
        {}                                                              \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        operator CELL_TYPE() const                                      \
        {                                                               \
            CELL_TYPE ret;                                              \
            *this >> ret;                                               \
            return ret;                                                 \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
            void operator+=(const long offset)                          \
        {                                                               \
            *index += offset;                                           \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        void operator++()                                               \
        {                                                               \
            ++*index;                                                   \
        }                                                               \
                                                                        \
        template<long X, long Y, long Z>                                \
        inline                                                          \
        __host__ __device__                                             \
        soa_accessor_light<CELL_TYPE, LIBFLATARRAY_PARAMS> operator[](  \
            coord<X, Y, Z>) const                                       \
        {                                                               \
            return soa_accessor_light<CELL_TYPE, LIBFLATARRAY_PARAMS>(  \
                data, *index);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void operator=(const CELL_TYPE& cell)                           \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_IN,                \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void operator<<(const CELL_TYPE& cell)                          \
        {                                                               \
            (*this) = cell;                                             \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void operator>>(CELL_TYPE& cell) const                          \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_OUT,               \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void load(const char *source, std::size_t count)                \
        {                                                               \
            load(source, count, 0, count);                              \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void load(                                                      \
            const char *source,                                         \
            std::size_t count,                                          \
            std::size_t offset,                                         \
            std::size_t stride)                                         \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_ARRAY_IN,          \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void save(char *target, std::size_t count) const                \
        {                                                               \
            save(target, count, 0, count);                              \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void save(                                                      \
            char *target,                                               \
            std::size_t count,                                          \
            std::size_t offset,                                         \
            std::size_t stride) const                                   \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_ARRAY_OUT,         \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void construct_members()                                        \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_INIT_SOA_GENERIC_MEMBER,                   \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void destroy_members()                                          \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_DESTROY_SOA_GENERIC_MEMBER,                \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        template<typename OTHER_ACCESSOR>                               \
        __host__ __device__                                             \
        inline                                                          \
        void copy_members(const OTHER_ACCESSOR& other, std::size_t count) \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER,                   \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        template<typename MEMBER_TYPE, long OFFSET>                     \
        inline                                                          \
        __host__ __device__                                             \
        MEMBER_TYPE& access_member()                                    \
        {                                                               \
            return *(MEMBER_TYPE*)(                                     \
                data +                                                  \
                DIM_PROD *                                              \
                detail::flat_array::offset<CELL_TYPE, OFFSET>::OFFSET + \
                *index * sizeof(MEMBER_TYPE) +                          \
                INDEX  * sizeof(MEMBER_TYPE));                          \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        char *access_member(const long size_of_member, const long offset) \
        {                                                               \
            return                                                      \
                data +                                                  \
                DIM_PROD * offset +                                     \
                *index * size_of_member +                               \
                INDEX  * size_of_member;                                \
        }                                                               \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_DECLARE_SOA_MEMBER_LIGHT_NORMAL,               \
            CELL_TYPE,                                                  \
            CELL_MEMBERS);                                              \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_DECLARE_SOA_MEMBER_LIGHT_CONST,                \
            CELL_TYPE,                                                  \
            CELL_MEMBERS);                                              \
                                                                        \
        __host__ __device__                                             \
        const char *get_data() const                                    \
        {                                                               \
            return data;                                                \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        char *get_data()                                                \
        {                                                               \
            return data;                                                \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        const long *get_index() const                                   \
        {                                                               \
            return index;                                               \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        long *get_index()                                               \
        {                                                               \
            return index;                                               \
        }                                                               \
                                                                        \
    private:                                                            \
        char *data;                                                     \
        long *index;                                                    \
    };                                                                  \
                                                                        \
    template<long MY_DIM_X, long MY_DIM_Y, long MY_DIM_Z, long INDEX>   \
    class const_soa_accessor_light<CELL_TYPE, MY_DIM_X, MY_DIM_Y, MY_DIM_Z, INDEX> \
    {                                                                   \
    public:                                                             \
        typedef CELL_TYPE MyCell;                                       \
                                                                        \
        static const long DIM_X = MY_DIM_X;                             \
        static const long DIM_Y = MY_DIM_Y;                             \
        static const long DIM_Z = MY_DIM_Z;                             \
        static const long DIM_PROD = DIM_X * DIM_Y * DIM_Z;             \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        static                                                          \
        long gen_index(const long x, const long y, const long z)        \
        {                                                               \
            return z * DIM_X * DIM_Y + y * DIM_X + x;                   \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        const_soa_accessor_light(const char *data, long& index) :       \
            data(data),                                                 \
            index(&index)                                               \
        {}                                                              \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        operator CELL_TYPE() const                                      \
        {                                                               \
            CELL_TYPE ret;                                              \
            *this >> ret;                                               \
            return ret;                                                 \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        void operator+=(const long offset)                              \
        {                                                               \
            *index += offset;                                           \
        }                                                               \
                                                                        \
        inline                                                          \
        __host__ __device__                                             \
        void operator++()                                               \
        {                                                               \
            ++*index;                                                   \
        }                                                               \
                                                                        \
        template<long X, long Y, long Z>                                \
        inline                                                          \
        __host__ __device__                                             \
        const_soa_accessor_light<CELL_TYPE, LIBFLATARRAY_PARAMS> operator[]( \
            coord<X, Y, Z>) const                                       \
        {                                                               \
            return const_soa_accessor_light<CELL_TYPE, LIBFLATARRAY_PARAMS>( \
                data, *index);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void operator>>(CELL_TYPE& cell) const                          \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_OUT,               \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void save(char *target, std::size_t count) const                \
        {                                                               \
            save(target, count, 0, count);                              \
        }                                                               \
                                                                        \
        __host__ __device__                                             \
        inline                                                          \
        void save(                                                      \
            char *target,                                               \
            std::size_t count,                                          \
            std::size_t offset,                                         \
            std::size_t stride) const                                   \
        {                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                      \
                LIBFLATARRAY_COPY_SOA_GENERIC_MEMBER_ARRAY_OUT,         \
                CELL_TYPE,                                              \
                CELL_MEMBERS);                                          \
        }                                                               \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_DECLARE_SOA_MEMBER_LIGHT_CONST,                \
            CELL_TYPE,                                                  \
            CELL_MEMBERS);                                              \
                                                                        \
        __host__ __device__                                             \
        const char *get_data() const                                    \
        {                                                               \
            return data;                                                \
        }                                                               \
                                                                        \
    private:                                                            \
        const char *data;                                               \
        long *index;                                                    \
    };                                                                  \
                                                                        \
    }                                                                   \
                                                                        \
    template<long DIM_X, long DIM_Y, long DIM_Z, long INDEX>            \
    __host__ __device__                                                 \
    inline                                                              \
    void operator<<(                                                    \
        CELL_TYPE& cell,                                                \
        const LibFlatArray::soa_accessor<                               \
            CELL_TYPE, DIM_X, DIM_Y, DIM_Z, INDEX> soa)                 \
    {                                                                   \
        soa >> cell;                                                    \
    }

#define LIBFLATARRAY_CUSTOM_SIZES(X_SIZES, Y_SIZES, Z_SIZES)            \
    typedef void has_sizes;                                             \
                                                                        \
    template<typename CELL, typename FUNCTOR>                           \
    void select_size(                                                   \
        char *data,                                                     \
        FUNCTOR& functor,                                               \
        const std::size_t dim_x = 1,                                    \
        const std::size_t dim_y = 1,                                    \
        const std::size_t dim_z = 1)                                    \
    {                                                                   \
        bind_size_x<CELL>(dim_x, dim_y, dim_z, data, functor);          \
    }                                                                   \
                                                                        \
    template<typename CELL, typename FUNCTOR>                           \
    void select_size(                                                   \
        const char *data,                                               \
        FUNCTOR& functor,                                               \
        const std::size_t dim_x = 1,                                    \
        const std::size_t dim_y = 1,                                    \
        const std::size_t dim_z = 1)                                    \
    {                                                                   \
        bind_size_x<CELL>(dim_x, dim_y, dim_z, data, functor);          \
    }                                                                   \
                                                                        \
    template<typename CELL, typename FUNCTOR>                           \
    void bind_size_x(                                                   \
        const std::size_t dim_x,                                        \
        const std::size_t dim_y,                                        \
        const std::size_t dim_z,                                        \
        char *data,                                                     \
        FUNCTOR& functor) const                                         \
    {                                                                   \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_X,                                    \
            unused,                                                     \
            X_SIZES);                                                   \
                                                                        \
        throw std::out_of_range("grid dimension X too large");          \
    }                                                                   \
                                                                        \
    template<typename CELL, typename FUNCTOR>                           \
    void bind_size_x(                                                   \
        const std::size_t dim_x,                                        \
        const std::size_t dim_y,                                        \
        const std::size_t dim_z,                                        \
        const char *data,                                               \
        FUNCTOR& functor) const                                         \
    {                                                                   \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_X,                                    \
            unused,                                                     \
            X_SIZES);                                                   \
                                                                        \
        throw std::out_of_range("grid dimension X too large");          \
    }                                                                   \
                                                                        \
    template<typename CELL, long DIM_X, typename FUNCTOR>               \
    void bind_size_y(                                                   \
        const std::size_t dim_y,                                        \
        const std::size_t dim_z,                                        \
        char *data,                                                     \
        FUNCTOR& functor) const                                         \
    {                                                                   \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_Y,                                    \
            unused,                                                     \
            Y_SIZES);                                                   \
                                                                        \
        throw std::out_of_range("grid dimension Y too large");          \
    }                                                                   \
                                                                        \
    template<typename CELL, long DIM_X, typename FUNCTOR>               \
    void bind_size_y(                                                   \
        const std::size_t dim_y,                                        \
        const std::size_t dim_z,                                        \
        const char *data,                                               \
        FUNCTOR& functor) const                                         \
    {                                                                   \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_Y,                                    \
            unused,                                                     \
            Y_SIZES);                                                   \
                                                                        \
        throw std::out_of_range("grid dimension Y too large");          \
    }                                                                   \
                                                                        \
    template<typename CELL, long DIM_X, long DIM_Y, typename FUNCTOR>   \
    void bind_size_z(                                                   \
        const std::size_t dim_z,                                        \
        char *data,                                                     \
        FUNCTOR& functor) const                                         \
    {                                                                   \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_Z,                                    \
            unused,                                                     \
            Z_SIZES);                                                   \
                                                                        \
        throw std::out_of_range("grid dimension Z too large");          \
    }                                                                   \
                                                                        \
    template<typename CELL, long DIM_X, long DIM_Y, typename FUNCTOR>   \
    void bind_size_z(                                                   \
        const std::size_t dim_z,                                        \
        const char *data,                                               \
        FUNCTOR& functor) const                                         \
    {                                                                   \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_Z,                                    \
            unused,                                                     \
            Z_SIZES);                                                   \
                                                                        \
        throw std::out_of_range("grid dimension Z too large");          \
    }

#define LIBFLATARRAY_CUSTOM_SIZES_1D_UNIFORM(SIZES)                     \
    typedef void has_sizes;                                             \
                                                                        \
    template<typename CELL, typename FUNCTOR>                           \
    void select_size(                                                   \
        char *data,                                                     \
        FUNCTOR& functor,                                               \
        const std::size_t dim_x = 1,                                    \
        const std::size_t dim_y = 1,                                    \
        const std::size_t dim_z = 1)                                    \
    {                                                                   \
        if (dim_y != 1) {                                               \
            throw std::out_of_range("expected 1D grid, but y != 1");    \
        }                                                               \
        if (dim_z != 1) {                                               \
            throw std::out_of_range("expected 1D grid, but z != 1");    \
        }                                                               \
        std::size_t max = dim_x;                                        \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_MAX_1D,                               \
            unused,                                                     \
            SIZES);                                                     \
                                                                        \
        throw std::out_of_range("max grid dimension too large");        \
    }                                                                   \
                                                                        \
    template<typename CELL, typename FUNCTOR>                           \
    void select_size(                                                   \
        const char *data,                                               \
        FUNCTOR& functor,                                               \
        const std::size_t dim_x = 1,                                    \
        const std::size_t dim_y = 1,                                    \
        const std::size_t dim_z = 1)                                    \
    {                                                                   \
        if (dim_z != 1) {                                               \
            throw std::out_of_range("expected 2D grid, but z != 1");    \
        }                                                               \
        std::size_t max = std::max(dim_x, dim_z);                       \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_MAX_2D,                               \
            unused,                                                     \
            SIZES);                                                     \
                                                                        \
        throw std::out_of_range("max grid dimension too large");        \
    }

#define LIBFLATARRAY_CUSTOM_SIZES_2D_UNIFORM(SIZES)                     \
    typedef void has_sizes;                                             \
                                                                        \
    template<typename CELL, typename FUNCTOR>                           \
    void select_size(                                                   \
        char *data,                                                     \
        FUNCTOR& functor,                                               \
        const std::size_t dim_x = 1,                                    \
        const std::size_t dim_y = 1,                                    \
        const std::size_t dim_z = 1)                                    \
    {                                                                   \
        if (dim_z != 1) {                                               \
            throw std::out_of_range("expected 2D grid, but z != 1");    \
        }                                                               \
        std::size_t max = std::max(dim_x, dim_z);                       \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_MAX_2D,                               \
            unused,                                                     \
            SIZES);                                                     \
                                                                        \
        throw std::out_of_range("max grid dimension too large");        \
    }                                                                   \
                                                                        \
    template<typename CELL, typename FUNCTOR>                           \
    void select_size(                                                   \
        const char *data,                                               \
        FUNCTOR& functor,                                               \
        const std::size_t dim_x = 1,                                    \
        const std::size_t dim_y = 1,                                    \
        const std::size_t dim_z = 1)                                    \
    {                                                                   \
        if (dim_z != 1) {                                               \
            throw std::out_of_range("expected 2D grid, but z != 1");    \
        }                                                               \
        std::size_t max = std::max(dim_x, dim_z);                       \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_MAX_2D,                               \
            unused,                                                     \
            SIZES);                                                     \
                                                                        \
        throw std::out_of_range("max grid dimension too large");        \
    }

#define LIBFLATARRAY_CUSTOM_SIZES_3D_UNIFORM(SIZES)                     \
    typedef void has_sizes;                                             \
                                                                        \
    template<typename CELL, typename FUNCTOR>                           \
    void select_size(                                                   \
        char *data,                                                     \
        FUNCTOR& functor,                                               \
        const std::size_t dim_x = 1,                                    \
        const std::size_t dim_y = 1,                                    \
        const std::size_t dim_z = 1)                                    \
    {                                                                   \
        std::size_t max = std::max(dim_x, std::max(dim_y, dim_z));      \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_MAX_3D,                               \
            unused,                                                     \
            SIZES);                                                     \
                                                                        \
        throw std::out_of_range("max grid dimension too large");        \
    }                                                                   \
                                                                        \
    template<typename CELL, typename FUNCTOR>                           \
    void select_size(                                                   \
        const char *data,                                               \
        FUNCTOR& functor,                                               \
        const std::size_t dim_x = 1,                                    \
        const std::size_t dim_y = 1,                                    \
        const std::size_t dim_z = 1)                                    \
    {                                                                   \
        std::size_t max = std::max(dim_x, std::max(dim_y, dim_z));      \
                                                                        \
        BOOST_PP_SEQ_FOR_EACH(                                          \
            LIBFLATARRAY_CASE_DIM_MAX_3D,                               \
            unused,                                                     \
            SIZES);                                                     \
                                                                        \
        throw std::out_of_range("max grid dimension too large");        \
    }

/**
 * CARGO: element type
 */
#define LIBFLATARRAY_LOOP_PEELER(CARGO, ARITY, COUNTER_TYPE,            \
                                 X, END_X, FUNCTION, ARGS...)           \
    {                                                                   \
        typedef LibFlatArray::short_vec<CARGO, (ARITY)> ShortVecType;   \
        typedef LibFlatArray::short_vec<CARGO, 1>     ScalarType;       \
        COUNTER_TYPE remainder = *(X) % (ARITY);                        \
        COUNTER_TYPE next_stop = remainder ?                            \
            *(X) + (ARITY) - remainder :                                \
            *(X);                                                       \
                                                                        \
        FUNCTION<ScalarType  >(X, next_stop, ARGS);                     \
        FUNCTION<ShortVecType>(X, (END_X),   ARGS);                     \
        FUNCTION<ScalarType  >(X, (END_X),   ARGS);                     \
    }

#endif
