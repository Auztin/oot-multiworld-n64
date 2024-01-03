#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/", "godot-cpp/gen/include/godot_cpp/classes/", "godot-cpp/gen/include/godot_cpp/variant/"])
env.Append(LIBPATH=['.'])
env.Append(LIBS=["ftd2xx"])
env.Append(LINKFLAGS=['-s'])
env["DSUFFIXES"] = ".{}.{}{}".format(env["platform"], env["target"], env["DSUFFIXES"])
env["SHOBJSUFFIX"] = ".{}.{}{}".format(env["platform"], env["target"], env["SHOBJSUFFIX"])
sources = Glob("src/*.cpp")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "bin/libMain.{}.{}.framework/libMain.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "bin/libMain{}".format(env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
