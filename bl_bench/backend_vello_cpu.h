#ifndef BLBENCH_BACKEND_VELLO_CPU_H
#define BLBENCH_BACKEND_VELLO_CPU_H

#include "backend.h"
#include "cpu_sparse.h"

namespace blbench {

    Backend* createVelloCpuBackend(uint32_t threadCount);

} // {blbench}

#endif // BLBENCH_BACKEND_VELLO_CPU_H