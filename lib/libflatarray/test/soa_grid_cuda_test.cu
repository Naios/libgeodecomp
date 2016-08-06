/**
 * Copyright 2016 Andreas Schäfer
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <libflatarray/flat_array.hpp>
#include <map>

#include "test.hpp"

class ActiveElement
{
public:
    __host__
    __device__
    ActiveElement()
    {
        val += 100000;
    }

    __host__
    __device__
    ~ActiveElement()
    {
        val += 1000000;
    }

    inline bool operator==(ActiveElement other) const
    {
        return val == other.val;
    }

    int val;
};

class PassiveElement
{
public:
    inline bool operator==(PassiveElement other) const
    {
        return val == other.val;
    }

    int val;
};

class ConstructorDestructorTestCellActive
{
public:
    inline
    explicit ConstructorDestructorTestCellActive(double temperature=0.0, bool alive=false) :
        temperature(temperature),
        alive(alive)
    {}

    inline bool operator==(const ConstructorDestructorTestCellActive& other) const
    {
        return
            (temperature == other.temperature) &&
            (alive == other.alive) &&
            (element == other.element);
    }

    inline bool operator!=(const ConstructorDestructorTestCellActive& other) const
    {
        return !(*this == other);
    }

    double temperature;
    bool alive;
    ActiveElement element;
};

class ConstructorDestructorTestCellPassive
{
public:
    inline
    explicit ConstructorDestructorTestCellPassive(double temperature=0.0, bool alive=false) :
        temperature(temperature),
        alive(alive)
    {}

    inline bool operator==(const ConstructorDestructorTestCellPassive& other) const
    {
        return
            (temperature == other.temperature) &&
            (alive == other.alive) &&
            (element == other.element);
    }

    inline bool operator!=(const ConstructorDestructorTestCellPassive& other) const
    {
        return !(*this == other);
    }

    double temperature;
    bool alive;
    PassiveElement element;
};

class CellWithArrayMember
{
public:
    __host__
    __device__
    inline
    explicit CellWithArrayMember(int j = 0) :
        j(j)
    {
        i[0] = j + 1;
        i[1] = j + 2;
        i[2] = j + 3;

        x[0] = j + 0.4;
        x[1] = j + 0.5;
    }

    __host__
    __device__
    inline
    CellWithArrayMember(int newI[3], double newX[2], int j) :
        j(j)
    {
        i[0] = newI[0];
        i[1] = newI[1];
        i[1] = newI[2];

        x[0] = newX[0];
        x[1] = newX[1];
    }

    int i[3];
    int j;
    double x[2];
};

class CellWithActiveArrayMember
{
public:
    __host__
    __device__
    inline
    explicit CellWithActiveArrayMember(int j = 0) :
        j(j)
    {
        i[0] = j + 1;
        i[1] = j + 2;
        i[2] = j + 3;
    }

    int i[3];
    int j;
    ActiveElement elements[2];
};

class CellWithPassiveArrayMember
{
public:
    __host__
    __device__
    inline
    explicit CellWithPassiveArrayMember(int j = 0) :
        j(j)
    {
        i[0] = j + 1;
        i[1] = j + 2;
        i[2] = j + 3;
    }

    int i[3];
    int j;
    PassiveElement elements[2];
};

LIBFLATARRAY_REGISTER_SOA(ConstructorDestructorTestCellActive,
                          ((double)(temperature))
                          ((ActiveElement)(element))
                          ((bool)(alive)) )

LIBFLATARRAY_REGISTER_SOA(ConstructorDestructorTestCellPassive,
                          ((double)(temperature))
                          ((PassiveElement)(element))
                          ((bool)(alive)) )

LIBFLATARRAY_REGISTER_SOA(CellWithArrayMember,
                          ((int)(i)(3))
                          ((int)(j))
                          ((double)(x)(2)) )

LIBFLATARRAY_REGISTER_SOA(CellWithActiveArrayMember,
                          ((int)(i)(3))
                          ((int)(j))
                          ((ActiveElement)(elements)(2)) )

LIBFLATARRAY_REGISTER_SOA(CellWithPassiveArrayMember,
                          ((int)(i)(3))
                          ((int)(j))
                          ((PassiveElement)(elements)(2)) )

namespace LibFlatArray {

std::map<std::size_t, char*> allocation_cache;

/**
 * We fake allocation here to make sure our grids in the tests below
 * get the same pointers. We need this to be sure that we're working
 * on the same memory region with each.
 */
template<class T>
class fake_cuda_allocator
{
public:
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    pointer allocate(std::size_t n, const void* = 0)
    {
        if (allocation_cache[n] != 0) {
            return allocation_cache[n];
        }


        pointer ret = 0;
        cudaMalloc(&ret, n * sizeof(T));
        allocation_cache[n] = ret;
        return ret;
    }

    void deallocate(pointer p, std::size_t)
    {
        // intentionally left blank
    }

    void deallocate_all()
    {
        for (typename std::map<std::size_t, pointer>::iterator i = allocation_cache.begin(); i != allocation_cache.end(); ++i) {
            cudaFree(i->second);
            i->second = 0;
        }
    }
};

ADD_TEST(TestCUDAConstructionDestruction)
{
    char *data = 0;
    {
        // prep device memory with consecutive numbers:
        soa_grid<ConstructorDestructorTestCellPassive, fake_cuda_allocator<char>, true> device_grid(20, 10, 5);
        data = device_grid.get_data();

        soa_grid<ConstructorDestructorTestCellPassive> host_grid(20, 10, 5);
        for (int z = 0; z < 5; ++z) {
            for (int y = 0; y < 10; ++y) {
                for (int x = 0; x < 20; ++x) {
                    ConstructorDestructorTestCellPassive cell((x + 1) * (y + 1), true);
                    cell.element.val = x + y * 20 + z * 20 * 10;
                    host_grid.set(x, y, z, cell);

                    cell = host_grid.get(x, y, z);
                }
            }
        }
        cudaMemcpy(device_grid.get_data(), host_grid.get_data(), device_grid.byte_size(), cudaMemcpyHostToDevice);

    }
    {
        // ensure c-tor was run by checking increment on all elements:
        soa_grid<ConstructorDestructorTestCellActive,  fake_cuda_allocator<char>, true> device_grid(20, 10, 5);
        BOOST_TEST(data == device_grid.get_data());

        soa_grid<ConstructorDestructorTestCellPassive> host_grid(20, 10, 5);
        cudaMemcpy(host_grid.get_data(), device_grid.get_data(), device_grid.byte_size(), cudaMemcpyDeviceToHost);
        for (int z = 0; z < 5; ++z) {
            for (int y = 0; y < 10; ++y) {
                for (int x = 0; x < 20; ++x) {
                    ConstructorDestructorTestCellPassive cell = host_grid.get(x, y, z);
                    int expected = x + y * 20 + z * 20 * 10 + 100000;

                    BOOST_TEST(cell.element.val == expected);
                    BOOST_TEST(cell.temperature == 0);
                    BOOST_TEST(cell.alive == false);
                }
            }
        }
    }
    {
        // ensure d-tor was run by checking increment on all elements:
        soa_grid<ConstructorDestructorTestCellPassive> host_grid(20, 10, 5);
        cudaMemcpy(host_grid.get_data(), data, host_grid.byte_size(), cudaMemcpyDeviceToHost);
        for (int z = 0; z < 5; ++z) {
            for (int y = 0; y < 10; ++y) {
                for (int x = 0; x < 20; ++x) {
                    ConstructorDestructorTestCellPassive cell = host_grid.get(x, y, z);
                    int expected = x + y * 20 + z * 20 * 10 + 1100000;

                    BOOST_TEST(cell.element.val == expected);
                    BOOST_TEST(cell.temperature == 0);
                    BOOST_TEST(cell.alive == false);
                }
            }
        }
    }

    fake_cuda_allocator<char>().deallocate_all();
}

ADD_TEST(TestCUDAGetSetSingleElements)
{
    soa_grid<ConstructorDestructorTestCellPassive, cuda_allocator<char>, true> device_grid(40, 13, 8);

    for (int z = 0; z < 8; ++z) {
        for (int y = 0; y < 13; ++y) {
            for (int x = 0; x < 40; ++x) {
                ConstructorDestructorTestCellPassive cell((x + 2) * (y + 2), true);
                cell.element.val = 10000 + x + y * 40 + z * 40 * 13;
                device_grid.set(x, y, z, cell);
            }
        }
    }

    for (int z = 0; z < 8; ++z) {
        for (int y = 0; y < 13; ++y) {
            for (int x = 0; x < 40; ++x) {
                ConstructorDestructorTestCellPassive cell = device_grid.get(x, y, z);

                int expected = 10000 + x + y * 40 + z * 40 * 13;
                BOOST_TEST(cell.element.val == expected);
                BOOST_TEST(cell.temperature == ((x + 2) * (y + 2)));
                BOOST_TEST(cell.alive       == true);
            }
        }
    }
}

ADD_TEST(TestCUDAGetSetMultipleElements)
{
    soa_grid<ConstructorDestructorTestCellPassive, cuda_allocator<char>, true> device_grid(35, 25, 15);

    for (int z = 0; z < 15; ++z) {
        for (int y = 0; y < 25; ++y) {
            std::vector<ConstructorDestructorTestCellPassive> cells(35);
            for (int x = 0; x < 35; ++x) {
                cells[x].alive = x % 2;
                cells[x].temperature = x * y * z;
                cells[x].element.val = 20000 + x + y * 35 + z * 35 * 25;
            }

            device_grid.set(0, y, z, cells.data(), 35);
        }
    }

    for (int z = 0; z < 15; ++z) {
        for (int y = 0; y < 25; ++y) {
            std::vector<ConstructorDestructorTestCellPassive> cells(35);
            device_grid.get(0, y, z, cells.data(), 35);

            for (int x = 0; x < 35; ++x) {
                int expected = 20000 + x + y * 35 + z * 35 * 25;

                BOOST_TEST(cells[x].element.val == expected);
                BOOST_TEST(cells[x].alive == (x % 2));
                BOOST_TEST(cells[x].temperature == (x * y * z));
            }
        }
    }
}

ADD_TEST(TestCUDALoadSaveElements)
{
    soa_grid<ConstructorDestructorTestCellPassive> host_grid(21, 10, 9);
    for (int z = 0; z < 9; ++z) {
        for (int y = 0; y < 10; ++y) {
            for (int x = 0; x < 21; ++x) {
                ConstructorDestructorTestCellPassive cell;
                cell.alive = ((x % 3) == 0);
                cell.temperature = x * y * z * -1;
                cell.element.val = 30000 + x + y * 21 + z * 21 * 10;
                host_grid.set(x, y, z, cell);
            }
        }
    }

    std::vector<char> buffer(10 * aggregated_member_size<ConstructorDestructorTestCellPassive>::VALUE);
    host_grid.save(11, 9, 8, buffer.data(), 10);

    soa_grid<ConstructorDestructorTestCellPassive, cuda_allocator<char>, true> device_grid(31, 20, 19);
    device_grid.load(21, 19, 18, buffer.data(), 10);

    for (int i = 0; i < 20; ++i) {
        ConstructorDestructorTestCellPassive cell;
        cell.alive = i % 4;
        cell.temperature = 4711 + i;
        cell.element.val = 100 * i;
        device_grid.set(i + 1, 5, 6, cell);
    }

    buffer.resize(20 * aggregated_member_size<ConstructorDestructorTestCellPassive>::VALUE);
    device_grid.save(1, 5, 6, buffer.data(), 20);

    // very load:
    soa_grid<ConstructorDestructorTestCellPassive> host_grid2(31, 20, 19);
    cudaMemcpy(host_grid2.get_data(), device_grid.get_data(), device_grid.byte_size(), cudaMemcpyDeviceToHost);

    for (int i = 0; i < 10; ++i) {
        ConstructorDestructorTestCellPassive cell = host_grid2.get(21 + i, 19, 18);

        bool expectedAlive = (((i + 11) % 3) == 0);
        double expectedTemperature = (11 + i) * 9 * 8 * -1;
        int expectedVal = 30000 + (11 + i) + 9 * 21 + 8 * 21 * 10;

        BOOST_TEST(cell.alive == expectedAlive);
        BOOST_TEST(cell.temperature == expectedTemperature);
        BOOST_TEST(cell.element.val == expectedVal);
    }

    // verify save:
    double *temperature = (double*)(buffer.data() +  0 * 20);
    int *val            = (int*)   (buffer.data() +  8 * 20);
    bool *alive         = (bool*)  (buffer.data() + 12 * 20);

    for (int i = 0; i < 20; ++i) {
        bool expectedAlive = i % 4;
        double expectedTemperature = 4711 + i;
        int expectedVal = i * 100;

        BOOST_TEST(expectedAlive       == alive[i]);
        BOOST_TEST(expectedTemperature == temperature[i]);
        BOOST_TEST(expectedVal         == val[i]);
    }

    // sanity check:
    cudaDeviceSynchronize();
    cudaError_t error = cudaGetLastError();
    if (error != cudaSuccess) {
        std::cerr << "ERROR: " << cudaGetErrorString(error) << "\n";
        throw std::runtime_error("CUDA error");
    }
}

ADD_TEST(TestCUDAArrayMembersGetSet)
{
    // test set/get single elements:
    soa_grid<CellWithArrayMember, cuda_allocator<char>, true> device_grid(12, 23, 34);

    for (int z = 0; z < 34; ++z) {
        for (int y = 0; y < 23; ++y) {
            for (int x = 0; x < 12; ++x) {
                CellWithArrayMember cell;
                cell.i[0] = x;
                cell.i[1] = y;
                cell.i[2] = z;
                cell.j    = x * y * z;
                cell.x[0] = x + y + 0.1;
                cell.x[1] = y + z + 0.2;

                device_grid.set(x, y, z, cell);
            }
        }
    }

    for (int z = 0; z < 34; ++z) {
        for (int y = 0; y < 23; ++y) {
            for (int x = 0; x < 12; ++x) {
                int expectedCellI0 = x;
                int expectedCellI1 = y;
                int expectedCellI2 = z;
                int expectedCellJ  = x * y * z;
                double expectedCellX0 = x + y + 0.1;
                double expectedCellX1 = y + z + 0.2;

                CellWithArrayMember cell = device_grid.get(x, y, z);

                BOOST_TEST(expectedCellI0 == cell.i[0]);
                BOOST_TEST(expectedCellI1 == cell.i[1]);
                BOOST_TEST(expectedCellI2 == cell.i[2]);

                BOOST_TEST(expectedCellJ  == cell.j);

                BOOST_TEST(expectedCellX0 == cell.x[0]);
                BOOST_TEST(expectedCellX1 == cell.x[1]);
            }
        }
    }
}

ADD_TEST(TestCUDAArrayMembersGetSetMultiple)
{
    // test set/get single elements:
    soa_grid<CellWithArrayMember, cuda_allocator<char>, true> device_grid(40, 23, 34);

    for (int z = 0; z < 34; ++z) {
        for (int y = 0; y < 23; ++y) {
            CellWithArrayMember cells[40];
            for (int x = 0; x < 40; ++x) {
                cells[x].i[0] = x;
                cells[x].i[1] = y;
                cells[x].i[2] = z;
                cells[x].j    = x * y * z;
                cells[x].x[0] = x + y + 0.1;
                cells[x].x[1] = y + z + 0.2;
            }

            device_grid.set(0, y, z, cells, 40);
        }
    }

    for (int z = 0; z < 34; ++z) {
        for (int y = 0; y < 23; ++y) {
            CellWithArrayMember cells[40];
            device_grid.get(0, y, z, cells, 40);

            for (int x = 0; x < 40; ++x) {
                int expectedCellI0 = x;
                int expectedCellI1 = y;
                int expectedCellI2 = z;
                int expectedCellJ  = x * y * z;
                double expectedCellX0 = x + y + 0.1;
                double expectedCellX1 = y + z + 0.2;

                BOOST_TEST(expectedCellI0 == cells[x].i[0]);
                BOOST_TEST(expectedCellI1 == cells[x].i[1]);
                BOOST_TEST(expectedCellI2 == cells[x].i[2]);

                BOOST_TEST(expectedCellJ  == cells[x].j);

                BOOST_TEST(expectedCellX0 == cells[x].x[0]);
                BOOST_TEST(expectedCellX1 == cells[x].x[1]);
            }
        }
    }
}

ADD_TEST(TestCUDAArrayMembersConstructDestruct)
{
    char *data = 0;
    {
        // prep device memory with consecutive numbers:
        soa_grid<CellWithPassiveArrayMember, fake_cuda_allocator<char>, true> device_grid(8, 9, 13);
        data = device_grid.get_data();

        soa_grid<CellWithPassiveArrayMember> host_grid(8, 9, 13);
        for (int z = 0; z < 13; ++z) {
            for (int y = 0; y < 9; ++y) {
                for (int x = 0; x < 8; ++x) {
                    CellWithPassiveArrayMember cell((x + 1) * (y + 1));
                    cell.elements[0].val = 40000 + x + y * 8 + z * 8 * 9;
                    cell.elements[1].val = 50000 + x + y * 8 + z * 8 * 9;
                    host_grid.set(x, y, z, cell);

                    cell = host_grid.get(x, y, z);
                }
            }
        }
        cudaMemcpy(device_grid.get_data(), host_grid.get_data(), device_grid.byte_size(), cudaMemcpyHostToDevice);

    }
    {
        // ensure c-tor was run by checking increment on all elements:
        soa_grid<CellWithActiveArrayMember,  fake_cuda_allocator<char>, true> device_grid(8, 9, 13);
        BOOST_TEST(data == device_grid.get_data());

        soa_grid<CellWithPassiveArrayMember> host_grid(8, 9, 13);
        cudaMemcpy(host_grid.get_data(), device_grid.get_data(), device_grid.byte_size(), cudaMemcpyDeviceToHost);
        for (int z = 0; z < 13; ++z) {
            for (int y = 0; y < 9; ++y) {
                for (int x = 0; x < 8; ++x) {
                    CellWithPassiveArrayMember cell = host_grid.get(x, y, z);
                    int expected0 = 40000 + x + y * 8 + z * 8 * 9 + 100000;
                    int expected1 = 50000 + x + y * 8 + z * 8 * 9 + 100000;

                    BOOST_TEST(cell.elements[0].val == expected0);
                    BOOST_TEST(cell.elements[1].val == expected1);

                    BOOST_TEST(cell.i[0] == 0);
                    BOOST_TEST(cell.i[1] == 0);
                    BOOST_TEST(cell.i[2] == 0);
                }
            }
        }
    }
    {
        // ensure d-tor was run by checking increment on all elements:
        soa_grid<CellWithPassiveArrayMember> host_grid(8, 9, 13);
        cudaMemcpy(host_grid.get_data(), data, host_grid.byte_size(), cudaMemcpyDeviceToHost);
        for (int z = 0; z < 13; ++z) {
            for (int y = 0; y < 9; ++y) {
                for (int x = 0; x < 8; ++x) {
                    CellWithPassiveArrayMember cell = host_grid.get(x, y, z);
                    int expected0 = 40000 + x + y * 8 + z * 8 * 9 + 1100000;
                    int expected1 = 50000 + x + y * 8 + z * 8 * 9 + 1100000;

                    BOOST_TEST(cell.elements[0].val == expected0);
                    BOOST_TEST(cell.elements[1].val == expected1);

                    BOOST_TEST(cell.i[0] == 0);
                    BOOST_TEST(cell.i[1] == 0);
                    BOOST_TEST(cell.i[2] == 0);
                }
            }
        }
    }

    fake_cuda_allocator<char>().deallocate_all();
}

ADD_TEST(TestCUDAArrayMembersLoadSave)
{
    soa_grid<CellWithPassiveArrayMember, cuda_allocator<char>, true> device_grid(45, 35, 25);
    for (int z = 0; z < 25; ++z) {
        for (int y = 0; y < 35; ++y) {
            for (int x = 0; x < 45; ++x) {
                CellWithPassiveArrayMember cell;
                cell.i[0] = x;
                cell.i[1] = y;
                cell.i[2] = z;
                cell.j = x * y * z;
                cell.elements[0].val = 4711 + x * y;
                cell.elements[1].val =  666 + y * z;

                device_grid.set(x, y, z, cell);
            }
        }
    }

    std::vector<char> buffer(aggregated_member_size<CellWithPassiveArrayMember>::VALUE * 33);
    device_grid.save(12, 34, 24, buffer.data(), 33);

    soa_grid<CellWithPassiveArrayMember, cuda_allocator<char>, true> device_grid2(35, 20, 5);
    device_grid2.load(2, 19, 4, buffer.data(), 33);

    for (int x = 0; x < 33; ++x) {
        CellWithPassiveArrayMember cell = device_grid2.get(x + 2, 19, 4);

        int expectedI0 = x + 12;
        int expectedI1 = 34;
        int expectedI2 = 24;

        int expectedJ = (x + 12) * 34 * 24;

        int expectedElements0 = 4711 + (x + 12) * 34;
        int expectedElements1 =  666 + 34 * 24;

        BOOST_TEST(cell.i[0] == expectedI0);
        BOOST_TEST(cell.i[1] == expectedI1);
        BOOST_TEST(cell.i[2] == expectedI2);

        BOOST_TEST(cell.j == expectedJ);

        BOOST_TEST(cell.elements[0].val == expectedElements0);
        BOOST_TEST(cell.elements[1].val == expectedElements1);
    }
}

}

int main(int argc, char **argv)
{
    return 0;
}
