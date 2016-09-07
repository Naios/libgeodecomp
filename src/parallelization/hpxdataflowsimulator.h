#ifndef LIBGEODECOMP_PARALLELIZATION_HPXDATAFLOWSIMULATOR_H
#define LIBGEODECOMP_PARALLELIZATION_HPXDATAFLOWSIMULATOR_H

#include <libgeodecomp/config.h>
#ifdef LIBGEODECOMP_WITH_HPX

#include <functional>
#include <libgeodecomp/geometry/partitions/unstructuredstripingpartition.h>
#include <libgeodecomp/geometry/partitionmanager.h>
#include <libgeodecomp/communication/hpxreceiver.h>
#include <libgeodecomp/parallelization/hierarchicalsimulator.h>
#include <mutex>
#include <stdexcept>

namespace LibGeoDecomp {

namespace HPXDataFlowSimulatorHelpers {

class UpdateEvent
{
public:
    inline
    UpdateEvent(const std::vector<int>& neighbors, int nanoStep, int step) :
        myNeighbors(neighbors),
        myNanoStep(nanoStep),
        myStep(step)
    {}

    inline int nanoStep() const
    {
        return myNanoStep;
    }

    inline int step() const
    {
        return myStep;
    }

private:
    const std::vector<int>& myNeighbors;
    int myNanoStep;
    int myStep;
};

template<typename MESSAGE>
class Neighborhood
{
public:
    // fixme: move semantics
    inline Neighborhood(
        int targetGlobalNanoStep,
        std::vector<int> messageNeighborIDs,
        std::vector<hpx::shared_future<MESSAGE> > messagesFromNeighbors,
        const std::map<int, hpx::id_type>& remoteIDs) :
        targetGlobalNanoStep(targetGlobalNanoStep),
        messageNeighborIDs(messageNeighborIDs),
        messagesFromNeighbors(hpx::util::unwrapped(messagesFromNeighbors)),
        remoteIDs(remoteIDs)
    {
        sentNeighbors.reserve(messageNeighborIDs.size());
    }

    inline
    const std::vector<int>& neighbors() const
    {
        return messageNeighborIDs;
    }

    inline
    const MESSAGE& operator[](int index) const
    {
        std::vector<int>::const_iterator i = std::find(messageNeighborIDs.begin(), messageNeighborIDs.end(), index);
        if (i == messageNeighborIDs.end()) {
            throw std::logic_error("ID not found for incoming messages");
        }

        return messagesFromNeighbors[i - messageNeighborIDs.begin()];
    }

    // fixme: move semantics
    inline
    void send(int remoteCellID, const MESSAGE& message)
    {
        std::map<int, hpx::id_type>::const_iterator iter = remoteIDs.find(remoteCellID);
        if (iter == remoteIDs.end()) {
            throw std::logic_error("ID not found for outgoing messages");
        }

        sentNeighbors << remoteCellID;

        hpx::apply(
            typename HPXReceiver<MESSAGE>::receiveAction(),
            iter->second,
            targetGlobalNanoStep,
            message);
    }

    inline
    void sendEmptyMessagesToUnnotifiedNeighbors()
    {
        for (int neighbor: messageNeighborIDs) {
            if (std::find(sentNeighbors.begin(), sentNeighbors.end(), neighbor) == sentNeighbors.end()) {
                send(neighbor, MESSAGE());
            }
        }
    }

private:
    int targetGlobalNanoStep;
    std::vector<int> messageNeighborIDs;
    std::vector<MESSAGE> messagesFromNeighbors;
    std::map<int, hpx::id_type> remoteIDs;
    std::vector<int> sentNeighbors;
};

// fixme: componentize
template<typename CELL, typename MESSAGE>
class CellComponent
{
public:
    static const unsigned NANO_STEPS = APITraits::SelectNanoSteps<CELL>::VALUE;
    typedef typename APITraits::SelectMessageType<CELL>::Value MessageType;
    typedef DisplacedGrid<CELL, Topologies::Unstructured::Topology> GridType;


    // fixme: move semantics
    explicit CellComponent(
        const std::string& basename = "",
        typename SharedPtr<GridType>::Type grid = 0,
        int id = -1,
        const std::vector<int> neighbors = std::vector<int>()) :
        basename(basename),
        // fixme: move semantics
        neighbors(neighbors),
        grid(grid),
        id(id)
    {
        for (auto&& neighbor: neighbors) {
            std::string linkName = endpointName(basename, neighbor, id);
            receivers[neighbor] = HPXReceiver<MESSAGE>::make(linkName).get();
        }
    }

    void setupRemoteReceiverIDs()
    {
        std::vector<hpx::future<void> > remoteIDFutures;
        remoteIDFutures.reserve(neighbors.size());
        hpx::lcos::local::spinlock mutex;

        for (auto i = neighbors.begin(); i != neighbors.end(); ++i) {
            std::string linkName = endpointName(basename, id, *i);

            int neighbor = *i;
            remoteIDFutures << HPXReceiver<MessageType>::find(linkName).then(
                [&mutex, neighbor, this](hpx::shared_future<hpx::id_type> remoteIDFuture)
                {
                    std::lock_guard<hpx::lcos::local::spinlock> l(mutex);
                    remoteIDs[neighbor] = remoteIDFuture.get();
                });
        }

        hpx::shared_future<void>(hpx::when_all(remoteIDFutures)).get(); // swallowing exceptions?
    }

    hpx::shared_future<void> setupDataflow(hpx::shared_future<void> lastTimeStepFuture, int startStep, int endStep)
    {
        if (startStep == 0) {
            setupRemoteReceiverIDs();
        }

        // fixme: add steerer/writer interaction
        for (int step = startStep; step < endStep; ++step) {
            for (std::size_t nanoStep = 0; nanoStep < NANO_STEPS; ++nanoStep) {
                int globalNanoStep = step * NANO_STEPS + nanoStep;

                std::vector<hpx::shared_future<MessageType> > receiveMessagesFutures;
                receiveMessagesFutures.reserve(neighbors.size());

                for (auto&& neighbor: neighbors) {
                    if ((globalNanoStep) > 0) {
                        receiveMessagesFutures << receivers[neighbor]->get(globalNanoStep);
                    } else {
                        receiveMessagesFutures <<  hpx::make_ready_future(MessageType());
                    }
                }

                auto Operation = std::bind(
                    &HPXDataFlowSimulatorHelpers::CellComponent<CELL, MessageType>::update,
                    *this,
                    // explicit namespace to avoid clashes with boost::bind's placeholders:
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    std::placeholders::_4,
                    std::placeholders::_5);

                hpx::shared_future<void> thisTimeStepFuture = hpx::dataflow(
                    hpx::launch::async,
                    Operation,
                    neighbors,
                    receiveMessagesFutures,
                    lastTimeStepFuture,
                    nanoStep,
                    step);

                using std::swap;
                swap(thisTimeStepFuture, lastTimeStepFuture);
            }
        }

        return lastTimeStepFuture;
    }

    // fixme: use move semantics here
    void update(
        std::vector<int> neighbors,
        std::vector<hpx::shared_future<MESSAGE> > inputFutures,
        // Unused, just here to ensure correct ordering of updates per cell:
        const hpx::shared_future<void>& lastTimeStepReady,
        int nanoStep,
        int step)
    {
        // fixme: lastTimeStepReady.get();

        int targetGlobalNanoStep = step * NANO_STEPS + nanoStep + 1;
        Neighborhood<MESSAGE> hood(targetGlobalNanoStep, neighbors, inputFutures, remoteIDs);
        UpdateEvent event(neighbors, nanoStep, step);

        cell()->update(hood, event);
        hood.sendEmptyMessagesToUnnotifiedNeighbors();
    }

private:
    std::string basename;
    std::vector<int> neighbors;
    typename SharedPtr<GridType>::Type grid;
    int id;
    std::map<int, std::shared_ptr<HPXReceiver<MESSAGE> > > receivers;
    std::map<int, hpx::id_type> remoteIDs;

    static std::string endpointName(const std::string& basename, int sender, int receiver)
    {
        return "HPXDataflowSimulatorEndPoint_" +
            basename +
            "_" +
            StringOps::itoa(sender) +
            "_to_" +
            StringOps::itoa(receiver);

    }

    CELL *cell()
    {
        return grid->baseAddress();
    }
};

}

/**
 * Experimental Simulator based on (surprise surprise) HPX' dataflow
 * operator. Primary use case (for now) is DGSWEM.
 */
template<typename CELL, typename PARTITION = UnstructuredStripingPartition>
class HPXDataflowSimulator : public HierarchicalSimulator<CELL>
{
public:
    typedef typename APITraits::SelectMessageType<CELL>::Value MessageType;
    typedef HierarchicalSimulator<CELL> ParentType;
    typedef typename DistributedSimulator<CELL>::Topology Topology;
    typedef PartitionManager<Topology> PartitionManagerType;
    using DistributedSimulator<CELL>::NANO_STEPS;
    using DistributedSimulator<CELL>::initializer;
    using HierarchicalSimulator<CELL>::initialWeights;

    /**
     * basename will be added to IDs for use in AGAS lookup, so for
     * each simulation all localities need to use the same basename,
     * but if you intent to run multiple different simulations in a
     * single program, either in parallel or sequentially, you'll need
     * to use a different basename.
     */
    inline HPXDataflowSimulator(
        Initializer<CELL> *initializer,
        const std::string& basename,
        int loadBalancingPeriod = 10000,
        bool enableFineGrainedParallelism = true,
        int chunkSize = 1000) :
        ParentType(
            initializer,
            loadBalancingPeriod * NANO_STEPS,
            enableFineGrainedParallelism),
        basename(basename),
        chunkSize(chunkSize)
    {}

    void step()
    {
        throw std::logic_error("HPXDataflowSimulator::step() not implemented");
    }

    long currentNanoStep() const
    {
        throw std::logic_error("HPXDataflowSimulator::currentNanoStep() not implemented");
        return 0;
    }

    void balanceLoad()
    {
        throw std::logic_error("HPXDataflowSimulator::balanceLoad() not implemented");
    }
    void run()
    {
        Region<1> localRegion;
        CoordBox<1> box = initializer->gridBox();
        std::size_t rank = hpx::get_locality_id();
        std::size_t numLocalities = hpx::get_num_localities().get();

        std::vector<double> rankSpeeds(numLocalities, 1.0);
        std::vector<std::size_t> weights = initialWeights(
            box.dimensions.prod(),
            rankSpeeds);

        Region<1> globalRegion;
        globalRegion << box;

        typename SharedPtr<PARTITION>::Type partition(
            new PARTITION(
                box.origin,
                box.dimensions,
                0,
                weights,
                initializer->getAdjacency(globalRegion)));

        PartitionManager<Topology> partitionManager;
        partitionManager.resetRegions(
            initializer,
            box,
            partition,
            rank,
            1);

        localRegion = partitionManager.ownRegion();
        SharedPtr<Adjacency>::Type adjacency = initializer->getAdjacency(localRegion);


        // fixme: instantiate components in agas and only hold ids of those
        typedef HPXDataFlowSimulatorHelpers::CellComponent<CELL, MessageType> ComponentType;
        typedef typename ComponentType::GridType GridType;

        std::map<int, ComponentType> components;
        std::vector<int> neighbors;

        for (Region<1>::Iterator i = localRegion.begin(); i != localRegion.end(); ++i) {
            int id = i->x();
            CoordBox<1> singleCellBox(Coord<1>(id), Coord<1>(1));
            typename SharedPtr<GridType>::Type grid(new GridType(singleCellBox));
            initializer->grid(&*grid);

            neighbors.clear();
            adjacency->getNeighbors(i->x(), &neighbors);
            HPXDataFlowSimulatorHelpers::CellComponent<CELL, MessageType> component(
                basename,
                grid,
                i->x(),
                neighbors);
            components[i->x()] = component;
        }

        // HPX Reset counters:
        hpx::reset_active_counters();

        typedef hpx::shared_future<void> UpdateResultFuture;
        typedef std::vector<UpdateResultFuture> TimeStepFutures;
        TimeStepFutures lastTimeStepFutures;
        TimeStepFutures nextTimeStepFutures;
        lastTimeStepFutures.reserve(localRegion.size());
        nextTimeStepFutures.reserve(localRegion.size());
        int maxTimeSteps = initializer->maxSteps();

	/*
        for (int startStep = 0; startStep < maxTimeSteps; startStep += chunkSize) {
            int endStep = std::min(maxTimeSteps, startStep + chunkSize);

            for (Region<1>::Iterator i = localRegion.begin(); i != localRegion.end(); ++i) {
                lastTimeStepFutures << components[i->x()].setupDataflow(startStep, endStep);
            }
            hpx::when_all(lastTimeStepFutures).get();
            lastTimeStepFutures.clear();
        }
	*/

        // HPX Sliding semaphore
        int chunkSize = 1000;
        // allow larger look-ahead for dataflow generation to better
        // overlap calculation and computation:
        int lookAheadDistance = 2 * chunkSize;
        hpx::lcos::local::sliding_semaphore semaphore(lookAheadDistance);

        for (Region<1>::Iterator i = localRegion.begin(); i != localRegion.end(); ++i) {
            lastTimeStepFutures << hpx::make_ready_future();
        }

        for (int startStep = 0; startStep < maxTimeSteps; startStep += chunkSize) {
            int endStep = std::min(maxTimeSteps, startStep + chunkSize);
            std::size_t index = 0;
            for (Region<1>::Iterator i = localRegion.begin(); i != localRegion.end(); ++i) {
                nextTimeStepFutures << components[i->x()].setupDataflow(lastTimeStepFutures[index], startStep, endStep);
                ++index;
            }

            nextTimeStepFutures[0].then(
                [&semaphore, startStep](hpx::shared_future<void>) {
                    // inform semaphore about new lower limit
                    semaphore.signal(startStep);
                });

            semaphore.wait(startStep);
            using std::swap;
            swap(lastTimeStepFutures, nextTimeStepFutures);
            nextTimeStepFutures.clear();
        }

        hpx::when_all(lastTimeStepFutures).get();
    }

    std::vector<Chronometer> gatherStatistics()
    {
        // fixme
        return std::vector<Chronometer>();
    }

private:
    std::string basename;
    int chunkSize;
};

}

#endif

#endif
