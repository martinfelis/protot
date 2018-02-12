all: fs_lines_occluded.sc fs_lines.sc varying.def.sc vs_lines.sc
	../../../build/3rdparty/bgfx/shaderc -f fs_lines.sc -o ../glsl/fs_lines.sc.bin --type fragment --platform linux -i ../common --varyingdef -p 120
	../../../build/3rdparty/bgfx/shaderc -f fs_lines_occluded.sc -o ../glsl/fs_lines_occluded.sc.bin --type fragment --platform linux -i ../common --varyingdef -p 120
	../../../build/3rdparty/bgfx/shaderc -f vs_lines.sc -o ../glsl/vs_lines.sc.bin --type vertex --platform linux -i ../common --varyingdef -p 120

