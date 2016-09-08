all: fs_sms_mesh.sc vs_sms_mesh.sc varying.def.sc fs_sms_shadow.sh
	../../build/shaderc -f fs_sms_mesh.sc -o ../glsl/fs_sms_mesh.bin --type fragment --platform linux -i ../common --varyingdef -p 120
	../../build/shaderc -f vs_sms_mesh.sc -o ../glsl/vs_sms_mesh.bin --type vertex --platform linux -i ../common --varyingdef -p 120
	../../build/shaderc -f fs_sms_mesh.sc -o ../glsl/fs_sms_mesh.bin --type fragment --platform linux -i ../common --varyingdef -p 120 --depends

