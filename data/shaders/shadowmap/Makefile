all: fs_sms_shadow.sc varying.def.sc vs_sms_mesh.sc
	../../../build/3rdparty/bgfx/shaderc -f fs_sms_shadow.sc -o ../glsl/fs_sms_shadow.sc.bin --type fragment --platform linux -i ../common --varyingdef -p 120
	../../../build/3rdparty/bgfx/shaderc -f vs_sms_mesh.sc -o ../glsl/vs_sms_mesh.sc.bin --type vertex --platform linux -i ../common --varyingdef -p 120
