#include "GPUClusterFinder.h"

#include <gpucf/ClEnv.h>
#include <gpucf/common/log.h>
#include <gpucf/common/RowInfo.h>

#include <shared/tpc.h>


using namespace gpucf;


std::vector<Digit> GPUClusterFinder::getPeaks() const
{
    return peaks;
}

void GPUClusterFinder::setupImpl(ClEnv &env, const DataSet &data)
{
    digits = data.deserialize<Digit>();

    context = env.getContext(); 
    device  = env.getDevice();

    cl::Program cfprg = env.buildFromSrc("clusterFinder.cl");
    findClusters = cl::Kernel(cfprg, "findClusters");
    digitsToChargeMap = cl::Kernel(cfprg, "digitsToChargeMap");

    // create buffers
    digitsBufSize = sizeof(Digit) * digits.size();
    digitsBuf = cl::Buffer(context,
                          CL_MEM_READ_ONLY,
                          digitsBufSize);

    globalToLocalRow = RowInfo::instance().globalToLocalMap;
    const size_t numOfRows = globalToLocalRow.size();
    globalToLocalRowBufSize = sizeof(cl_int) * numOfRows;
    globalToLocalRowBuf = cl::Buffer(context,
                                     CL_MEM_READ_ONLY,
                                     globalToLocalRowBufSize);

    isPeakBufSize = digits.size() * sizeof(cl_int);
    isPeakBuf = cl::Buffer(context, 
                           CL_MEM_WRITE_ONLY, 
                           isPeakBufSize);

    clusterBufSize = digits.size() * sizeof(Cluster);
    clusterBuf = cl::Buffer(context,
                            CL_MEM_READ_WRITE,
                            clusterBufSize);

    log::Info() << "Found " << numOfRows << " rows";

    chargeMapSize  = 
        numOfRows * TPC_PADS_PER_ROW_PADDED 
                  * TPC_MAX_TIME_PADDED 
                  * sizeof(cl_float);
    chargeMap = cl::Buffer(context, CL_MEM_READ_WRITE, chargeMapSize);
}

GPUAlgorithm::Result GPUClusterFinder::runImpl()
{
    static_assert(sizeof(cl_int) == sizeof(int));

    log::Info() << "Looking for clusters...";

    // Setup queue
    cl::CommandQueue queue(context, device);

    ASSERT(globalToLocalRowBufSize > 0);
    queue.enqueueWriteBuffer(globalToLocalRowBuf, CL_FALSE, 0, 
            globalToLocalRowBufSize, globalToLocalRow.data());

    ASSERT(digitsBufSize > 0);
    queue.enqueueWriteBuffer(digitsBuf, CL_FALSE, 0, digitsBufSize, 
            digits.data());

    ASSERT(chargeMapSize > 0);
    queue.enqueueFillBuffer(chargeMap, 0.0f, 0, chargeMapSize);

    cl::NDRange global(digits.size());
    cl::NDRange local(16);

    digitsToChargeMap.setArg(0, digitsBuf);
    digitsToChargeMap.setArg(1, chargeMap);
    queue.enqueueNDRangeKernel(digitsToChargeMap, cl::NullRange, global, local);

    findClusters.setArg(0, chargeMap);
    findClusters.setArg(1, digitsBuf);
    findClusters.setArg(2, globalToLocalRowBuf);
    findClusters.setArg(3, isPeakBuf);
    findClusters.setArg(4, clusterBuf);
    queue.enqueueNDRangeKernel(findClusters, cl::NullRange, global, local);

    log::Info() << "Copy results back...";
    std::vector<Cluster> clusters(digits.size());
    ASSERT(clusters.size() * sizeof(Cluster) == clusterBufSize);
    queue.enqueueReadBuffer(clusterBuf, CL_TRUE, 0, clusterBufSize, 
            clusters.data());

    std::vector<int> isClusterCenter(digits.size());
    ASSERT(isClusterCenter.size() * sizeof(int) == isPeakBufSize);
    queue.enqueueReadBuffer(isPeakBuf, CL_TRUE, 0, isPeakBufSize,
            isClusterCenter.data());

    printClusters(isClusterCenter, clusters, 10);

    clusters = filterCluster(isClusterCenter, clusters);
    peaks = findPeaks(isClusterCenter, digits);

    ASSERT(clusters.size() == peaks.size());

    log::Info() << "Found " << clusters.size() << " clusters.";

    DataSet res;
    res.serialize(clusters);

    return GPUAlgorithm::Result{res, Measurements()};
}


void GPUClusterFinder::printClusters(
        const std::vector<int> &isCenter,
        const std::vector<Cluster> &clusters,
        size_t maxClusters)
{
    log::Debug() << "Printing found clusters";
    ASSERT(isCenter.size() == clusters.size());
    for (size_t i = 0; i < isCenter.size(); i++)
    {
        ASSERT(isCenter[i] == 0 || isCenter[i] == 1);
        if (isCenter[i])
        {
            log::Debug() << clusters[i];
            maxClusters--;
            if (maxClusters == 0)
            {
                break;
            }
        }
    }
}

std::vector<Cluster> GPUClusterFinder::filterCluster(
        const std::vector<int> &isCenter,
        const std::vector<Cluster> &clusters)
{
    std::vector<Cluster> actualClusters; 

    for (size_t i = 0; i < clusters.size(); i++)
    {
        if (isCenter[i])
        {
            actualClusters.push_back(clusters[i]);
        }
    }

    return actualClusters;
}

std::vector<Digit> GPUClusterFinder::findPeaks(
        const std::vector<int> &isCenter,
        const std::vector<Digit> &digits)
{
    std::vector<Digit> peaks;    

    for (size_t i = 0; i < digits.size(); i++)
    {
        if (isCenter[i])
        {
            peaks.push_back(digits[i]);
        }
    }

    return peaks;
}

// vim: set ts=4 sw=4 sts=4 expandtab: