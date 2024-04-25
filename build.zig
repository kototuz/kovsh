const std = @import("std");

const c_flags = [_][]const u8{
    "-Wall",
    "-Wextra",
    "-pedantic",
};

const kovsh_main = "src/main.c";
const test_dir = "tests";

const source_files = [_][]const u8{
    "src/parse/lexer.c",
    "src/parse/rule.c",
    "src/parse/eval.c"
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Options
    const main = b.option([]const u8, "main", "Main file that contains a main function");
    const sources = b.option([]const []const u8, "sources", "Source files");

    stepRun(
        b,
        main orelse kovsh_main,
        sources orelse &.{},
        target,
        optimize
    );
    stepBuildKovsh(b, target, optimize);
    stepCompile(b, main, target, optimize);
}

fn stepRun(
    b: *std.Build,
    main: []const u8,
    sources: []const []const u8,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode
) void {
    const exe = b.addExecutable(.{
        .name = "exe",
        .target = target,
        .optimize = optimize
    });

    exe.linkLibC();
    exe.addCSourceFile(.{
        .file = .{ .path = main },
        .flags = &c_flags
    });

    exe.addCSourceFiles(.{
        .files = sources,
        .flags = &c_flags
    });

    const run = b.addRunArtifact(exe);

    const run_step = b.step("run", "Run an executable");
    run_step.dependOn(&run.step);
}

fn stepTest(
    b: *std.Build,
    file: ?[]const u8,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode
) !void {
    _ = b;
    _ = file;
    _ = target;
    _ = optimize;
    @panic("not implemented");
}

fn stepBuildKovsh(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode
) void {
    const exe = b.addExecutable(.{
        .name = "kovsh",
        .target = target,
        .optimize = optimize
    });

    exe.linkLibC();
    exe.addCSourceFile(.{
        .file = .{ .path = kovsh_main},
        .flags = &c_flags
    });

    exe.addCSourceFiles(.{
        .files = &source_files,
        .flags = &c_flags
    });

    b.getInstallStep().dependOn(&exe.step);
}

fn stepCompile(
    b: *std.Build,
    file: ?[]const u8,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode
) void {
    const object = b.addObject(.{
        .name = "obj",
        .target = target,
        .optimize = optimize
    });

    object.linkLibC();
    object.addCSourceFile(.{
        .file = .{ .path = file orelse kovsh_main},
        .flags = &c_flags
    });

    const step = b.step("compile", "Compile C file");
    step.dependOn(&object.step);
}


test "build tests" {
    const Foo = struct {
        const str = "Hello, World";
        num: u32
    };
    std.log.warn("{}", .{@typeInfo(Foo).Struct.fields.len});
}
