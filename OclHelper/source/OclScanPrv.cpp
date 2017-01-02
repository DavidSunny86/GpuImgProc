#include "OclScanPrv.h"
#include "OclUtils.h"

#include <sstream>

using namespace Ocl;

ScanPrv::ScanPrv(cl::Context& ctxt, cl::CommandQueue& queue)
    :mDepth(8),
     mBlkSize(1<<8),
     mContext(ctxt),
     mQueue(queue),
     mIntBuff(mContext, CL_MEM_READ_WRITE|CL_MEM_ALLOC_HOST_PTR, (size_t)(mBlkSize*sizeof(int)))
{
    try
    {
        init(32);
        size_t wgmSize = Ocl::getWorkGroupSizeMultiple(mQueue, mScanKernel);
        //Intel HD4xxx series GPU's warp_size is not 32
        if (wgmSize != 32)
        {
            //re-initialize with wgmSize
            init(wgmSize);
        }
    }

    catch (cl::Error error)
    {
        fprintf(stderr, "Error: %s", error.what());
        exit(0);
    }
}

ScanPrv::~ScanPrv()
{
}

void ScanPrv::init(int warp_size)
{
    std::ostringstream options;
    options << "-DWARP_SIZE=" << warp_size << " -DSH_MEM_SIZE=" << (1 << mDepth);

    cl::Program::Sources source(1, std::make_pair(sSource, strlen(sSource)));
    mProgram = cl::Program(mContext, source);
    mProgram.build(options.str().c_str());

    mScanKernel = cl::Kernel(mProgram, "prefix_sum");
    mAddResKernel = cl::Kernel(mProgram, "add_data");
    mGatherScanKernel = cl::Kernel(mProgram, "gather_scan");
}

size_t ScanPrv::process(Ocl::DataBuffer<int>& buffer)
{
    cl::Event event;
    size_t buffSize = buffer.count();

    mScanKernel.setArg(0, buffer.buffer());
    mScanKernel.setArg(1, (int)buffSize);

    size_t gSize = (buffSize/mBlkSize);
    gSize += ((buffSize%mBlkSize) == 0) ? 0 : 1;
    gSize *= mBlkSize;
    mQueue.enqueueNDRangeKernel(mScanKernel, cl::NullRange, cl::NDRange(gSize/2), cl::NDRange(mBlkSize/2), NULL, &event);
    event.wait();
    size_t time = kernelExecTime(mQueue, event);

    for (size_t i = mBlkSize; i < buffSize; i += (mBlkSize*mBlkSize))
    {
        mGatherScanKernel.setArg(0, buffer);
        mGatherScanKernel.setArg(1, (int)i);
        mGatherScanKernel.setArg(2, (int)buffSize);
        mGatherScanKernel.setArg(3, mIntBuff);
        mQueue.enqueueNDRangeKernel(mGatherScanKernel, cl::NullRange, cl::NDRange(mBlkSize/2), cl::NDRange(mBlkSize/2), NULL, &event);
        event.wait();
        time += kernelExecTime(mQueue, event);

        mAddResKernel.setArg(0, buffer);
        mAddResKernel.setArg(1, (int)i);
        mAddResKernel.setArg(2, (int)buffSize);
        mAddResKernel.setArg(3, mIntBuff);
        mQueue.enqueueNDRangeKernel(mAddResKernel, cl::NullRange, cl::NDRange(mBlkSize*mBlkSize/4), cl::NDRange(mBlkSize/4), NULL, &event);
        event.wait();
        time += kernelExecTime(mQueue, event);
    }
    //printf("\nKernel Time: %llf us", ((double)time)/1000.0);
    return time;
}
