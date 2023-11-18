cmake_minimum_required(VERSION 3.20.0)

project(HilbertCurve)

add_executable(HilbertCurve
    code/main.cpp
    code/hilbert.cpp)

set(CMAKE_SUPPRESS_REGENERATION true)

set_target_properties(HilbertCurve PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
set_property(TARGET HilbertCurve PROPERTY CXX_STANDARD 17)

add_compile_options(/Zi /FC /nologo /W4 /wd4201)
add_link_options(user32.lib gdi32.lib winmm.lib)

