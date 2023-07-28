import os
from os.path import join, dirname
import shutil

platform = 'x64-mingw-static'

LIBS=['raylib', 'opengl32', 'gdi32', 'winmm']
LIBPATH=[f'vcpkg/installed/{platform}/lib']
CPPPATH=[f'vcpkg/installed/{platform}/include', 'submodule/raylib-cpp/include', 'submodule/tileson']

CCFLAGS='--target=x86_64-w64-windows-gnu -static -Wno-unknown-warning-option'
CXXFLAGS='--target=x86_64-w64-windows-gnu -static -std=c++17 -Wno-unknown-warning-option'
LINKFLAGS = '--target=x86_64-w64-windows-gnu'

VariantDir('build', 'src', duplicate=False)
env = Environment(
	TOOLS=['clang', 'clang++', 'gnulink'],
	CPPPATH=CPPPATH,
	ENV = {'PATH' : os.environ['PATH']},
	LIBS=LIBS,
	LIBPATH=LIBPATH,
	CXXFLAGS=CXXFLAGS,
	LINKFLAGS=LINKFLAGS
)
env['ENV']['TERM'] = os.environ['TERM'] # Colored output

source = [ Glob('build/*.cc') ]
env.Program('bin/hemoglobin.exe', source)
