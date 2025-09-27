// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <memory>
#include <nanobind/nanobind.h>
#include <oqmc/oqmc.h>

namespace nb = nanobind;

// Python-friendly cache wrapper class
template <typename SamplerType>
class SamplerCache
{
  public:
	SamplerCache() : cacheData(new char[SamplerType::cacheSize])
	{
		SamplerType::initialiseCache(cacheData.get());
	}

	[[nodiscard]] const void* data() const
	{
		return cacheData.get();
	}

  private:
	std::unique_ptr<char[]> cacheData;
};

// Macro to reduce sampler binding duplication with improved cache API
#define BIND_SAMPLER(CLASS_NAME, MODULE_NAME)                                  \
	/* Bind the cache class */                                                 \
	nb::class_<SamplerCache<oqmc::CLASS_NAME>>(m, #CLASS_NAME "Cache")         \
	    .def(nb::init<>());                                                    \
                                                                               \
	/* Bind the sampler class */                                               \
	nb::class_<oqmc::CLASS_NAME>(m, #CLASS_NAME)                               \
	    .def(nb::init<int, int, int, int,                                      \
	                  const SamplerCache<oqmc::CLASS_NAME>&>(),                \
	         "x", "y", "frame", "index", "cache",                              \
	         [](int x, int y, int frame, int index,                            \
	            const SamplerCache<oqmc::CLASS_NAME>& cache) {                 \
		         return oqmc::CLASS_NAME(x, y, frame, index, cache.data());    \
	         })                                                                \
	    .def("new_domain", &oqmc::CLASS_NAME::newDomain, "key")                \
	    .def("new_domain_split", &oqmc::CLASS_NAME::newDomainSplit, "key",     \
	         "size", "index")                                                  \
	    .def("new_domain_distrib", &oqmc::CLASS_NAME::newDomainDistrib, "key", \
	         "index")                                                          \
	    .def("new_domain_chain", &oqmc::CLASS_NAME::newDomainChain, "key",     \
	         "index")                                                          \
	    .def("draw_sample_1d",                                                 \
	         [](const oqmc::CLASS_NAME& self) {                                \
		         float sample[1];                                              \
		         self.drawSample<1>(sample);                                   \
		         return sample[0];                                             \
	         })                                                                \
	    .def("draw_sample_2d",                                                 \
	         [](const oqmc::CLASS_NAME& self) {                                \
		         float sample[2];                                              \
		         self.drawSample<2>(sample);                                   \
		         return std::make_pair(sample[0], sample[1]);                  \
	         })                                                                \
	    .def("draw_sample_3d",                                                 \
	         [](const oqmc::CLASS_NAME& self) {                                \
		         float sample[3];                                              \
		         self.drawSample<3>(sample);                                   \
		         return std::make_tuple(sample[0], sample[1], sample[2]);      \
	         })                                                                \
	    .def("draw_sample_4d", [](const oqmc::CLASS_NAME& self) {              \
		    float sample[4];                                                   \
		    self.drawSample<4>(sample);                                        \
		    return std::make_tuple(sample[0], sample[1], sample[2],            \
		                           sample[3]);                                 \
	    });

NB_MODULE(openqmc_python, m)
{
	m.doc() = "OpenQMC Python bindings - Quasi-Monte Carlo sampling library";

	BIND_SAMPLER(PmjSampler, "pmj")
	BIND_SAMPLER(PmjBnSampler, "pmjbn")
	BIND_SAMPLER(SobolSampler, "sobol")
	BIND_SAMPLER(SobolBnSampler, "sobolbn")
	BIND_SAMPLER(LatticeSampler, "lattice")
	BIND_SAMPLER(LatticeBnSampler, "latticebn")
}
