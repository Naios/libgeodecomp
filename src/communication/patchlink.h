#ifndef LIBGEODECOMP_COMMUNICATION_PATCHLINK_H
#define LIBGEODECOMP_COMMUNICATION_PATCHLINK_H

#include <libgeodecomp/config.h>
#ifdef LIBGEODECOMP_FEATURE_MPI

#include <deque>
#include <libgeodecomp/communication/mpilayer.h>
#include <libgeodecomp/storage/patchaccepter.h>
#include <libgeodecomp/storage/patchprovider.h>

namespace LibGeoDecomp {

/**
 * PatchLink encapsulates the transmission of patches to and from
 * remote processes. PatchLink::Accepter takes the patches from a
 * Stepper hands them on to MPI, while PatchLink::Provider will receive
 * the patches from the net and provide then to a Stepper.
 */
template<class GRID_TYPE>
class PatchLink
{
public:
    const static int DIM = GRID_TYPE::DIM;
    const static size_t ENDLESS = -1;

    class Link
    {
    public:
        typedef typename GRID_TYPE::CellType CellType;

        // fixme: there may be multiple PatchLinks connecting any two
        // nodes. Since MPI matches messages by node, datatype and tag
        // and the first two of these three will be identical, we need
        // to make sure that the tag differs. We could use the "level"
        // of the UpdateGroup in the hierarchy for this or some kind
        // of registry.
        inline Link(
            const Region<DIM>& region,
            const int tag,
            MPI_Comm communicator = MPI_COMM_WORLD) :
            lastNanoStep(0),
            stride(1),
            mpiLayer(communicator),
            region(region),
            buffer(region.size()),
            tag(tag)
        {}

        virtual ~Link()
        {
            wait();
        }

        virtual void charge(const std::size_t next, const std::size_t last, const long newStride)
        {
            lastNanoStep = last;
            stride = newStride;
        }

        inline void wait()
        {
            mpiLayer.wait(tag);
        }

        inline void cancel()
        {
            mpiLayer.cancelAll();
        }

    protected:
        std::size_t lastNanoStep;
        long stride;
        MPILayer mpiLayer;
        Region<DIM> region;
        std::vector<CellType> buffer;
        int tag;
    };

    class Accepter :
        public Link,
        public PatchAccepter<GRID_TYPE>
    {
    public:
        using Link::buffer;
        using Link::lastNanoStep;
        using Link::mpiLayer;
        using Link::region;
        using Link::stride;
        using Link::tag;
        using Link::wait;
        using PatchAccepter<GRID_TYPE>::checkNanoStepPut;
        using PatchAccepter<GRID_TYPE>::pushRequest;
        using PatchAccepter<GRID_TYPE>::requestedNanoSteps;

        inline Accepter(
            const Region<DIM>& region,
            const int dest,
            const int tag,
            const MPI_Datatype& cellMPIDatatype,
            MPI_Comm communicator = MPI_COMM_WORLD) :
            Link(region, tag, communicator),
            dest(dest),
            cellMPIDatatype(cellMPIDatatype)
        {}

        virtual void charge(const std::size_t next, const std::size_t last, const std::size_t newStride)
        {
            Link::charge(next, last, newStride);
            pushRequest(next);
        }

        virtual void put(
            const GRID_TYPE& grid,
            const Region<DIM>& /*validRegion*/,
            const std::size_t nanoStep)
        {
            if (!checkNanoStepPut(nanoStep)) {
                return;
            }

            wait();
            GridVecConv::gridToVector(grid, &buffer, region);
            mpiLayer.send(&buffer[0], dest, buffer.size(), tag, cellMPIDatatype);

            std::size_t nextNanoStep = (min)(requestedNanoSteps) + stride;
            if ((lastNanoStep == ENDLESS) ||
                (nextNanoStep < lastNanoStep)) {
                requestedNanoSteps << nextNanoStep;
            }

            erase_min(requestedNanoSteps);
        }

    private:
        int dest;
        MPI_Datatype cellMPIDatatype;
    };

    class Provider :
        public Link,
        public PatchProvider<GRID_TYPE>
    {
    public:
        using Link::buffer;
        using Link::lastNanoStep;
        using Link::mpiLayer;
        using Link::region;
        using Link::stride;
        using Link::tag;
        using Link::wait;
        using PatchProvider<GRID_TYPE>::checkNanoStepGet;
        using PatchProvider<GRID_TYPE>::storedNanoSteps;

        inline Provider(
            const Region<DIM>& region,
            const int& source,
            const int& tag,
            const MPI_Datatype& cellMPIDatatype,
            MPI_Comm communicator = MPI_COMM_WORLD) :
            Link(region, tag, communicator),
            source(source),
            cellMPIDatatype(cellMPIDatatype)
        {}

        virtual void charge(const std::size_t next, const std::size_t last, const std::size_t newStride)
        {
            Link::charge(next, last, newStride);
            recv(next);
        }

        virtual void get(
            GRID_TYPE *grid,
            const Region<DIM>& patchableRegion,
            const std::size_t nanoStep,
            const bool remove=true)
        {
            if (storedNanoSteps.empty() || (nanoStep < (min)(storedNanoSteps))) {
                return;
            }

            checkNanoStepGet(nanoStep);
            wait();

            GridVecConv::vectorToGrid(buffer, grid, region);

            std::size_t nextNanoStep = (min)(storedNanoSteps) + stride;
            if ((lastNanoStep == ENDLESS) ||
                (nextNanoStep < lastNanoStep)) {
                recv(nextNanoStep);
            }

            erase_min(storedNanoSteps);
        }

        void recv(const std::size_t nanoStep)
        {
            storedNanoSteps << nanoStep;
            mpiLayer.recv(&buffer[0], source, buffer.size(), tag, cellMPIDatatype);
        }

    private:
        int source;
        MPI_Datatype cellMPIDatatype;
    };

};

}

#endif
#endif