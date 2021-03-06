#pragma once
/*
* Covariant Script Mathematics Extension
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Copyright (C) 2018 Michael Lee(李登淳)
* Email: mikecovlee@163.com
* Github: https://github.com/mikecovlee
*/
#include <covscript/cni.hpp>
#include <hexagon/ort.h>
#include <algorithm>

static cs::extension math_ext;
namespace math_cs_ext {
	using namespace cs;

	number abs(number n)
	{
		return std::abs(n);
	}

	number ln(number n)
	{
		return std::log(n);
	}

	number log10(number n)
	{
		return std::log10(n);
	}

	number log(number a, number b)
	{
		return std::log(b / a);
	}

	number sin(number n)
	{
		return std::sin(n);
	}

	number cos(number n)
	{
		return std::cos(n);
	}

	number tan(number n)
	{
		return std::tan(n);
	}

	number asin(number n)
	{
		return std::asin(n);
	}

	number acos(number n)
	{
		return std::acos(n);
	}

	number atan(number n)
	{
		return std::atan(n);
	}

	number sqrt(number n)
	{
		return std::sqrt(n);
	}

	number root(number a, number b)
	{
		return std::pow(a, number(1) / b);
	}

	number pow(number a, number b)
	{
		return std::pow(a, b);
	}

	number min(number a, number b)
	{
		return std::min(a, b);
	}

	number max(number a, number b)
	{
		return std::max(a, b);
	}

	void init()
	{
		math_ext.add_var("pi", var::make_constant<number>(3.1415926535));
		math_ext.add_var("e", var::make_constant<number>(2.7182818284));
		math_ext.add_var("abs", var::make_protect<callable>(cni(abs), true));
		math_ext.add_var("ln", var::make_protect<callable>(cni(ln), true));
		math_ext.add_var("log10", var::make_protect<callable>(cni(log10), true));
		math_ext.add_var("log", var::make_protect<callable>(cni(log), true));
		math_ext.add_var("sin", var::make_protect<callable>(cni(sin), true));
		math_ext.add_var("cos", var::make_protect<callable>(cni(cos), true));
		math_ext.add_var("tan", var::make_protect<callable>(cni(tan), true));
		math_ext.add_var("asin", var::make_protect<callable>(cni(asin), true));
		math_ext.add_var("acos", var::make_protect<callable>(cni(acos), true));
		math_ext.add_var("atan", var::make_protect<callable>(cni(atan), true));
		math_ext.add_var("sqrt", var::make_protect<callable>(cni(sqrt), true));
		math_ext.add_var("root", var::make_protect<callable>(cni(root), true));
		math_ext.add_var("pow", var::make_protect<callable>(cni(pow), true));
		math_ext.add_var("min", var::make_protect<callable>(cni(min), true));
		math_ext.add_var("max", var::make_protect<callable>(cni(max), true));
	}
}

class math_ext_hvm_impl : public ort::ProxiedObject {
public:
	math_ext_hvm_impl() {
	}

	virtual void Init(ort::ObjectProxy& proxy) {
		ort::Runtime& rt = *cs::get_active_runtime();

		proxy.SetStaticField("pi", ort::Value::FromFloat(3.1415926535));
		proxy.SetStaticField("e", ort::Value::FromFloat(2.7182818284));
		proxy.SetStaticField("abs", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::abs(
				rt.GetArgument(0).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("ln", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::ln(
				rt.GetArgument(0).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("log10", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::log10(
				rt.GetArgument(0).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("log", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::log(
				rt.GetArgument(0).ToF64(),
				rt.GetArgument(1).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("sin", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::sin(
				rt.GetArgument(0).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("cos", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::cos(
				rt.GetArgument(0).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("tan", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::tan(
				rt.GetArgument(0).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("asin", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::asin(
				rt.GetArgument(0).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("acos", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::acos(
				rt.GetArgument(0).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("atan", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::atan(
				rt.GetArgument(0).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("sqrt", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::sqrt(
				rt.GetArgument(0).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("root", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::root(
				rt.GetArgument(0).ToF64(),
				rt.GetArgument(1).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("pow", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::pow(
				rt.GetArgument(0).ToF64(),
				rt.GetArgument(1).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("min", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::min(
				rt.GetArgument(0).ToF64(),
				rt.GetArgument(1).ToF64()
			));
		}).Pin(rt));
		proxy.SetStaticField("max", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(math_cs_ext::max(
				rt.GetArgument(0).ToF64(),
				rt.GetArgument(1).ToF64()
			));
		}).Pin(rt));
		proxy.Freeze();
	}
};
