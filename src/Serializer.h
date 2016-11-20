#pragma once

#include "luatables.h"
#include "SimpleMath/SimpleMath.h"
#include <iostream>

// uint16_t
template<> 
uint16_t LuaTableNode::getDefault<uint16_t>(
		const uint16_t &default_value);

template<>
void LuaTableNode::set<uint16_t>(
		const uint16_t &value);

// Vector3f
template<> 
SimpleMath::Vector3f LuaTableNode::getDefault<SimpleMath::Vector3f>(
		const SimpleMath::Vector3f &default_value);

template<>
void LuaTableNode::set<SimpleMath::Vector3f>(
		const SimpleMath::Vector3f &value);

/*
template<SimpleMath::Matrix44f> 
SimpleMath::Matrix44f LuaTableNode::getDefault<SimpleMath::Matrix44f>(
		const SimpleMath::Matrix44f &default_value);

template<SimpleMath::Matrix44f>
void LuaTableNode::set<SimpleMath::Matrix44f>(
		const SimpleMath::Matrix44f &value);
		*/
