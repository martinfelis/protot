all: fs_debug.sc varying.def.sc vs_debug.sc
	../../../build/3rdparty/bgfx/shaderc -f fs_debug.sc -o ../glsl/fs_debug.sc.bin --type fragment --platform linux -i ../common --varyingdef -p 120
	../../../build/3rdparty/bgfx/shaderc -f vs_debug.sc -o ../glsl/vs_debug.sc.bin --type vertex --platform linux -i ../common --varyingdef -p 120
