#ifndef LIBGEODECOMP_MISC_UNSTRUCTUREDTESTCELL_H
#define LIBGEODECOMP_MISC_UNSTRUCTUREDTESTCELL_H

#include <libgeodecomp/misc/apitraits.h>
#include <libgeodecomp/misc/testcell.h>
#include <libgeodecomp/storage/fixedarray.h>
#include <libgeodecomp/storage/unstructuredlooppeeler.h>
#include <libflatarray/macros.hpp>

namespace LibGeoDecomp {

namespace UnstructuredTestCellHelpers {

/**
 * We'll use UnstructuredTestCell with different API specs. This empty
 * one doesn't add anything to the default.
 */
class EmptyAPI
{};

/**
 * Struct of Arrays is another important dimension of the test range.
 * We'll check different values for C and Sigma.
 */
class SoAAPI1 :
        public LibFlatArray::api_traits::has_default_1d_sizes,
        public APITraits::HasSoA,
        public APITraits::HasUpdateLineX,
        public APITraits::HasSellC<32>,
        public APITraits::HasSellSigma<1>
{};

/**
 * C=8, Sigma=1
 */
class SoAAPI2 :
        public LibFlatArray::api_traits::has_default_1d_sizes,
        public APITraits::HasSoA,
        public APITraits::HasUpdateLineX,
        public APITraits::HasSellC<8>,
        public APITraits::HasSellSigma<1>
{};

/**
 * C=8, Sigma=64
 */
class SoAAPI3 :
        public LibFlatArray::api_traits::has_default_1d_sizes,
        public APITraits::HasSoA,
        public APITraits::HasUpdateLineX,
        public APITraits::HasSellC<8>,
        public APITraits::HasSellSigma<64>
{};

/**
 * C=16, Sigma=32
 */
class SoAAPI4 :
        public LibFlatArray::api_traits::has_default_1d_sizes,
        public APITraits::HasSoA,
        public APITraits::HasUpdateLineX,
        public APITraits::HasSellC<16>,
        public APITraits::HasSellSigma<32>
{};

/**
 * Helper class used in updateLineX() to reuse neighbor verification
 * functions.
 */
class IterAdapter
{
public:
    inline
    explicit
    IterAdapter(std::map<int, double>::iterator iter) :
        iter(iter)
    {}

    inline
    int first() const
    {
        return iter->first;
    }

    inline
    double second() const
    {
        return iter->second;
    }

    inline
    bool operator==(const IterAdapter& other)
    {
        return iter == other.iter;
    }

    inline
    bool operator!=(const IterAdapter& other)
    {
        return iter != other.iter;
    }

    inline
    void operator++()
    {
        ++iter;
    }

private:
    std::map<int, double>::iterator iter;
};

}

/**
 * This class is the counterpart to TestCell for structured grids. We
 * use this class to verify that our Simulators correcly invoke
 * updates on unstructed grids and that ghost zone synchronization is
 * working as expected.
 */
template<
    typename ADDITIONAL_API = UnstructuredTestCellHelpers::EmptyAPI,
    typename OUTPUT = TestCellHelpers::StdOutput>
class UnstructuredTestCell
{
public:
    friend class Typemaps;
    friend class HPXSerialization;

    class API :
        public ADDITIONAL_API,
        public APITraits::HasUnstructuredTopology,
        public APITraits::HasNanoSteps<13>
#ifdef LIBGEODECOMP_WITH_MPI
        , public APITraits::HasAutoGeneratedMPIDataType<Typemaps>
#endif
    {
    public:
        LIBFLATARRAY_CUSTOM_SIZES((16)(32)(64)(128)(256)(512)(1024), (1), (1))
    };

    const static unsigned NANO_STEPS = APITraits::SelectNanoSteps<UnstructuredTestCell>::VALUE;

    inline explicit
    UnstructuredTestCell(int id = -1, unsigned cycleCounter = 0, bool isValid = false, bool isEdgeCell = false) :
        id(id),
        cycleCounter(cycleCounter),
        isValid(isValid),
        isEdgeCell(isEdgeCell)
    {}

    bool operator==(const UnstructuredTestCell& other) const
    {
        return
            (id                      == other.id) &&
            (cycleCounter            == other.cycleCounter) &&
            (isValid                 == other.isValid) &&
            (isEdgeCell              == other.isEdgeCell) &&
            (expectedNeighborWeights == other.expectedNeighborWeights);
    }

    bool operator!=(const UnstructuredTestCell& other) const
    {
        return !(*this == other);
    }

    bool valid() const
    {
        return isValid;
    }

    bool edgeCell() const
    {
        return isEdgeCell;
    }

    template<typename HOOD>
    void update(const HOOD& hood, int nanoStep)
    {
        *this = hood[hood.index()];
        verify(hood.begin(), hood.end(), hood, nanoStep);
    }

    template<typename HOOD_OLD, typename HOOD_NEW>
    static void updateLineX(HOOD_NEW& hoodNew, int indexEnd, HOOD_OLD& hoodOld, int nanoStep)
    {
        // This update function traverses the chunks in a scalar
        // fashion. That's not advisable from a performance point of
        // view, but simplifies testing for correctness:
        for (; hoodNew.index() < indexEnd; ++hoodNew, hoodOld.incIntraChunkOffset()) {
            // assemble weight map:
            std::map<int, double> weights;
            for (typename HOOD_OLD::ScalarIterator i = hoodOld.beginScalar(); i != hoodOld.endScalar(); ++i) {
                const int column = *i.first();
                const double weight = *i.second();

                // ignore 0-padding
                if ((column != 0) || (weight != 0.0)) {
                    weights[column] = weight;
                }
            }

            UnstructuredTestCell cell;
            hoodOld[hoodNew.index()] >> cell;

            cell.verify(
                UnstructuredTestCellHelpers::IterAdapter(weights.begin()),
                UnstructuredTestCellHelpers::IterAdapter(weights.end()),
                hoodOld,
                nanoStep);

            // copy back to new grid:
            hoodNew << cell;
        }
    }

    std::string toString() const
    {
        std::ostringstream ret;
        ret << "UnstructuredTestCell\n"
            << "  id: " << id << "\n"
            << "  cycleCounter: " << cycleCounter << "\n"
            << "  isEdgeCell: " << (isEdgeCell ? "true" : "false") << "\n"
            << "  expectedNeighborWeights: " << expectedNeighborWeights << "\n"
            << "  isValid: " << (isValid ? "true" : "false") << "\n";
        return ret.str();
    }

    int id;
    unsigned cycleCounter;
    bool isValid;
    bool isEdgeCell;
    FixedArray<double, 100> expectedNeighborWeights;

private:
    template<typename CELL>
    void checkNeighbor(const int otherID, const CELL& otherCell)
    {
        if (otherCell.id != otherID) {
            OUTPUT() << "UnstructuredTestCell error: other cell has ID " << otherCell.id
                     << ", but expected ID " << otherID << "\n";
            isValid = false;
        }

        if (otherCell.cycleCounter != cycleCounter) {
            OUTPUT() << "UnstructuredTestCell error: other cell on ID " << otherCell.id
                     << " is in cycle " << otherCell.cycleCounter
                     << ", but expected " << cycleCounter << "\n";
            isValid = false;
        }

        if (!otherCell.isValid) {
            OUTPUT() << "UnstructuredTestCell error: other cell on ID " << otherCell.id
                     << " is invalid\n";
            isValid = false;
        }
    }

    template<typename ITERATOR1, typename ITERATOR2, typename HOOD>
    void verify(const ITERATOR1& begin, const ITERATOR2& end, const HOOD& hood, int nanoStep)
    {
        FixedArray<int, 100> actualNeighborIDs;
        FixedArray<double, 100> actualNeighborWeights;

        for (ITERATOR1 i = begin; i != end; ++i) {
            int expectedID = i.second();
            UnstructuredTestCell neighbor = hood[i.first()];
            checkNeighbor(expectedID, neighbor);
            actualNeighborIDs << i.first();
            actualNeighborWeights << i.second();
        }
        // SELL-C-Sigma may reorder weights, but our reference is sorted:
        std::sort(actualNeighborWeights.begin(), actualNeighborWeights.end());

        if (expectedNeighborWeights != actualNeighborWeights) {
            OUTPUT() << "UnstructuredTestCell error: id " << id
                     << " is not valid on cycle " << cycleCounter
                     << ", nanoStep: " << nanoStep << "\n"
                     << "  actual IDs:   " << actualNeighborIDs << "\n"
                     << "  expected weights: " << expectedNeighborWeights << "\n"
                     << "  actual weights:   " << actualNeighborWeights << "\n";
            isValid = false;
        }

        int expectedNanoStep = cycleCounter % APITraits::SelectNanoSteps<UnstructuredTestCell>::VALUE;
        if (expectedNanoStep != nanoStep) {
            OUTPUT() << "UnstructuredTestCell error: id " << id
                     << " saw bad nano step " << nanoStep
                     << " (expected: " << expectedNanoStep << ")\n";
            isValid = false;
        }

        if (!isValid) {
            OUTPUT() << "UnstructuredTestCell error: id " << id << " is invalid\n";
        }

        ++cycleCounter;
    }
};

typedef FixedArray<int, 100> IDsVec;
typedef FixedArray<double, 100> WeightsVec;

typedef UnstructuredTestCell<UnstructuredTestCellHelpers::SoAAPI1> UnstructuredTestCellSoA1;
typedef UnstructuredTestCell<UnstructuredTestCellHelpers::SoAAPI2> UnstructuredTestCellSoA2;
typedef UnstructuredTestCell<UnstructuredTestCellHelpers::SoAAPI3> UnstructuredTestCellSoA3;
typedef UnstructuredTestCell<UnstructuredTestCellHelpers::SoAAPI4> UnstructuredTestCellSoA4;

/**
 * Helper class that enforces an MPI datatype to be created for
 * UnstructuredTestCell with the template parameters given here.
 */
class UnstructuredTestCellMPIDatatypeHelper
{
    friend class Typemaps;
    UnstructuredTestCell<UnstructuredTestCellHelpers::EmptyAPI> a;
};

template<
    typename CharT,
    typename Traits,
    typename AdditionalAPI,
    typename Output>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os,
           const UnstructuredTestCell<AdditionalAPI, Output>& cell)
{
    os << cell.toString();
    return os;
}

}

LIBFLATARRAY_REGISTER_SOA(
    LibGeoDecomp::UnstructuredTestCellSoA1,
    ((int)(id))
    ((unsigned)(cycleCounter))
    ((bool)(isValid))
    ((bool)(isEdgeCell))
    ((LibGeoDecomp::WeightsVec)(expectedNeighborWeights)) )

LIBFLATARRAY_REGISTER_SOA(
    LibGeoDecomp::UnstructuredTestCellSoA2,
    ((int)(id))
    ((unsigned)(cycleCounter))
    ((bool)(isValid))
    ((bool)(isEdgeCell))
    ((LibGeoDecomp::WeightsVec)(expectedNeighborWeights)) )

LIBFLATARRAY_REGISTER_SOA(
    LibGeoDecomp::UnstructuredTestCellSoA3,
    ((int)(id))
    ((unsigned)(cycleCounter))
    ((bool)(isValid))
    ((bool)(isEdgeCell))
    ((LibGeoDecomp::WeightsVec)(expectedNeighborWeights)) )

LIBFLATARRAY_REGISTER_SOA(
    LibGeoDecomp::UnstructuredTestCellSoA4,
    ((int)(id))
    ((unsigned)(cycleCounter))
    ((bool)(isValid))
    ((bool)(isEdgeCell))
    ((LibGeoDecomp::WeightsVec)(expectedNeighborWeights)) )

#endif
