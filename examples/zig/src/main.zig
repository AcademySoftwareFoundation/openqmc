// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

// Calls every function in the C ABI through the module translated from the C
// header, with no hand written bindings. Built as a separate project, this
// checks that OpenQMC works as a Zig package dependency. Mirrors the C example
// in cmake/examples/cabi/main.c.

const std = @import("std");
const oqmc = @import("oqmc");

comptime {
    std.debug.assert(@sizeOf(oqmc.oqmc_sampler) == 16);
    std.debug.assert(@alignOf(oqmc.oqmc_sampler) == 8);
}

var failures: u32 = 0;

fn check(condition: bool, message: []const u8) void {
    if (!condition) {
        std.debug.print("FAILED: {s}\n", .{message});
        failures += 1;
    }
}

fn checkDraws(sampler: *const oqmc.oqmc_sampler) void {
    var integers: [4]u32 = undefined;
    var floats: [4]f32 = undefined;

    oqmc.oqmc_sampler_draw_sample_u(sampler, integers.len, &integers);
    oqmc.oqmc_sampler_draw_sample_f(sampler, floats.len, &floats);

    for (floats) |value| {
        check(value >= 0 and value < 1, "sample in [0, 1)");
    }

    oqmc.oqmc_sampler_draw_rnd_u(sampler, integers.len, &integers);
    oqmc.oqmc_sampler_draw_rnd_f(sampler, floats.len, &floats);

    for (floats) |value| {
        check(value >= 0 and value < 1, "rnd in [0, 1)");
    }
}

pub fn main(init: std.process.Init) !void {
    const name: []const u8 = if (oqmc.oqmc_sampler_name()) |ptr| std.mem.span(ptr) else "";
    check(name.len > 0, "name is not empty");

    // The cache holds tables of 32 bit integers.
    const cache = try init.gpa.alignedAlloc(u8, .of(u32), oqmc.oqmc_sampler_cache_size());
    defer init.gpa.free(cache);

    oqmc.oqmc_sampler_initialise_cache(cache.ptr);

    var sampler = oqmc.oqmc_sampler_create(2, 3, 0, 0, cache.ptr);
    checkDraws(&sampler);

    sampler = oqmc.oqmc_sampler_new_domain(&sampler, 1);
    checkDraws(&sampler);

    const split = oqmc.oqmc_sampler_new_domain_split(&sampler, 2, 4, 3);
    checkDraws(&split);

    const distrib = oqmc.oqmc_sampler_new_domain_distrib(&sampler, 3, 1);
    checkDraws(&distrib);

    const chain = oqmc.oqmc_sampler_new_domain_chain(&sampler, 4, 1);
    checkDraws(&chain);

    if (failures > 0) {
        std.debug.print("{d} check(s) failed for sampler '{s}'\n", .{ failures, name });
        return error.ChecksFailed;
    }

    std.debug.print("All checks passed for sampler '{s}'\n", .{name});
}
