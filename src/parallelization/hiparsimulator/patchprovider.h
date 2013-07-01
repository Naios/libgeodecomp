#ifndef LIBGEODECOMP_PARALLELIZATION_HIPARSIMULATOR_PATCHPROVIDER_H
#define LIBGEODECOMP_PARALLELIZATION_HIPARSIMULATOR_PATCHPROVIDER_H

#include <libgeodecomp/misc/region.h>
#include <libgeodecomp/misc/superset.h>
#include <libgeodecomp/misc/stringops.h>
#include <libgeodecomp/parallelization/hiparsimulator/gridvecconv.h>

namespace LibGeoDecomp {
namespace HiParSimulator {

/**
 * PatchProvider fills in grid patches into a Stepper, which is
 * important for ghostzone synchronization, but could be used for
 * steering, too.
 */
template<class GRID_TYPE>
class PatchProvider
{
public:
    typedef typename GRID_TYPE::CellType CellType;
    const static int DIM = GRID_TYPE::DIM;

    virtual ~PatchProvider() {};

    virtual void setRegion(const Region<DIM>& region)
    {
        // empty as most implementations won't need it anyway.
    }

    // fixme: add nextRequiredNanoStep() to let steppers check if PatchProviders need to modify the grid

    virtual void get(
        GRID_TYPE *destinationGrid,
        const Region<DIM>& patchableRegion,
        const std::size_t nanoStep,
        const bool remove=true) = 0;

protected:
    SuperSet<std::size_t> storedNanoSteps;

    void checkNanoStepGet(const std::size_t nanoStep) const
    {
        if (storedNanoSteps.empty()) {
            throw std::logic_error("no nano step available");
        }
        if ((storedNanoSteps.min)() != nanoStep)
            throw std::logic_error(
                std::string(
                    "requested time step doesn't match expected nano step.")
                + " expected: " + StringOps::itoa((storedNanoSteps.min)())
                + " is: " + StringOps::itoa(nanoStep));
    }
};

}
}

#endif
