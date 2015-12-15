/**
 * Copyright 2013 - 2015 Andreas Schäfer
 * Copyright 2015 Di Xiao
 * Copyright 2015 Kurt Kanzenbach
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <libflatarray/config.h>
#include <cmath>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>
#include <sstream>
#include <libflatarray/macros.hpp>
#include <libflatarray/short_vec.hpp>
#include <stdexcept>
#include <vector>
#include <cstring>

#include <typeinfo>

#include "test.hpp"

namespace LibFlatArray {

// maximum alignment needed for AVX512 is currently 64 bytes
#ifndef __ALIGNED
#define __ALIGNED __attribute__ ((aligned (64)))
#endif

template<typename CARGO, int ARITY>
void testImplementationReal()
{
    typedef short_vec<CARGO, ARITY> ShortVec;
    typedef typename ShortVec::strategy Strategy;
    std::cout << "testImplementationReal " << ARITY << " " << typeid(CARGO).name() << " strategy: " << typeid(Strategy).name() << " A\n";
    int numElements = ShortVec::ARITY * 10;

    std::vector<CARGO> vec1(numElements);
    std::vector<CARGO> vec2(numElements, 4711);

    // init vec1:
    for (int i = 0; i < numElements; ++i) {
        vec1[i] = i + 0.1;
    }

    std::cout << "testImplementationReal " << ARITY << " B\n";
    // test default c-tor:
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST(4711 == vec2[i]);
    }
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v;
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST(0 == vec2[i]);
    }

    std::cout << "testImplementationReal " << ARITY << " C\n";
    // tests vector load/store:
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        TEST_REAL((i + 0.1), vec2[i]);
    }

    std::cout << "testImplementationReal " << ARITY << " D\n";
    // tests scalar load, vector add:
    ShortVec w = vec1[0];

    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        &vec2[i] << (v + w);
    }
    for (int i = 0; i < numElements; ++i) {
        TEST_REAL((i + 0.2), vec2[i]);
    }

    std::cout << "testImplementationReal " << ARITY << " E\n";
    // tests +=
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        v += w;
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        TEST_REAL((2 * i + 0.3), vec2[i]);
    }

    std::cout << "testImplementationReal " << ARITY << " F\n";
    // test -
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        &vec2[i] << (v - w);
    }
    for (int i = 0; i < numElements; ++i) {
        TEST_REAL((-i - 0.2), vec2[i]);
    }

    std::cout << "testImplementationReal " << ARITY << " G\n";
    // test -=
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        v -= w;
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        TEST_REAL((2 * i + 0.3), vec2[i]);
    }

    std::cout << "testImplementationReal " << ARITY << " H\n";
    // test *
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        &vec2[i] << (v * w);
    }
    for (int i = 0; i < numElements; ++i) {
        double reference = ((i + 0.1) * (2 * i + 0.3));
        TEST_REAL(reference, vec2[i]);
    }

    std::cout << "testImplementationReal " << ARITY << " I\n";
    // test *=
    for (int i = 0; i < numElements; ++i) {
        vec2[i] = i + 0.2;
    }
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        v *= w;
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        TEST_REAL((i + 0.1) * (i + 0.2), vec2[i]);
    }

    std::cout << "testImplementationReal " << ARITY << " J\n";
    // test /
    for (int i = 0; i < numElements; ++i) {
        vec2[i] = i + 0.2;
    }
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        &vec2[i] << (v / w);
    }
    for (int i = 0; i < numElements; ++i) {
        // accept lower accuracy for estimated division, really low
        // accuracy accepted because of results from ARM NEON:
        TEST_REAL_ACCURACY((i + 0.1) / (i + 0.2), vec2[i], 0.0025);
    }

    std::cout << "testImplementationReal " << ARITY << " K\n";
    // test /=
    for (int i = 0; i < numElements; ++i) {
        vec2[i] = i + 0.2;
    }
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        v /= w;
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        // here, too, lower accuracy is acceptable. As with divisions,
        // ARM NEON costs us an order of magnitude here compared to X86.
        TEST_REAL_ACCURACY((i + 0.1) / (i + 0.2), vec2[i], 0.0025);
    }

    std::cout << "testImplementationReal " << ARITY << " L\n";
    // test sqrt()
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        &vec2[i] << sqrt(v);
    }
    for (int i = 0; i < numElements; ++i) {
        // lower accuracy, mainly for ARM NEON
        TEST_REAL_ACCURACY(std::sqrt(double(i + 0.1)), vec2[i], 0.0025);
    }

    std::cout << "testImplementationReal " << ARITY << " M\n";
    // test "/ sqrt()"
    for (int i = 0; i < numElements; ++i) {
        vec2[i] = i + 0.2;
    }
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        &vec2[i] << w / sqrt(v);
    }
    for (int i = 0; i < numElements; ++i) {
        // the expression "foo / sqrt(bar)" will again result in an
        // estimated result for single precision floats, so lower accuracy is acceptable:
        TEST_REAL_ACCURACY((i + 0.2) / std::sqrt(double(i + 0.1)), vec2[i], 0.0035);
    }

    std::cout << "testImplementationReal " << ARITY << " N\n";
    // test string conversion
    for (int i = 0; i < ShortVec::ARITY; ++i) {
        vec1[i] = i + 0.1;
    }
    ShortVec v(&vec1[0]);
    std::ostringstream buf1;
    buf1 << v;

    std::ostringstream buf2;
    buf2 << "[";
    for (int i = 0; i < (ShortVec::ARITY - 1); ++i) {
        buf2 << (i + 0.1) << ", ";
    }
    buf2 << (ShortVec::ARITY - 1 + 0.1) << "]";

    BOOST_TEST(buf1.str() == buf2.str());

    std::cout << "testImplementationReal " << ARITY << " O\n";
    // test gather
    {
        CARGO array[ARITY * 10];
        unsigned indices[ARITY] __ALIGNED;
        CARGO actual[ARITY];
        CARGO expected[ARITY];
        std::memset(array, '\0', sizeof(CARGO) * ARITY * 10);
        std::cout << "testImplementationReal " << ARITY << " O1\n";

        for (int i = 0; i < ARITY * 10; ++i) {
            if (i % 10 == 0) {
                array[i] = i * 0.75;
            }
        }
        std::cout << "testImplementationReal " << ARITY << " O2\n";

        for (int i = 0; i < ARITY; ++i) {
            indices[i] = i * 10;
            expected[i] = (i * 10) * 0.75;
        }

        ShortVec vec;
        std::cout << "testImplementationReal " << ARITY << " O3\n";
        vec.gather(array, indices);
        std::cout << "testImplementationReal " << ARITY << " O4\n";
        actual << vec;

        for (int i = 0; i < ARITY; ++i) {
            TEST_REAL_ACCURACY(actual[i], expected[i], 0.001);
        }
    }

    std::cout << "testImplementationReal " << ARITY << " P\n";
#ifdef LIBFLATARRAY_WITH_CPP14
    // test gather via initializer_list
    {
        CARGO actual1[ARITY];
        CARGO actual2[ARITY];
        CARGO expected[ARITY];
        for (int i = 0; i < ARITY; ++i) {
            expected[i] = (i * 10) * 0.75;
        }

        // max: 32
        ShortVec vec1 = { 0.0, 7.5, 15.0, 22.50, 30.0, 37.5, 45.0, 52.5,
                          60.0, 67.5, 75.0, 82.5, 90.0, 97.5, 105.0, 112.5,
                          120.0, 127.5, 135.0, 142.5, 150.0, 157.5, 165.0, 172.5,
                          180.0, 187.5, 195.0, 202.5, 210.0, 217.5, 225.0, 232.5 };
        ShortVec vec2;
        vec2 = { 0.0, 7.5, 15.0, 22.50, 30.0, 37.5, 45.0, 52.5,
                 60.0, 67.5, 75.0, 82.5, 90.0, 97.5, 105.0, 112.5,
                 120.0, 127.5, 135.0, 142.5, 150.0, 157.5, 165.0, 172.5,
                 180.0, 187.5, 195.0, 202.5, 210.0, 217.5, 225.0, 232.5 };
        actual1 << vec1;
        actual2 << vec2;
        for (int i = 0; i < ARITY; ++i) {
            TEST_REAL_ACCURACY(actual1[i], expected[i], 0.001);
            TEST_REAL_ACCURACY(actual2[i], expected[i], 0.001);
        }
    }
#endif

    std::cout << "testImplementationReal " << ARITY << " R\n";
    // test scatter
    {
        ShortVec vec;
        CARGO array[ARITY * 10];
        CARGO expected[ARITY * 10];
        unsigned indices[ARITY] __ALIGNED;
        std::memset(array,    '\0', sizeof(CARGO) * ARITY * 10);
        std::memset(expected, '\0', sizeof(CARGO) * ARITY * 10);
        for (int i = 0; i < ARITY * 10; ++i) {
            if (i % 10 == 0) {
                expected[i] = i * 0.75;
            }
        }
        for (int i = 0; i < ARITY; ++i) {
            indices[i] = i * 10;
        }

        vec.gather(expected, indices);
        vec.scatter(array, indices);
        for (int i = 0; i < ARITY * 10; ++i) {
            TEST_REAL_ACCURACY(array[i], expected[i], 0.001);
        }
    }

    std::cout << "testImplementationReal " << ARITY << " S\n";
    // test non temporal stores
    {
        CARGO array[ARITY] __ALIGNED;
        CARGO expected[ARITY] __ALIGNED;

        for (int i = 0; i < ARITY; ++i) {
            expected[i] = 5.0;
        }
        ShortVec v1 = 5.0;
        v1.store_nt(array);
        for (int i = 0; i < ARITY; ++i) {
            TEST_REAL_ACCURACY(array[i], expected[i], 0.001);
        }

        for (int i = 0; i < ARITY; ++i) {
            expected[i] = i + 0.1;
        }
        ShortVec v2 = expected;
        v2.store_nt(array);
        for (int i = 0; i < ARITY; ++i) {
            TEST_REAL_ACCURACY(array[i], expected[i], 0.001);
        }
    }

    std::cout << "testImplementationReal " << ARITY << " T\n";
    // test aligned stores
    {
        CARGO array[ARITY] __ALIGNED;
        CARGO expected[ARITY] __ALIGNED;

        for (int i = 0; i < ARITY; ++i) {
            expected[i] = 5.0;
        }
        ShortVec v1 = 5.0;
        v1.store_aligned(array);
        for (int i = 0; i < ARITY; ++i) {
            TEST_REAL_ACCURACY(array[i], expected[i], 0.001);
        }

        for (int i = 0; i < ARITY; ++i) {
            expected[i] = i + 0.1;
        }
        ShortVec v2 = expected;
        v2.store_aligned(array);
        for (int i = 0; i < ARITY; ++i) {
            TEST_REAL_ACCURACY(array[i], expected[i], 0.001);
        }
    }

    std::cout << "testImplementationReal " << ARITY << " U\n";
    // test aligned loads
    {
        CARGO array[ARITY] __ALIGNED;
        CARGO expected[ARITY] __ALIGNED;

        for (int i = 0; i < ARITY; ++i) {
            array[i]    = i + 0.1;
            expected[i] = 0;
        }
        ShortVec v1;
        v1.load_aligned(array);
        v1.store(expected);
        for (int i = 0; i < ARITY; ++i) {
            TEST_REAL_ACCURACY(array[i], expected[i], 0.001);
        }
    }
    std::cout << "testImplementationReal " << ARITY << " V\n";
}

template<typename CARGO, int ARITY>
void testImplementationInt()
{
    std::cout << "testImplementationInt \n";
    typedef short_vec<CARGO, ARITY> ShortVec;
    const int numElements = ShortVec::ARITY * 10;

    std::vector<CARGO> vec1(numElements);
    std::vector<CARGO> vec2(numElements, 4711);

    // init vec1:
    for (int i = 0; i < numElements; ++i) {
        vec1[i] = i;
    }

    // test default c-tor:
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST(4711 == vec2[i]);
    }
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v;
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST(0 == vec2[i]);
    }

    // tests vector load/store:
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST_EQ(i, vec2[i]);
    }

    // tests scalar load, vector add:
    ShortVec w = vec1[1];

    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        &vec2[i] << (v + w);
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST_EQ((i + 1), vec2[i]);
    }

    // tests +=
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        v += w;
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST_EQ((2 * i + 1), vec2[i]);
    }

    // test -
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        &vec2[i] << (v - w);
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST_EQ((-i - 1), vec2[i]);
    }

    // test -=
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        v -= w;
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST_EQ((2 * i + 1), vec2[i]);
    }

    // test *
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        &vec2[i] << (v * w);
    }
    for (int i = 0; i < numElements; ++i) {
        int reference = (i * (2 * i + 1));
        BOOST_TEST_EQ(reference, vec2[i]);
    }

    // test *=
    for (int i = 0; i < numElements; ++i) {
        vec2[i] = i + 2;
    }
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        v *= w;
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST_EQ(i * (i + 2), vec2[i]);
    }

    // test /
    for (int i = 0; i < numElements; ++i) {
        vec1[i] = 4 * (i + 1);
        vec2[i] = (i + 1);
    }
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        &vec2[i] << (v / w);
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST_EQ(4, vec2[i]);
    }

    // test /=
    for (int i = 0; i < numElements; ++i) {
        vec1[i] = 4 * (i + 1);
        vec2[i] = (i + 1);
    }
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        v /= w;
        &vec2[i] << v;
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST_EQ(4, vec2[i]);
    }

    // test sqrt()
    for (int i = 0; i < numElements; ++i) {
        vec1[i] = i * i;
    }
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        &vec2[i] << sqrt(v);
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST_EQ(i, vec2[i]);
    }

    // test "/ sqrt()"
    for (int i = 0; i < numElements; ++i) {
        vec1[i] = (i + 1) * (i + 1);
        vec2[i] = (i + 1) * 2;
    }
    for (int i = 0; i < (numElements - ShortVec::ARITY + 1); i += ShortVec::ARITY) {
        ShortVec v = &vec1[i];
        ShortVec w = &vec2[i];
        &vec2[i] << w / sqrt(v);
    }
    for (int i = 0; i < numElements; ++i) {
        BOOST_TEST_EQ(2, vec2[i]);
    }

    // test string conversion
    for (int i = 0; i < ShortVec::ARITY; ++i) {
        vec1[i] = i + 5;
    }
    ShortVec v(&vec1[0]);
    std::ostringstream buf1;
    buf1 << v;

    std::ostringstream buf2;
    buf2 << "[";
    for (int i = 0; i < (ShortVec::ARITY - 1); ++i) {
        buf2 << (i + 5) << ", ";
    }
    buf2 << (ShortVec::ARITY - 1 + 5) << "]";

    BOOST_TEST(buf1.str() == buf2.str());

    // test gather
    {
        CARGO array[ARITY * 10];
        unsigned indices[ARITY] __ALIGNED;
        CARGO actual[ARITY];
        CARGO expected[ARITY];
        std::memset(array, '\0', sizeof(CARGO) * ARITY * 10);

        for (int i = 0; i < ARITY * 10; ++i) {
            if (i % 10 == 0) {
                array[i] = i + 5;
            }
        }

        for (int i = 0; i < ARITY; ++i) {
            indices[i] = i * 10;
            expected[i] = (i * 10) + 5;
        }

        ShortVec vec;
        vec.gather(array, indices);
        actual << vec;

        for (int i = 0; i < ARITY; ++i) {
            BOOST_TEST_EQ(actual[i], expected[i]);
        }
    }

#ifdef LIBFLATARRAY_WITH_CPP14
    // test gather via initializer_list
    {
        CARGO actual1[ARITY];
        CARGO actual2[ARITY];
        CARGO expected[ARITY];
        for (int i = 0; i < ARITY; ++i) {
            expected[i] = (i * 10) + 5;
        }

        // max: 32
        ShortVec vec1 = { 5, 15, 25, 35, 45, 55, 65, 75,
                          85, 95, 105, 115, 125, 135, 145, 155,
                          165, 175, 185, 195, 205, 215, 225, 235,
                          245, 255, 265, 275, 285, 295, 305, 315 };
        ShortVec vec2;
        vec2 = { 5, 15, 25, 35, 45, 55, 65, 75,
                 85, 95, 105, 115, 125, 135, 145, 155,
                 165, 175, 185, 195, 205, 215, 225, 235,
                 245, 255, 265, 275, 285, 295, 305, 315 };
        actual1 << vec1;
        actual2 << vec2;
        for (int i = 0; i < ARITY; ++i) {
            BOOST_TEST_EQ(actual1[i], expected[i]);
            BOOST_TEST_EQ(actual2[i], expected[i]);
        }
    }
#endif

    // test scatter
    {
        ShortVec vec;
        CARGO array[ARITY * 10];
        CARGO expected[ARITY * 10];
        unsigned indices[ARITY] __ALIGNED;
        std::memset(array,    '\0', sizeof(CARGO) * ARITY * 10);
        std::memset(expected, '\0', sizeof(CARGO) * ARITY * 10);
        for (int i = 0; i < ARITY * 10; ++i) {
            if (i % 10 == 0) {
                expected[i] = i + 5;
            }
        }
        for (int i = 0; i < ARITY; ++i) {
            indices[i] = i * 10;
        }

        vec.gather(expected, indices);
        vec.scatter(array, indices);
        for (int i = 0; i < ARITY * 10; ++i) {
            BOOST_TEST_EQ(array[i], expected[i]);
        }
    }

    // test non temporal stores
    {
        CARGO array[ARITY] __ALIGNED;
        CARGO expected[ARITY] __ALIGNED;

        for (int i = 0; i < ARITY; ++i) {
            expected[i] = 5;
        }
        ShortVec v1 = 5;
        v1.store_nt(array);
        for (int i = 0; i < ARITY; ++i) {
            BOOST_TEST_EQ(array[i], expected[i]);
        }

        for (int i = 0; i < ARITY; ++i) {
            expected[i] = i;
        }
        ShortVec v2 = expected;
        v2.store_nt(array);
        for (int i = 0; i < ARITY; ++i) {
            BOOST_TEST_EQ(array[i], expected[i]);
        }
    }

    // test aligned stores
    {
        CARGO array[ARITY] __ALIGNED;
        CARGO expected[ARITY] __ALIGNED;

        for (int i = 0; i < ARITY; ++i) {
            expected[i] = 5;
        }
        ShortVec v1 = 5;
        v1.store_aligned(array);
        for (int i = 0; i < ARITY; ++i) {
            BOOST_TEST_EQ(array[i], expected[i]);
        }

        for (int i = 0; i < ARITY; ++i) {
            expected[i] = i;
        }
        ShortVec v2 = expected;
        v2.store_aligned(array);
        for (int i = 0; i < ARITY; ++i) {
            BOOST_TEST_EQ(array[i], expected[i]);
        }
    }

    // test aligned loads
    {
        CARGO array[ARITY] __ALIGNED;
        CARGO expected[ARITY] __ALIGNED;

        for (int i = 0; i < ARITY; ++i) {
            array[i]    = i;
            expected[i] = 0;
        }
        ShortVec v1;
        v1.load_aligned(array);
        v1.store(expected);
        for (int i = 0; i < ARITY; ++i) {
            BOOST_TEST_EQ(array[i], expected[i]);
        }
    }
}

ADD_TEST(TestBasic)
{
    std::cout << "testbasic1\n";
    testImplementationReal<double, 1>();
    testImplementationReal<double, 2>();
    testImplementationReal<double, 4>();
    testImplementationReal<double, 8>();
    testImplementationReal<double, 16>();
    testImplementationReal<double, 32>();

    std::cout << "testbasic2\n";
    testImplementationReal<float, 1>();
    testImplementationReal<float, 2>();
    testImplementationReal<float, 4>();
    testImplementationReal<float, 8>();
    testImplementationReal<float, 16>();
    testImplementationReal<float, 32>();

    std::cout << "testbasic3\n";
    testImplementationInt<int, 1>();
    testImplementationInt<int, 2>();
    testImplementationInt<int, 4>();
    testImplementationInt<int, 8>();
    testImplementationInt<int, 16>();
    testImplementationInt<int, 32>();
    std::cout << "testbasic4\n";
}

template<typename STRATEGY>
void checkForStrategy(STRATEGY, STRATEGY)
{}

ADD_TEST(TestImplementationStrategyDouble)
{
    std::cout << "testimplementationstrategydouble1\n";
#define EXPECTED_TYPE short_vec_strategy::scalar
    checkForStrategy(short_vec<double, 1>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __SSE__
#define EXPECTED_TYPE short_vec_strategy::sse
#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif
    checkForStrategy(short_vec<double, 2>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __SSE__
#ifdef __AVX__
#define EXPECTED_TYPE short_vec_strategy::avx
#else
#define EXPECTED_TYPE short_vec_strategy::sse
#endif
#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif
    checkForStrategy(short_vec<double, 4>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __MIC__
#define EXPECTED_TYPE short_vec_strategy::mic
#else
#ifdef __SSE__
#ifdef __AVX__
#ifdef __AVX512F__
#define EXPECTED_TYPE short_vec_strategy::avx512
#else
#define EXPECTED_TYPE short_vec_strategy::avx
#endif
#else
#define EXPECTED_TYPE short_vec_strategy::sse
#endif
#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif
#endif
    checkForStrategy(short_vec<double, 8>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __MIC__
#define EXPECTED_TYPE short_vec_strategy::mic
#else
#ifdef __AVX__
#ifdef __AVX512F__
#define EXPECTED_TYPE short_vec_strategy::avx512
#else
#define EXPECTED_TYPE short_vec_strategy::avx
#endif
#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif
#endif
    checkForStrategy(short_vec<double, 16>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __MIC__
#define EXPECTED_TYPE short_vec_strategy::mic
#else
#ifdef __AVX512F__
#define EXPECTED_TYPE short_vec_strategy::avx512
#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif
#endif
    checkForStrategy(short_vec<double, 32>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE
}

ADD_TEST(TestImplementationStrategyFloat)
{
#define EXPECTED_TYPE short_vec_strategy::scalar
    checkForStrategy(short_vec<float, 1>::strategy(), EXPECTED_TYPE());
    checkForStrategy(short_vec<float, 2>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __SSE__
#define EXPECTED_TYPE short_vec_strategy::sse

#elif __ARM_NEON__
#define EXPECTED_TYPE short_vec_strategy::neon

#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif
checkForStrategy(short_vec<float, 4>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __SSE__

#ifdef __AVX__
#define EXPECTED_TYPE short_vec_strategy::avx
#else
#define EXPECTED_TYPE short_vec_strategy::sse
#endif

#elif __ARM_NEON__
#define EXPECTED_TYPE short_vec_strategy::neon

#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif
    checkForStrategy(short_vec<float, 8>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __MIC__
#define EXPECTED_TYPE short_vec_strategy::mic

#else

#ifdef __SSE__

#ifdef __AVX__

#ifdef __AVX512F__
#define EXPECTED_TYPE short_vec_strategy::avx512
#else
#define EXPECTED_TYPE short_vec_strategy::avx
#endif

#else
#define EXPECTED_TYPE short_vec_strategy::sse

#endif /* __AVX512F__ */

#else

#ifdef __ARM_NEON__
#define EXPECTED_TYPE short_vec_strategy::neon
#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif /* __ARM_NEON__ */

#endif /* __AVX__ */

#endif /* __SSE__ */
    checkForStrategy(short_vec<float, 16>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __MIC__
#define EXPECTED_TYPE short_vec_strategy::mic

#else

#ifdef __AVX__

#ifdef __AVX512F__
#define EXPECTED_TYPE short_vec_strategy::avx512
#else
#define EXPECTED_TYPE short_vec_strategy::avx
#endif /* __AVX512F__ */

#else

#ifdef __ARM_NEON__
#define EXPECTED_TYPE short_vec_strategy::neon
#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif /* __ARM_NEON__ */

#endif /* __AVX__ */

#endif
    checkForStrategy(short_vec<float, 32>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE
    std::cout << "testimplementationstrategydouble2\n";
}

ADD_TEST(TestImplementationStrategyInt)
{
    std::cout << "testimplementationstrategyint1\n";
#define EXPECTED_TYPE short_vec_strategy::scalar
    checkForStrategy(short_vec<int, 1>::strategy(), EXPECTED_TYPE());
    checkForStrategy(short_vec<int, 2>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __SSE2__
#define EXPECTED_TYPE short_vec_strategy::sse
#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif
    checkForStrategy(short_vec<int, 4>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __AVX2__
#define EXPECTED_TYPE short_vec_strategy::avx
#else
#ifdef __SSE2__
#define EXPECTED_TYPE short_vec_strategy::sse
#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif
#endif
    checkForStrategy(short_vec<int, 8>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __AVX512F__
#define EXPECTED_TYPE short_vec_strategy::avx512
#else
#ifdef __AVX2__
#define EXPECTED_TYPE short_vec_strategy::avx
#else
#ifdef __SSE2__
#define EXPECTED_TYPE short_vec_strategy::sse
#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif
#endif
#endif
    checkForStrategy(short_vec<int, 16>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE

#ifdef __AVX512F__
#define EXPECTED_TYPE short_vec_strategy::avx512
#else
#ifdef __AVX2__
#define EXPECTED_TYPE short_vec_strategy::avx
#else
#define EXPECTED_TYPE short_vec_strategy::scalar
#endif
#endif
    checkForStrategy(short_vec<int, 32>::strategy(), EXPECTED_TYPE());
#undef EXPECTED_TYPE
    std::cout << "testimplementationstrategyint2\n";
}

template<typename SHORT_VEC>
void scaler(int *i, int endX, double *data, double factor)
{
    for (; *i < endX - (SHORT_VEC::ARITY - 1); *i +=SHORT_VEC::ARITY) {
        SHORT_VEC vec(data + *i);
        vec *= factor;
        (data + *i) << vec;
    }
}

ADD_TEST(TestLoopPeeler)
{
    std::cout << "testlooppeeler1\n";
    std::vector<double> foo;
    for (int i = 0; i < 123; ++i) {
        foo.push_back(1000 + i);
    }

    int x = 3;
    LIBFLATARRAY_LOOP_PEELER(double, 8, int, &x, 113, scaler, &foo[0], 2.5);

    for (int i = 0; i < 123; ++i) {
        double expected = 1000 + i;
        if ((i >= 3) && (i < 113)) {
            expected *= 2.5;
        }

        BOOST_TEST(expected == foo[i]);
    }
    std::cout << "testlooppeeler2\n";
}

}

int main(int argc, char **argv)
{
    return 0;
}
