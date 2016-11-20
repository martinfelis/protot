#include "Serializer.h"

extern "C"
{
   #include "lua.h"
   #include "lauxlib.h"
   #include "lualib.h"
}

void l_push_LuaKey (lua_State *L, const LuaKey &key) {
	if (key.type == LuaKey::Integer)
		lua_pushnumber (L, key.int_value);
	else
		lua_pushstring(L, key.string_value.c_str());
}

// uint16_t
template<> 
uint16_t LuaTableNode::getDefault<uint16_t>(
		const uint16_t &default_value) {
	uint16_t result = default_value;

	if (stackQueryValue()) {
		result = static_cast<uint16_t>(lua_tonumber (luaTable->L, -1));
	}

	stackRestore();

	return result;
}

template<>
void LuaTableNode::set<uint16_t>(
		const uint16_t &value) {
	stackCreateLuaTable();

	l_push_LuaKey (luaTable->L, key);
	lua_pushnumber(luaTable->L, static_cast<double>(value));
	// stack: parent, key, value
	lua_settable (luaTable->L, -3);

	stackRestore();
}



// Vector3f
template<> 
SimpleMath::Vector3f LuaTableNode::getDefault<SimpleMath::Vector3f>(
		const SimpleMath::Vector3f &default_value) {
	SimpleMath::Vector3f result = default_value;

	if (stackQueryValue()) {
		LuaTable vector_table = LuaTable::fromLuaState (luaTable->L);

		if (vector_table.length() != 3) {
			std::cerr << "LuaModel Error: invalid 3d vector!" << std::endl;
			abort();
		}

		result[0] = vector_table[1];
		result[1] = vector_table[2];
		result[2] = vector_table[3];
	} else {
		std::cout << "did not get value" << std::endl;
	}

	stackRestore();

	return result;
}

template<>
void LuaTableNode::set<SimpleMath::Vector3f>(
		const SimpleMath::Vector3f &value) {
	LuaTable custom_table = stackCreateLuaTable();

	custom_table[1] = static_cast<double>(value[0]);
	custom_table[2] = static_cast<double>(value[1]);
	custom_table[3] = static_cast<double>(value[2]);

	stackRestore();
}

/*
template<SimpleMath::Matrix44f> 
SimpleMath::Matrix44f LuaTableNode::getDefault<SimpleMath::Matrix44f>(
		const SimpleMath::Matrix44f &default_value) {

	SimpleMath::Matrix44f result = default_value;

	if (stackQueryValue()) {
		LuaTable vector_table = LuaTable::fromLuaState (luaTable->L);

		if (vector_table.length() != 3) {
			std::cerr << "LuaModel Error: invalid 3d vector!" << std::endl;
			abort();
		}

		result(0,0) = vector_table[1][1];
		result(0,1) = vector_table[1][2];
		result(0,2) = vector_table[1][3];
		result(0,3) = vector_table[1][4];

		result(1,0) = vector_table[2][1];
		result(1,1) = vector_table[2][2];
		result(1,2) = vector_table[2][3];
		result(1,3) = vector_table[2][4];

		result(2,0) = vector_table[3][1];
		result(2,1) = vector_table[3][2];
		result(2,2) = vector_table[3][3];
		result(2,3) = vector_table[3][4];

		result(3,0) = vector_table[4][1];
		result(3,1) = vector_table[4][2];
		result(3,2) = vector_table[4][3];
		result(3,3) = vector_table[4][4];
	}

	stackRestore();

	return result;
}

template<SimpleMath::Matrix44f>
void LuaTableNode::set<SimpleMath::Matrix44f>(
		const SimpleMath::Matrix44f &value) {

	LuaTable custom_table = stackCreateLuaTable();

	custom_table[1][1] = static_cast<double>(value(0,0));
	custom_table[1][2] = static_cast<double>(value(0,1));
	custom_table[1][3] = static_cast<double>(value(0,2));
	custom_table[1][4] = static_cast<double>(value(0,3));

	custom_table[2][1] = static_cast<double>(value(1,0));
	custom_table[2][2] = static_cast<double>(value(1,1));
	custom_table[2][3] = static_cast<double>(value(1,2));
	custom_table[2][4] = static_cast<double>(value(1,3));

	custom_table[3][1] = static_cast<double>(value(2,0));
	custom_table[3][2] = static_cast<double>(value(2,1));
	custom_table[3][3] = static_cast<double>(value(2,2));
	custom_table[3][4] = static_cast<double>(value(2,3));

	custom_table[4][1] = static_cast<double>(value(3,0));
	custom_table[4][2] = static_cast<double>(value(3,1));
	custom_table[4][3] = static_cast<double>(value(3,2));
	custom_table[4][4] = static_cast<double>(value(3,3));

	stackRestore();
}
*/
