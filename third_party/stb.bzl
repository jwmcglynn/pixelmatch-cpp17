"""
Module for building STB libraries.
This module provides a macro for creating cc_library targets from STB header files.
"""

load("@rules_cc//cc:defs.bzl", "cc_library")

def stb_library(name, emit_definition_macro = "", stb_prefix = True, copts = None, defines = None, definition_includes = None):
    """Creates an STB library target, optionally emitting a definition macro.

    Args:
        name: Base name of the STB header (e.g., "image", "vorbis").
        emit_definition_macro: If non-empty, emits a genrule to define the library.
        stb_prefix: If True, prefixes the header file with "stb_".
        copts: Additional compiler options (list of strings).
        defines: Additional defines for the cc_library.
        definition_includes: List of headers to include when emitting definition.
    """

    # Avoid mutable default arguments.
    if copts == None:
        copts = []
    if defines == None:
        defines = []
    if definition_includes == None:
        definition_includes = []
    definition_name = "{}_definition".format(name)
    header_file_name = "{0}{1}.h".format("stb_" if stb_prefix else "", name)
    src_file_name = "{}_definition.cpp".format(name)

    if emit_definition_macro:
        includes = "\n".join(["#include <{}>".format(include) for include in definition_includes] + [""])

        # Generate the source file for lib compilation
        src_file_content = "{}#define {}\n#include <stb/{}>".format(
            includes,
            emit_definition_macro,
            header_file_name,
        )
        native.genrule(
            name = definition_name,
            outs = [src_file_name],
            cmd = 'echo "{0}" > $(@D)/{1}'.format(
                src_file_content,
                src_file_name,
            ),
            visibility = ["//visibility:private"],
        )
    cc_library(
        name = name,
        hdrs = ["{}".format(header_file_name)],
        include_prefix = "stb",
        srcs = [src_file_name] if emit_definition_macro else [],
        copts = copts,
        defines = defines,
        visibility = ["//visibility:public"],
    )
