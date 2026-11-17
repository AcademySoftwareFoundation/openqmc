// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

const std = @import("std");
const Sampler = @import("openqmc").Sampler;

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const sampler = b.option(Sampler, "sampler", "Sampler wrapped by OpenQMC");

    const openqmc = b.dependency("openqmc", .{
        .target = target,
        .optimize = optimize,
        .sampler = sampler,
    });

    const exe = b.addExecutable(.{
        .name = "example-zig",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
            .imports = &.{
                .{ .name = "oqmc", .module = openqmc.module("oqmc") },
            },
        }),
    });
    b.installArtifact(exe);

    const run = b.addRunArtifact(exe);
    b.step("run", "Run the example").dependOn(&run.step);
}
