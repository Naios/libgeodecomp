#include <cxxtest/TestSuite.h>
#include <libgeodecomp/geometry/stencils.h>
#include <libgeodecomp/misc/testcell.h>
#include <libgeodecomp/storage/soagrid.h>

using namespace LibGeoDecomp;

class SoATestCell
{
public:
    class API :
        public APITraits::HasStencil<Stencils::Moore<3, 2> >
    {};

    SoATestCell(int v = 0) :
        v(v)
    {}

    bool operator==(const SoATestCell& other)
    {
        return v == other.v;
    }

    int v;
};

template<typename _CharT, typename _Traits>
std::basic_ostream<_CharT, _Traits>&
operator<<(std::basic_ostream<_CharT, _Traits>& __os,
           const SoATestCell& cell)
{
    __os << "SoATestCell(" << cell.v << ")";
    return __os;
}

LIBFLATARRAY_REGISTER_SOA(SoATestCell, ((int)(v)))

namespace LibGeoDecomp {

class CheckCellValues
{
public:
    CheckCellValues(int startOffset, int endOffset, int expected) :
        startOffset(startOffset),
        endOffset(endOffset),
        expected(expected)
    {}

    template<typename ACCESSOR>
    void operator()(ACCESSOR accessor, int *index)
    {
        *index += startOffset;

        for (int offset = startOffset; offset < endOffset; ++offset) {
            TS_ASSERT_EQUALS(expected, accessor.v());
            ++*index;
        }
    }

private:
    int startOffset;
    int endOffset;
    int expected;
};

class SoAGridTest : public CxxTest::TestSuite
{
public:
    typedef TestCellSoA TestCellType2;
    typedef APITraits::SelectTopology<TestCellType2>::Value Topology2;

    void testBasic()
    {
        CoordBox<3> box(Coord<3>(10, 15, 22), Coord<3>(50, 40, 35));
        SoATestCell defaultCell(1);
        SoATestCell edgeCell(2);

        SoAGrid<SoATestCell, Topologies::Cube<3>::Topology> grid(box, defaultCell, edgeCell);
        grid.set(Coord<3>(1, 1, 1) + box.origin, 3);
        grid.set(Coord<3>(2, 2, 3) + box.origin, 4);

        TS_ASSERT_EQUALS(grid.actualDimensions, Coord<3>(54, 44, 39));
        TS_ASSERT_EQUALS(grid.boundingBox(), box);
        TS_ASSERT_EQUALS(grid.get(Coord<3>(0, 0, 0)), edgeCell);
        TS_ASSERT_EQUALS(grid.get(Coord<3>(0, 0, 0) + box.origin), defaultCell);
        TS_ASSERT_EQUALS(grid.get(Coord<3>(1, 1, 1) + box.origin), 3);
        TS_ASSERT_EQUALS(grid.get(Coord<3>(2, 2, 3) + box.origin), 4);

        edgeCell = -1;
        grid.setEdge(edgeCell);
        TS_ASSERT_EQUALS(grid.get(Coord<3>(0, 0, 0)), edgeCell);
        TS_ASSERT_EQUALS(grid.get(Coord<3>(0, 0, 0) + box.origin), defaultCell);
        TS_ASSERT_EQUALS(grid.get(Coord<3>(1, 1, 1) + box.origin), 3);
        TS_ASSERT_EQUALS(grid.get(Coord<3>(2, 2, 3) + box.origin), 4);
    }

    void test2d()
    {
        CoordBox<2> box(Coord<2>(10, 15), Coord<2>(50, 40));
        SoATestCell defaultCell(1);
        SoATestCell edgeCell(2);

        SoAGrid<SoATestCell, Topologies::Cube<2>::Topology> grid(box, defaultCell, edgeCell);

        grid.set(Coord<2>(1, 1) + box.origin, 3);
        grid.set(Coord<2>(2, 2) + box.origin, 4);

        TS_ASSERT_EQUALS(grid.actualDimensions, Coord<3>(54, 44, 1));
        TS_ASSERT_EQUALS(grid.boundingBox(), box);
        TS_ASSERT_EQUALS(grid.get(Coord<2>(0, 0)), edgeCell);
        TS_ASSERT_EQUALS(grid.get(Coord<2>(0, 0) + box.origin).v, defaultCell.v);
        TS_ASSERT_EQUALS(grid.get(Coord<2>(1, 1) + box.origin), 3);
        TS_ASSERT_EQUALS(grid.get(Coord<2>(2, 2) + box.origin), 4);
        TS_ASSERT_EQUALS(grid.get(Coord<2>(3, 3) + box.origin), 1);

        edgeCell = -1;
        grid.setEdge(edgeCell);
        TS_ASSERT_EQUALS(grid.get(Coord<2>(0, 0)), edgeCell);
        TS_ASSERT_EQUALS(grid.get(Coord<2>(0, 0) + box.origin), defaultCell);
        TS_ASSERT_EQUALS(grid.get(Coord<2>(1, 1) + box.origin), 3);
        TS_ASSERT_EQUALS(grid.get(Coord<2>(2, 2) + box.origin), 4);
    }

    void testGetSetManyCells()
    {
        Coord<2> origin(20, 15);
        Coord<2> dim(30, 10);
        Coord<2> end = origin + dim;
        SoAGrid<SoATestCell> testGrid(CoordBox<2>(origin, dim));

        int num = 200;
        for (int y = origin.y(); y < end.y(); y++) {
            for (int x = origin.x(); x < end.x(); x++) {
                testGrid.set(Coord<2>(x, y), SoATestCell(num * 10000 + x * 100 + y));
            }
        }

	SoATestCell cells[5];
	testGrid.get(Streak<2>(Coord<2>(21, 18), 26), cells);

	for (int i = 0; i < 5; ++i) {
	    TS_ASSERT_EQUALS(cells[i], testGrid.get(Coord<2>(i + 21, 18)));
	}

	for (int i = 0; i < 5; ++i) {
            cells[i].v = i + 1234;
        }
        testGrid.set(Streak<2>(Coord<2>(21, 18), 26), cells);

	for (int i = 0; i < 5; ++i) {
	    TS_ASSERT_EQUALS(cells[i], testGrid.get(Coord<2>(i + 21, 18)));
	}
    }

    void testInitialization()
    {
        CoordBox<3> box(Coord<3>(20, 25, 32), Coord<3>(51, 21, 15));
        Coord<3> topoDim(60, 50, 50);
        SoATestCell defaultCell(1);
        SoATestCell edgeCell(2);

        // next larger dimensions for x/y from 51 and 21 are 64 and 64.
        // int oppositeSideOffset = 22 * 64 + 16 * 64 * 64;
        int oppositeSideOffset0 = (21 + 4 - 1) * 64;
        int oppositeSideOffset1 = (15 + 4 - 1) * 64 * 64;
        int oppositeSideOffset2 = (15 + 4 - 1) * 64 * 64 + (21 + 4 - 1) * 64;

        SoAGrid<SoATestCell, Topologies::Cube<3>::Topology, true> grid(box, defaultCell, edgeCell, topoDim);

        // check not only first row, but also all other 3 outermost horizontal edges:
        // (width 51 + 2 edge cell layers on each side)
        grid.callback(CheckCellValues(0, 51 + 4, 2));
        grid.callback(CheckCellValues(0 + oppositeSideOffset0, 51 + 4 + oppositeSideOffset0, 2));
        grid.callback(CheckCellValues(0 + oppositeSideOffset1, 51 + 4 + oppositeSideOffset1, 2));
        grid.callback(CheckCellValues(0 + oppositeSideOffset2, 51 + 4 + oppositeSideOffset2, 2));

        grid.setEdge(SoATestCell(4));

        grid.callback(CheckCellValues(0, 51 + 4, 4));
        grid.callback(CheckCellValues(0 + oppositeSideOffset0, 51 + 4 + oppositeSideOffset0, 4));
        grid.callback(CheckCellValues(0 + oppositeSideOffset1, 51 + 4 + oppositeSideOffset1, 4));
        grid.callback(CheckCellValues(0 + oppositeSideOffset2, 51 + 4 + oppositeSideOffset2, 4));
    }

    void testDisplacementWithTopologicalCorrectness()
    {
        CoordBox<3> box(Coord<3>(20, 25, 32), Coord<3>(50, 40, 35));
        Coord<3> topoDim(60, 50, 50);
        SoATestCell defaultCell(1);
        SoATestCell edgeCell(2);

        SoAGrid<SoATestCell, Topologies::Torus<3>::Topology, true> grid(box, defaultCell, edgeCell, topoDim);
        for (CoordBox<3>::Iterator i = box.begin(); i != box.end(); ++i) {
            TS_ASSERT_EQUALS(grid.get(*i), defaultCell);
        }

        // here we check that topological correctness correctly maps
        // coordinates in the octant close to the origin to the
        // overlap of the far end of the grid delimited by topoDim.
        CoordBox<3> originOctant(Coord<3>(), box.origin + box.dimensions - topoDim);
        for (CoordBox<3>::Iterator i = originOctant.begin(); i != originOctant.end(); ++i) {
            TS_ASSERT_EQUALS(grid.get(*i), defaultCell);
        }

        SoATestCell dummy(4711);
        grid.set(Coord<3>(1, 2, 3), dummy);
        TS_ASSERT_EQUALS(grid.get(Coord<3>( 1,  2,  3)), dummy);
        TS_ASSERT_EQUALS(grid.get(Coord<3>(61, 52, 53)), dummy);
    }


    void testSoA()
    {
        Coord<3> dim(30, 20, 10);
        SoAGrid<TestCellType2, Topology2> grid(CoordBox<3>(Coord<3>(), dim));
        Region<3> region;
        region << Streak<3>(Coord<3>(0,  0, 0), 30)
               << Streak<3>(Coord<3>(5, 11, 0), 24)
               << Streak<3>(Coord<3>(2,  5, 5), 20);
        std::vector<char> buffer(
            LibFlatArray::aggregated_member_size<TestCellType2>::VALUE *
            region.size());

        int counter = 444;
        for (int z = 0; z < dim.z(); ++z) {
            for (int y = 0; y < dim.y(); ++y) {
                for (int x = 0; x < dim.x(); ++x) {
                    Coord<3> c(x, y, z);
                    TestCellType2 cell(c, dim, 0, counter);
                    grid.set(c, cell);
                    ++counter;
                }
            }
        }

        grid.saveRegion(&buffer[0], region);

        for (int z = 0; z < dim.z(); ++z) {
            for (int y = 0; y < dim.y(); ++y) {
                for (int x = 0; x < dim.x(); ++x) {
                    Coord<3> c(x, y, z);
                    TestCellType2 cell = grid.get(c);
                    cell.testValue = 666;
                    grid.set(c, cell);
                }
            }
        }

        grid.loadRegion(&buffer[0], region);

        counter = 444;
        for (int z = 0; z < dim.z(); ++z) {
            for (int y = 0; y < dim.y(); ++y) {
                for (int x = 0; x < dim.x(); ++x) {
                    Coord<3> c(x, y, z);
                    double testValue = 666;
                    if (region.count(c)) {
                        testValue = counter;
                    }

                    TestCellType2 actual = grid.get(c);
                    TestCellType2 expected(c, dim, 0, testValue);

                    TS_ASSERT_EQUALS(expected, actual);
                    ++counter;
                }
            }
        }
    }

    void testSoAWithOffset()
    {
        Coord<3> origin(5, 7, 3);
        Coord<3> dim(30, 20, 10);
        SoAGrid<TestCellType2, Topology2> grid(CoordBox<3>(origin, dim));
        Region<3> region;
        region << Streak<3>(Coord<3>(5,  7,  3), 30)
               << Streak<3>(Coord<3>(8, 11,  3), 24)
               << Streak<3>(Coord<3>(9, 10,  5), 20)
               << Streak<3>(Coord<3>(5, 27, 13), 35);
        std::vector<char> buffer(
            LibFlatArray::aggregated_member_size<TestCellType2>::VALUE *
            region.size());

        int counter = 444;
        for (int z = 0; z < dim.z(); ++z) {
            for (int y = 0; y < dim.y(); ++y) {
                for (int x = 0; x < dim.x(); ++x) {
                    Coord<3> c(x, y, z);
                    TestCellType2 cell(c, dim, 0, counter);
                    grid.set(origin + c, cell);
                    ++counter;
                }
            }
        }

        grid.saveRegion(&buffer[0], region);

        for (int z = 0; z < dim.z(); ++z) {
            for (int y = 0; y < dim.y(); ++y) {
                for (int x = 0; x < dim.x(); ++x) {
                    Coord<3> c(x, y, z);
                    TestCellType2 cell = grid.get(origin + c);
                    cell.testValue = 666;
                    grid.set(origin + c, cell);
                }
            }
        }

        grid.loadRegion(&buffer[0], region);

        counter = 444;
        for (int z = 0; z < dim.z(); ++z) {
            for (int y = 0; y < dim.y(); ++y) {
                for (int x = 0; x < dim.x(); ++x) {
                    Coord<3> c(x, y, z);
                    double testValue = 666;
                    if (region.count(c + origin)) {
                        testValue = counter;
                    }

                    TestCellType2 actual = grid.get(origin + c);
                    TestCellType2 expected(c, dim, 0, testValue);

                    if (expected != actual) {
                        std::cout << "error at " << c << "\n"
                                  << "  expected: " << expected.toString() << "\n"
                                  << "  actual: " << actual.toString() << "\n";
                    }

                    TS_ASSERT_EQUALS(expected, actual);
                    ++counter;
                }
            }
        }
    }
};

}