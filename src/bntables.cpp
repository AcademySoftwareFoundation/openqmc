// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <cstdint>

namespace oqmc
{

namespace bntables
{

namespace pmj
{

extern const std::uint32_t keyTable[] = {
#include <oqmc/data/pmj/keys.txt>
};

extern const std::uint32_t rankTable[] = {
#include <oqmc/data/pmj/ranks.txt>
};

} // namespace pmj

} // namespace bntables

} // namespace oqmc
