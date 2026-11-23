// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

const std = @import("std");
const Translator = @import("translate_c").Translator;

/// Sampler wrapped by the C ABI library, chosen at build time. Public so that
/// dependent build scripts can declare a matching option.
pub const Sampler = enum { PMJ, PMJBN, SOBOL, SOBOLBN, LATTICE, LATTICEBN };

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const sampler = b.option(
        Sampler,
        "sampler",
        "Sampler wrapped by the C ABI library (default: PMJBN)",
    ) orelse .PMJBN;
    const shared = b.option(
        bool,
        "shared",
        "Build a shared library, for C and C++ users (default: false)",
    ) orelse false;

    // Compile the C ABI library into a static archive, or a shared library.
    // libc++ is needed only for headers such as <cstdint>, so nothing beyond
    // libc is linked.
    const lib = b.addLibrary(.{
        .name = "oqmc",
        .linkage = if (shared) .dynamic else .static,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libcpp = true,
            .strip = optimize != .debug,
        }),
    });
    lib.root_module.addIncludePath(b.path("include"));
    lib.root_module.addCMacro(samplerMacro(sampler), "1");
    if (shared) lib.root_module.addCMacro("OQMC_CABI_EXPORT_SYMBOLS", "1");
    lib.root_module.addCSourceFile(.{
        .file = b.path("src/oqmc_impl.cpp"),
        // Hide all symbols, so that they're private to any binary that links
        // the archive, such as a plugin, and separate copies can't bind to one
        // another. Also set standard to be strictly C++17.
        .flags = &.{ "-std=c++17", "-fvisibility=hidden" },
    });
    lib.installHeader(b.path("include/oqmc/oqmc_c.h"), "oqmc/oqmc_c.h");
    b.installArtifact(lib);

    // Translate the C header into the Zig module "oqmc". Importing the module
    // also links the library.
    const translate_c = b.dependency("translate_c", .{});
    const t: Translator = .init(translate_c, .{
        .c_source_file = b.path("include/oqmc/oqmc_c.h"),
        .target = target,
        .optimize = optimize,
    });
    t.linkLibrary(lib);

    // Translator has no addModule, so register its module directly, as
    // b.addModule does. Likely to change as the translate-c package evolves.
    b.modules.put(b.graph.arena, "oqmc", t.mod) catch @panic("OOM");
}

fn samplerMacro(sampler: Sampler) []const u8 {
    return switch (sampler) {
        .PMJ => "OQMC_CABI_SAMPLER_PMJ",
        .PMJBN => "OQMC_CABI_SAMPLER_PMJBN",
        .SOBOL => "OQMC_CABI_SAMPLER_SOBOL",
        .SOBOLBN => "OQMC_CABI_SAMPLER_SOBOLBN",
        .LATTICE => "OQMC_CABI_SAMPLER_LATTICE",
        .LATTICEBN => "OQMC_CABI_SAMPLER_LATTICEBN",
    };
}
