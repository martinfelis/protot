all: fs_sms_mesh.sc fs_sms_mesh_textured.sc fs_sms_shadow.sh varying.def.sc vs_sms_mesh.sc vs_sms_mesh_textured.sc
	../../../build/3rdparty/bgfx/shaderc -f fs_sms_mesh.sc -o ../glsl/fs_sms_mesh.sc.bin --type fragment --platform linux -i ../common --varyingdef -p 120
	../../../build/3rdparty/bgfx/shaderc -f fs_sms_mesh_textured.sc -o ../glsl/fs_sms_mesh_textured.sc.bin --type fragment --platform linux -i ../common --varyingdef -p 120
	../../../build/3rdparty/bgfx/shaderc -f vs_sms_mesh.sc -o ../glsl/vs_sms_mesh.sc.bin --type vertex --platform linux -i ../common --varyingdef -p 120
	../../../build/3rdparty/bgfx/shaderc -f vs_sms_mesh_textured.sc -o ../glsl/vs_sms_mesh_textured.sc.bin --type vertex --platform linux -i ../common --varyingdef -p 120

