all: fs_ibl_skybox.sc shaderlib.sh uniforms.sh varying.def.sc vs_ibl_skybox.sc
	../../../build/3rdparty/bgfx/shaderc -f fs_ibl_skybox.sc -o ../glsl/fs_ibl_skybox.sc.bin --type fragment --platform linux -i ../common --varyingdef -p 120
	../../../build/3rdparty/bgfx/shaderc -f vs_ibl_skybox.sc -o ../glsl/vs_ibl_skybox.sc.bin --type vertex --platform linux -i ../common --varyingdef -p 120
