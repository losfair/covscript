#pragma once
/*
* Covariant Script Runtime Extension
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
#include <mozart/random.hpp>
#include <mozart/timer.hpp>
#include <covscript/cni.hpp>
#include <covscript/runtime.hpp>
#include <cstdlib>
#include <hexagon/ort.h>
#include <vector>
#include <unordered_map>
#include <functional>

static cs::extension context_ext;
static cs::extension_t context_ext_shared = cs::make_shared_extension(context_ext);
namespace cs_impl {
	template<>
	cs::extension_t &get_ext<cs::context_t>()
	{
		return context_ext_shared;
	}

	template<>
	constexpr const char *get_name_of_type<cov::tree<cs::token_base * >>
	        ()
	{
		return "cs::expression";
	}
}
static cs::extension runtime_ext;
namespace runtime_cs_ext {
	using namespace cs;

	string get_import_path()
	{
		return import_path;
	}

	void info()
	{
		std::cout << "Covariant Script Programming Language Interpreter\nVersion: " << cs::version << "\n"
		          "Copyright (C) 2018 Michael Lee.All rights reserved.\n"
		          "This program is free software: you can redistribute it and/or modify "
		          "it under the terms of the GNU Affero General Public License as published "
		          "by the Free Software Foundation, either version 3 of the License, or "
		          "(at your option) any later version.\n"
		          "\n"
		          "This program is distributed in the hope that it will be useful, "
		          "but WITHOUT ANY WARRANTY; without even the implied warranty of "
		          "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		          "GNU Affero General Public License for more details.\n"
		          "\n"
		          "You should have received a copy of the GNU Affero General Public License "
		          "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
		          "Please visit <http://covscript.org/> for more information." << std::endl;
	}

	number time()
	{
		return cov::timer::time(cov::timer::time_unit::milli_sec);
	}

	void delay(number time)
	{
		cov::timer::delay(cov::timer::time_unit::milli_sec, time);
	}

	number rand(number b, number e)
	{
		return cov::rand<number>(b, e);
	}

	number randint(number b, number e)
	{
		return cov::rand<long>(b, e);
	}

	var exception(const string &str)
	{
		return var::make<lang_error>(str);
	}

	std::size_t hash(const var &val)
	{
		return val.hash();
	}

	using expression_t=cov::tree<token_base *>;

	var build(const context_t &context, const string &expr)
	{
		std::deque<char> buff;
		std::deque<token_base *> tokens;
		expression_t tree;
		for (auto &ch:expr)
			buff.push_back(ch);
		context->instance->process_char_buff(buff, tokens);
		context->instance->process_brackets(tokens);
		context->instance->kill_brackets(tokens);
		context->instance->gen_tree(tree, tokens);
		return var::make<expression_t>(tree);
	}

	var solve(const context_t &context, expression_t &tree)
	{
		return context->instance->parse_expr(tree.root());
	}

	var dynamic_import(const context_t &context, const string &path, const string &name)
	{
		return var::make_protect<extension_t>(context->instance->import(path, name));
	}

	void init()
	{
		runtime_ext.add_var("std_version", var::make_constant<number>(std_version));
		runtime_ext.add_var("get_import_path", var::make_protect<callable>(cni(get_import_path), true));
		runtime_ext.add_var("info", var::make_protect<callable>(cni(info)));
		runtime_ext.add_var("time", var::make_protect<callable>(cni(time)));
		runtime_ext.add_var("delay", var::make_protect<callable>(cni(delay)));
		runtime_ext.add_var("rand", var::make_protect<callable>(cni(rand)));
		runtime_ext.add_var("randint", var::make_protect<callable>(cni(randint)));
		runtime_ext.add_var("exception", var::make_protect<callable>(cni(exception)));
		runtime_ext.add_var("hash", var::make_protect<callable>(cni(hash), true));
		runtime_ext.add_var("build", var::make_protect<callable>(cni(build)));
		runtime_ext.add_var("solve", var::make_protect<callable>(cni(solve)));
		runtime_ext.add_var("dynamic_import", var::make_protect<callable>(cni(dynamic_import), true));
		context_ext.add_var("build", var::make_protect<callable>(cni(build)));
		context_ext.add_var("solve", var::make_protect<callable>(cni(solve)));
		context_ext.add_var("dynamic_import", var::make_protect<callable>(cni(dynamic_import), true));
	}
}

class runtime_ext_hvm_impl : public ort::ProxiedObject {
private:
	std::unordered_map<std::string, std::function<ort::Value ()>> fields;

public:
	runtime_ext_hvm_impl() {
	}

	virtual void Init(ort::ObjectProxy& proxy) {
		ort::Runtime& rt = *cs::get_active_runtime();

		proxy.SetStaticField("time", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(runtime_cs_ext::time());
		}).Pin(rt));
		proxy.SetStaticField("std_version", ort::Value::FromInt((long long) cs::std_version));
		proxy.SetStaticField("get_import_path", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromString(
				runtime_cs_ext::get_import_path(),
				rt
			);
		}).Pin(rt));
		proxy.SetStaticField("info", ort::Function::LoadNative([&rt]() {
			runtime_cs_ext::info();
			return ort::Value::Null();
		}).Pin(rt));
		proxy.SetStaticField("delay", ort::Function::LoadNative([&rt]() {
			cs::number t = rt.GetArgument(0).ToF64();
			runtime_cs_ext::delay(t);
			return ort::Value::Null();
		}).Pin(rt));
		proxy.SetStaticField("rand", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(
				runtime_cs_ext::rand(
					rt.GetArgument(0).ToF64(),
					rt.GetArgument(1).ToF64()
				)
			);
		}).Pin(rt));
		proxy.SetStaticField("randint", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromFloat(
				runtime_cs_ext::randint(
					rt.GetArgument(0).ToF64(),
					rt.GetArgument(1).ToF64()
				)
			);
		}).Pin(rt));
		proxy.SetStaticField("exception", ort::Function::LoadNative([&rt]() {
			return runtime_cs_ext::exception(
				rt.GetArgument(0).ToString(rt)
			).to_hvm_value();
		}).Pin(rt));
		proxy.SetStaticField("hash", ort::Function::LoadNative([&rt]() {
			return ort::Value::FromInt(runtime_cs_ext::hash(
				cs::var::from_hvm_value(rt.GetArgument(0))
			));
		}).Pin(rt));
		proxy.Freeze();
	}

	virtual ort::Value GetField(const char *name) {
		auto it = fields.find(name);
		if(it == fields.end()) {
			throw cs::internal_error("Field not found");
		}
		return (it -> second)();
	}
};
