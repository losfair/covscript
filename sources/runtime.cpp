/*
* Covariant Script Runtime
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
#include <covscript/runtime.hpp>
#include <covscript/unique_id.hpp>
#include <string>
#include <vector>

namespace cs {
	thread_local static std::vector<hexagon::ort::Runtime *> active_runtimes;

	hvm_runtime_guard::hvm_runtime_guard(hexagon::ort::Runtime *rt) {
		active_runtimes.push_back(rt);
	}

	hvm_runtime_guard::~hvm_runtime_guard() {
		active_runtimes.pop_back();
	}

	hexagon::ort::Runtime * get_active_runtime() {
		if(active_runtimes.size() == 0) {
			throw internal_error("No active runtime(s)");
		}
		return active_runtimes[active_runtimes.size() - 1];
	}

	var runtime_type::parse_add(const var &a, const var &b)
	{
		if (a.type() == typeid(number) && b.type() == typeid(number))
			return a.const_val<number>() + b.const_val<number>();
		else if (a.type() == typeid(string))
			return var::make<std::string>(a.const_val<string>() + b.to_string());
		else
			throw syntax_error("Unsupported operator operations(Add).");
	}

	var runtime_type::parse_addasi(var a, const var &b)
	{
		a.swap(parse_add(a, b), true);
		return a;
	}

	var runtime_type::parse_sub(const var &a, const var &b)
	{
		if (a.type() == typeid(number) && b.type() == typeid(number))
			return a.const_val<number>() - b.const_val<number>();
		else
			throw syntax_error("Unsupported operator operations(Sub).");
	}

	var runtime_type::parse_subasi(var a, const var &b)
	{
		a.swap(parse_sub(a, b), true);
		return a;
	}

	var runtime_type::parse_minus(const var &b)
	{
		if (b.type() == typeid(number))
			return -b.const_val<number>();
		else
			throw syntax_error("Unsupported operator operations(Minus).");
	}

	var runtime_type::parse_mul(const var &a, const var &b)
	{
		if (a.type() == typeid(number) && b.type() == typeid(number))
			return a.const_val<number>() * b.const_val<number>();
		else
			throw syntax_error("Unsupported operator operations(Mul).");
	}

	var runtime_type::parse_mulasi(var a, const var &b)
	{
		a.swap(parse_mul(a, b), true);
		return a;
	}

	var runtime_type::parse_escape(const var &b)
	{
		if (b.type() == typeid(pointer)) {
			const pointer &ptr = b.const_val<pointer>();
			if (ptr.data.usable())
				return ptr.data;
			else
				throw syntax_error("Escape from null pointer.");
		}
		else
			throw syntax_error("Unsupported operator operations(Escape).");
	}

	var runtime_type::parse_div(const var &a, const var &b)
	{
		if (a.type() == typeid(number) && b.type() == typeid(number))
			return a.const_val<number>() / b.const_val<number>();
		else
			throw syntax_error("Unsupported operator operations(Div).");
	}

	var runtime_type::parse_divasi(var a, const var &b)
	{
		a.swap(parse_div(a, b), true);
		return a;
	}

	var runtime_type::parse_mod(const var &a, const var &b)
	{
		if (a.type() == typeid(number) && b.type() == typeid(number))
			return number(std::fmod(a.const_val<number>(), b.const_val<number>()));
		else
			throw syntax_error("Unsupported operator operations(Mod).");
	}

	var runtime_type::parse_modasi(var a, const var &b)
	{
		a.swap(parse_mod(a, b), true);
		return a;
	}

	var runtime_type::parse_pow(const var &a, const var &b)
	{
		if (a.type() == typeid(number) && b.type() == typeid(number))
			return number(std::pow(a.const_val<number>(), b.const_val<number>()));
		else
			throw syntax_error("Unsupported operator operations(Pow).");
	}

	var runtime_type::parse_powasi(var a, const var &b)
	{
		a.swap(parse_pow(a, b), true);
		return a;
	}

	var runtime_type::parse_dot(const var &a, token_base *b)
	{
		if (a.type() == typeid(constant_values)) {
			switch (a.const_val<constant_values>()) {
			case constant_values::global_namespace:
				return storage.get_var_global(static_cast<token_id *>(b)->get_id());
				break;
			case constant_values::current_namespace:
				return storage.get_var_current(static_cast<token_id *>(b)->get_id());
				break;
			default:
				throw syntax_error("Unsupported operator operations(Dot).");
				break;
			}
		}
		else if (a.type() == typeid(extension_t))
			return a.val<extension_t>(true)->get_var(static_cast<token_id *>(b)->get_id());
		else if (a.type() == typeid(type))
			return a.val<type>(true).get_var(static_cast<token_id *>(b)->get_id());
		else if (a.type() == typeid(structure)) {
			var &val = a.val<structure>(true).get_var(static_cast<token_id *>(b)->get_id());
			if (val.type() == typeid(callable) && val.const_val<callable>().is_member_fn())
				return var::make_protect<object_method>(a, val);
			else
				return val;
		}
		else {
			var &val = a.get_ext()->get_var(static_cast<token_id *>(b)->get_id());
			if (val.type() == typeid(callable))
				return var::make_protect<object_method>(a, val, val.const_val<callable>().is_constant());
			else
				return val;
		}
	}

	var runtime_type::parse_arraw(const var &a, token_base *b)
	{
		if (a.type() == typeid(pointer))
			return parse_dot(a.const_val<pointer>().data, b);
		else
			throw syntax_error("Unsupported operator operations(Arraw).");
	}

	var runtime_type::parse_typeid(const var &b)
	{
		if (b.type() == typeid(type))
			return b.const_val<type>().id;
		else if (b.type() == typeid(structure))
			return b.const_val<structure>().get_hash();
		else
			return cs_impl::hash<std::string>(b.type().name());
	}

	var runtime_type::parse_new(const var &b)
	{
		if (b.type() == typeid(type))
			return b.const_val<type>().constructor();
		else
			throw syntax_error("Unsupported operator operations(New).");
	}

	var runtime_type::parse_gcnew(const var &b)
	{
		if (b.type() == typeid(type))
			return var::make<pointer>(b.const_val<type>().constructor());
		else
			throw syntax_error("Unsupported operator operations(GcNew).");
	}

	var runtime_type::parse_und(const var &a, const var &b)
	{
		if (a.type() == typeid(number) && b.type() == typeid(number))
			return boolean(a.const_val<number>() < b.const_val<number>());
		else
			throw syntax_error("Unsupported operator operations(Und).");
	}

	var runtime_type::parse_abo(const var &a, const var &b)
	{
		if (a.type() == typeid(number) && b.type() == typeid(number))
			return boolean(a.const_val<number>() > b.const_val<number>());
		else
			throw syntax_error("Unsupported operator operations(Abo).");
	}

	var runtime_type::parse_ueq(const var &a, const var &b)
	{
		if (a.type() == typeid(number) && b.type() == typeid(number))
			return boolean(a.const_val<number>() <= b.const_val<number>());
		else
			throw syntax_error("Unsupported operator operations(Ueq).");
	}

	var runtime_type::parse_aeq(const var &a, const var &b)
	{
		if (a.type() == typeid(number) && b.type() == typeid(number))
			return boolean(a.const_val<number>() >= b.const_val<number>());
		else
			throw syntax_error("Unsupported operator operations(Aeq).");
	}

	var runtime_type::parse_asi(var a, const var &b)
	{
		a.swap(copy(b), true);
		return a;
	}

	var runtime_type::parse_choice(const var &a, const cov::tree<token_base *>::iterator &b)
	{
		if (a.type() == typeid(boolean)) {
			if (a.const_val<boolean>())
				return parse_expr(b.left());
			else
				return parse_expr(b.right());
		}
		else
			throw syntax_error("Unsupported operator operations(Choice).");
	}

	var runtime_type::parse_pair(const var &a, const var &b)
	{
		if (a.type() != typeid(pair) && b.type() != typeid(pair))
			return var::make<pair>(copy(a), copy(b));
		else
			throw syntax_error("Unsupported operator operations(Pair).");
	}

	var runtime_type::parse_equ(const var &a, const var &b)
	{
		return boolean(a.compare(b));
	}

	var runtime_type::parse_neq(const var &a, const var &b)
	{
		return boolean(!a.compare(b));
	}

	var runtime_type::parse_and(const var &a, const var &b)
	{
		if (a.type() == typeid(boolean) && b.type() == typeid(boolean))
			return boolean(a.const_val<boolean>() && b.const_val<boolean>());
		else
			throw syntax_error("Unsupported operator operations(And).");
	}

	var runtime_type::parse_or(const var &a, const var &b)
	{
		if (a.type() == typeid(boolean) && b.type() == typeid(boolean))
			return boolean(a.const_val<boolean>() || b.const_val<boolean>());
		else
			throw syntax_error("Unsupported operator operations(Or).");
	}

	var runtime_type::parse_not(const var &b)
	{
		if (b.type() == typeid(boolean))
			return boolean(!b.const_val<boolean>());
		else
			throw syntax_error("Unsupported operator operations(Not).");
	}

	var runtime_type::parse_inc(var a, var b)
	{
		if (a.usable()) {
			if (b.usable())
				throw syntax_error("Unsupported operator operations(Inc).");
			else
				return a.val<number>(true)++;
		}
		else {
			if (!b.usable())
				throw syntax_error("Unsupported operator operations(Inc).");
			else
				return ++b.val<number>(true);
		}
	}

	var runtime_type::parse_dec(var a, var b)
	{
		if (a.usable()) {
			if (b.usable())
				throw syntax_error("Unsupported operator operations(Dec).");
			else
				return a.val<number>(true)--;
		}
		else {
			if (!b.usable())
				throw syntax_error("Unsupported operator operations(Dec).");
			else
				return --b.val<number>(true);
		}
	}

	var runtime_type::parse_fcall(const var &a, token_base *b)
	{
		if (a.type() == typeid(callable)) {
			vector args;
			args.reserve(static_cast<token_arglist *>(b)->get_arglist().size());
			for (auto &tree:static_cast<token_arglist *>(b)->get_arglist())
				args.push_back(parse_expr(tree.root()));
			return a.const_val<callable>().call(args);
		}
		else if (a.type() == typeid(object_method)) {
			const object_method &om = a.const_val<object_method>();
			vector args{om.object};
			args.reserve(static_cast<token_arglist *>(b)->get_arglist().size());
			for (auto &tree:static_cast<token_arglist *>(b)->get_arglist())
				args.push_back(parse_expr(tree.root()));
			return om.callable.const_val<callable>().call(args);
		}
		else
			throw syntax_error("Unsupported operator operations(Fcall).");
	}

	var runtime_type::parse_access(var a, const var &b)
	{
		if (a.type() == typeid(array)) {
			if (b.type() != typeid(number))
				throw syntax_error("Index must be a number.");
			if (b.const_val<number>() < 0)
				throw syntax_error("Index must above zero.");
			const array &carr = a.const_val<array>();
			std::size_t posit = b.const_val<number>();
			if (posit >= carr.size()) {
				array & arr = a.val<array>(true);
				for (std::size_t i = posit - arr.size() + 1; i > 0; --i)
					arr.emplace_back(number(0));
			}
			return carr.at(posit);
		}
		else if (a.type() == typeid(hash_map)) {
			const hash_map &cmap = a.const_val<hash_map>();
			if (cmap.count(b) == 0)
				a.val<hash_map>(true).emplace(copy(b), number(0));
			return cmap.at(b);
		}
		else if (a.type() == typeid(string)) {
			if (b.type() != typeid(number))
				throw syntax_error("Index must be a number.");
			return a.const_val<string>().at(b.const_val<number>());
		}
		else
			throw syntax_error("Access non-array or string object.");
	}

	// pushes exactly one value
	static void build_value_load(function_builder& builder, const var& v) {
		using namespace hexagon::assembly_writer;

		if(v.type() == typeid(int) || v.type() == typeid(long) || v.type() == typeid(long long) || v.type() == typeid(char)) {
			auto inner = v.to_integer();
			builder.get_current().Write(BytecodeOp("LoadInt", Operand::I64(inner)));
		} else if(v.type() == typeid(float)) {
			builder.get_current().Write(BytecodeOp("LoadFloat", Operand::F64(v.const_val<float>())));
		} else if(v.type() == typeid(double)) {
			builder.get_current().Write(BytecodeOp("LoadFloat", Operand::F64(v.const_val<double>())));
		} else if(v.type() == typeid(long double)) {
			builder.get_current().Write(BytecodeOp("LoadFloat", Operand::F64(v.const_val<long double>())));
		} else if(v.type() == typeid(string)) {
			auto inner = v.to_string();
			builder.get_current().Write(BytecodeOp("LoadString", Operand::String(inner)));
		} else if(v.type() == typeid(array)) {
			array arr;
			for (const var& elem : v.const_val<array>()) {
				arr.push_back(elem);
			}
			std::string v_id = cs_impl::unique_id::random_string(16);

			builder.external_vars.insert(std::make_pair(v_id, var::make<array>(std::move(arr))));
			builder.get_current().Write(BytecodeOp("LoadString", Operand::String(v_id)));
			builder.write_get_from_global_registry();
		} else if(v.type() == typeid(pointer)) {
			pointer p = v.const_val<pointer>();
			if(p.data.usable()) {
				throw syntax_error("Only null pointers are supported");
			}
			builder.get_current().Write(BytecodeOp("LoadNull"));
		} else if(v.type() == typeid(cs::boolean)) {
			bool b = v.const_val<cs::boolean>();
			builder.get_current().Write(BytecodeOp("LoadBool", Operand::Bool(b)));
		} else if(v.type() == typeid(cs::callable)) {
			const cs::callable& callable = v.const_val<cs::callable>();
			throw internal_error("callable");
		} else {
			throw internal_error(std::string("Unsupported value type: ") + v.get_type_name());
		}
	}

	// This should push **exactly** one value onto the stack.
	void runtime_type::generate_code_from_expr(const cov::tree<token_base *>::iterator &it, function_builder& builder) {
		using namespace hexagon::assembly_writer;
	
		if (!it.usable())
			throw internal_error("The expression tree is not available.");
		token_base *token = it.data();
		if (token == nullptr) {
			builder.get_current().Write(BytecodeOp("LoadNull"));
			return;
		}

		switch(token -> get_type()) {
		case token_types::id: {
			int local_id = -1;
			bool found = builder.try_map_local(
				static_cast<token_id *>(token)->get_id(),
				local_id
			);
			if(!found) {
				builder.get_current()
					.Write(BytecodeOp("LoadString", Operand::String(static_cast<token_id *>(token)->get_id())))
					.Write(BytecodeOp("LoadThis"))
					.Write(BytecodeOp("GetField"));
			} else {
				builder.get_current().Write(BytecodeOp("GetLocal", Operand::I64(local_id)));
			}
			return;
		}
		case token_types::value:
			build_value_load(builder, static_cast<token_value *>(token)->get_value());
			return;
		case token_types::expr:
			generate_code_from_expr(static_cast<token_expr *>(token)->get_tree().root(), builder);
			return;
		case token_types::array:
			builder.get_current()
				.Write(BytecodeOp("LoadString", Operand::String("__new__")))
				.Write(BytecodeOp("LoadNull"))
				.Write(BytecodeOp("LoadString", Operand::String("array")))
				.Write(BytecodeOp("LoadThis"))
				.Write(BytecodeOp("GetField"))
				.Write(BytecodeOp("CallField", Operand::I64(0)));

			for (auto &tree:static_cast<token_array *>(token)->get_array()) {
				builder.get_current().Write(BytecodeOp("Dup"));
				generate_code_from_expr(tree.root(), builder);
				builder.get_current()
					.Write(BytecodeOp("Rotate2"))
					.Write(BytecodeOp("LoadString", Operand::String("push_back")))
					.Write(BytecodeOp("Rotate2"))
					.Write(BytecodeOp("LoadNull"))
					.Write(BytecodeOp("Rotate2"))
					.Write(BytecodeOp("CallField", Operand::I64(1)))
					.Write(BytecodeOp("Pop"));
			}
			return;
		case token_types::signal:
			switch (static_cast<token_signal *>(token)->get_signal()) {
				case signal_types::add_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("Add"));
					break;
				}
				case signal_types::addasi_: {
					int rvalue_id = builder.anonymous_local();
					generate_code_from_expr(it.right(), builder);
					builder.get_current()
						.Write(BytecodeOp("SetLocal", Operand::I64(rvalue_id)));

					generate_code_from_expr(it.left(), builder);

					int result_id = builder.anonymous_local();

					builder.transform_last_op_to_modify([&]() {
						builder.get_current()
							.Write(BytecodeOp("GetLocal", Operand::I64(rvalue_id)))
							.Write(BytecodeOp("Rotate2"))
							.Write(BytecodeOp("Add"))
							.Write(BytecodeOp("Dup"))
							.Write(BytecodeOp("SetLocal", Operand::I64(result_id)));
					});
					builder.get_current().Write(BytecodeOp("GetLocal", Operand::I64(result_id)));
					break;
				}
				case signal_types::sub_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("Sub"));
					break;
				}
				case signal_types::subasi_: {
					int rvalue_id = builder.anonymous_local();
					generate_code_from_expr(it.right(), builder);
					builder.get_current()
						.Write(BytecodeOp("SetLocal", Operand::I64(rvalue_id)));

					generate_code_from_expr(it.left(), builder);

					int result_id = builder.anonymous_local();

					builder.transform_last_op_to_modify([&]() {
						builder.get_current()
							.Write(BytecodeOp("GetLocal", Operand::I64(rvalue_id)))
							.Write(BytecodeOp("Rotate2"))
							.Write(BytecodeOp("Sub"))
							.Write(BytecodeOp("Dup"))
							.Write(BytecodeOp("SetLocal", Operand::I64(result_id)));
					});
					builder.get_current().Write(BytecodeOp("GetLocal", Operand::I64(result_id)));
					break;
				}
				case signal_types::mul_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("Mul"));
					break;
				}
				case signal_types::mulasi_: {
					int rvalue_id = builder.anonymous_local();
					generate_code_from_expr(it.right(), builder);
					builder.get_current()
						.Write(BytecodeOp("SetLocal", Operand::I64(rvalue_id)));

					generate_code_from_expr(it.left(), builder);

					int result_id = builder.anonymous_local();

					builder.transform_last_op_to_modify([&]() {
						builder.get_current()
							.Write(BytecodeOp("GetLocal", Operand::I64(rvalue_id)))
							.Write(BytecodeOp("Rotate2"))
							.Write(BytecodeOp("Mul"))
							.Write(BytecodeOp("Dup"))
							.Write(BytecodeOp("SetLocal", Operand::I64(result_id)));
					});
					builder.get_current().Write(BytecodeOp("GetLocal", Operand::I64(result_id)));
					break;
				}
				case signal_types::div_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("Div"));
					break;
				}
				case signal_types::divasi_: {
					int rvalue_id = builder.anonymous_local();
					generate_code_from_expr(it.right(), builder);
					builder.get_current()
						.Write(BytecodeOp("SetLocal", Operand::I64(rvalue_id)));

					generate_code_from_expr(it.left(), builder);

					int result_id = builder.anonymous_local();

					builder.transform_last_op_to_modify([&]() {
						builder.get_current()
							.Write(BytecodeOp("GetLocal", Operand::I64(rvalue_id)))
							.Write(BytecodeOp("Rotate2"))
							.Write(BytecodeOp("Div"))
							.Write(BytecodeOp("Dup"))
							.Write(BytecodeOp("SetLocal", Operand::I64(result_id)));
					});
					builder.get_current().Write(BytecodeOp("GetLocal", Operand::I64(result_id)));
					break;
				}
				case signal_types::mod_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("Mod"));
					break;
				}
				case signal_types::modasi_: {
					int rvalue_id = builder.anonymous_local();
					generate_code_from_expr(it.right(), builder);
					builder.get_current()
						.Write(BytecodeOp("SetLocal", Operand::I64(rvalue_id)));

					generate_code_from_expr(it.left(), builder);

					int result_id = builder.anonymous_local();

					builder.transform_last_op_to_modify([&]() {
						builder.get_current()
							.Write(BytecodeOp("GetLocal", Operand::I64(rvalue_id)))
							.Write(BytecodeOp("Rotate2"))
							.Write(BytecodeOp("Mod"))
							.Write(BytecodeOp("Dup"))
							.Write(BytecodeOp("SetLocal", Operand::I64(result_id)));
					});
					builder.get_current().Write(BytecodeOp("GetLocal", Operand::I64(result_id)));
					break;
				}
				case signal_types::pow_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("Pow"));
					break;
				}
				case signal_types::powasi_: {
					int rvalue_id = builder.anonymous_local();
					generate_code_from_expr(it.right(), builder);
					builder.get_current()
						.Write(BytecodeOp("SetLocal", Operand::I64(rvalue_id)));

					generate_code_from_expr(it.left(), builder);

					int result_id = builder.anonymous_local();

					builder.transform_last_op_to_modify([&]() {
						builder.get_current()
							.Write(BytecodeOp("GetLocal", Operand::I64(rvalue_id)))
							.Write(BytecodeOp("Rotate2"))
							.Write(BytecodeOp("Pow"))
							.Write(BytecodeOp("Dup"))
							.Write(BytecodeOp("SetLocal", Operand::I64(result_id)));
					});
					builder.get_current().Write(BytecodeOp("GetLocal", Operand::I64(result_id)));
					break;
				}
				case signal_types::minus_: {
					generate_code_from_expr(it.right(), builder);
					builder.get_current().Write(BytecodeOp("LoadInt", Operand::I64(0)));
					builder.get_current().Write(BytecodeOp("Sub"));
					break;
				}
				case signal_types::inc_: {
					bool is_right = false;

					token_base *left_data = it.left().data();
					token_base *right_data = it.right().data();

					if(left_data && !right_data) {
						is_right = false;
						generate_code_from_expr(it.left(), builder);
					} else if(right_data && !left_data) {
						is_right = true;
						generate_code_from_expr(it.right(), builder);
					} else {
						throw syntax_error("Invalid use of the inc operator");
					}

					int result_id = builder.anonymous_local();

					builder.transform_last_op_to_modify([&]() {
						if(!is_right) {
							builder.get_current()
								.Write(BytecodeOp("Dup"))
								.Write(BytecodeOp("SetLocal", Operand::I64(result_id)));
						}

						builder.get_current()
							.Write(BytecodeOp("LoadInt", Operand::I64(1)))
							.Write(BytecodeOp("Rotate2"))
							.Write(BytecodeOp("IntAdd"));
							

						if(is_right) {
							builder.get_current()
								.Write(BytecodeOp("Dup"))
								.Write(BytecodeOp("SetLocal", Operand::I64(result_id)));
						}
					});
					builder.get_current().Write(BytecodeOp("GetLocal", Operand::I64(result_id)));
					break;
				}
				case signal_types::dec_: {
					bool is_right = false;

					token_base *left_data = it.left().data();
					token_base *right_data = it.right().data();

					if(left_data && !right_data) {
						is_right = false;
						generate_code_from_expr(it.left(), builder);
					} else if(right_data && !left_data) {
						is_right = true;
						generate_code_from_expr(it.right(), builder);
					} else {
						throw syntax_error("Invalid use of the inc operator");
					}

					int result_id = builder.anonymous_local();

					builder.transform_last_op_to_modify([&]() {
						if(!is_right) {
							builder.get_current()
								.Write(BytecodeOp("Dup"))
								.Write(BytecodeOp("SetLocal", Operand::I64(result_id)));
						}

						builder.get_current()
							.Write(BytecodeOp("LoadInt", Operand::I64(1)))
							.Write(BytecodeOp("Rotate2"))
							.Write(BytecodeOp("IntSub"));
							

						if(is_right) {
							builder.get_current()
								.Write(BytecodeOp("Dup"))
								.Write(BytecodeOp("SetLocal", Operand::I64(result_id)));
						}
					});
					builder.get_current().Write(BytecodeOp("GetLocal", Operand::I64(result_id)));
					break;
				}
				case signal_types::asi_: {
					generate_code_from_expr(it.right(), builder);
					builder.get_current().Write(BytecodeOp("Dup"));

					generate_code_from_expr(it.left(), builder);
					builder.transform_last_op_to_set();
					break;
				}
				case signal_types::und_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("TestLt"));
					break;
				}
				case signal_types::abo_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("TestGt"));
					break;
				}
				case signal_types::ueq_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("TestLe"));
					break;
				}
				case signal_types::aeq_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("TestGe"));
					break;
				}
				case signal_types::neq_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("TestNe"));
					break;
				}
				case signal_types::equ_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("TestEq"));
					break;
				}
				case signal_types::and_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("And"));
					break;
				}
				case signal_types::or_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("Or"));
					break;
				}
				case signal_types::not_: {
					generate_code_from_expr(it.left(), builder);
					builder.get_current().Write(BytecodeOp("Not"));
					break;
				}
				case signal_types::access_: {
					generate_code_from_expr(it.right(), builder);
					generate_code_from_expr(it.left(), builder);

					builder.get_current().Write(BytecodeOp("GetArrayElement"));
					break;
				}
				case signal_types::dot_: {
					token_base *right_data = it.right().data();
					std::string field_name = static_cast<token_id *>(right_data)->get_id();
					builder.get_current().Write(BytecodeOp("LoadString", Operand::String(field_name)));

					generate_code_from_expr(it.left(), builder);

					builder.get_current().Write(BytecodeOp("GetField"));

					break;
				}
				case signal_types::fcall_: {
					token_base *args = it.right().data();
					int n_args = static_cast<token_arglist *>(args)->get_arglist().size();
					
					for (auto &tree : static_cast<token_arglist *>(args)->get_arglist()) {
						generate_code_from_expr(tree.root(), builder);
					}
					if(n_args) {
						builder.get_current().Write(BytecodeOp("RotateReverse", Operand::I64(n_args)));
					}

					generate_code_from_expr(it.left(), builder);
					builder.complete_call(n_args);
					break;
				}
				case signal_types::lambda_: {
					token_base *lptr = it.left().data();
					token_base *rptr = it.right().data();

					std::vector<std::string> args;
					for (auto &it:dynamic_cast<token_arglist *>(lptr)->get_arglist()) {
						if (it.root().data() == nullptr)
							throw internal_error("Null pointer accessed.");
						if (it.root().data()->get_type() != token_types::id)
							throw syntax_error("Wrong grammar for function definition.");
						const std::string &str = dynamic_cast<token_id *>(it.root().data())->get_id();
						for (auto &it:args)
							if (it == str)
								throw syntax_error("Redefinition of function argument.");
						args.push_back(str);
					}

					const std::string lambda_name = cs_impl::unique_id::random_string(16);

					{
						function_builder& lambda_builder = builder.create_child(lambda_name);
						for(auto& arg : args) {
							lambda_builder.add_argument(arg);
						}
						lambda_builder.map_arg_names();

						generate_code_from_expr(it.right(), lambda_builder);
						lambda_builder.get_current().Write(BytecodeOp("Return"));
					}

					builder.get_current().Write(BytecodeOp("LoadString", Operand::String(lambda_name)));
					builder.write_get_from_global_registry();

					break;
				}
				case signal_types::new_: {
					builder.get_current()
						.Write(BytecodeOp("LoadString", Operand::String("__new__")))
						.Write(BytecodeOp("LoadNull"));

					generate_code_from_expr(it.right(), builder);
					builder.get_current().Write(BytecodeOp("CallField", Operand::I64(0)));

					break;
				}
				default:
					throw internal_error("Unrecognized signal.");
			}
			return;
		default:
			throw internal_error("Unrecognized expression.");
		}
	}

	var runtime_type::parse_expr(const cov::tree<token_base *>::iterator &it)
	{
		if (!it.usable())
			throw internal_error("The expression tree is not available.");
		token_base *token = it.data();
		if (token == nullptr)
			return var();
		switch (token->get_type()) {
		default:
			break;
		case token_types::id:
			return storage.get_var(static_cast<token_id *>(token)->get_id());
			break;
		case token_types::value:
			return static_cast<token_value *>(token)->get_value();
			break;
		case token_types::expr:
			return parse_expr(static_cast<token_expr *>(token)->get_tree().root());
			break;
		case token_types::array: {
			array arr;
			for (auto &tree:static_cast<token_array *>(token)->get_array())
				arr.push_back(copy(parse_expr(tree.root())));
			return var::make<array>(std::move(arr));
		}
		case token_types::signal: {
			switch (static_cast<token_signal *>(token)->get_signal()) {
			default:
				break;
			case signal_types::add_:
				return parse_add(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::addasi_:
				return parse_addasi(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::sub_:
				return parse_sub(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::subasi_:
				return parse_subasi(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::minus_:
				return parse_minus(parse_expr(it.right()));
				break;
			case signal_types::mul_:
				return parse_mul(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::mulasi_:
				return parse_mulasi(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::escape_:
				return parse_escape(parse_expr(it.right()));
				break;
			case signal_types::div_:
				return parse_div(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::divasi_:
				return parse_divasi(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::mod_:
				return parse_mod(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::modasi_:
				return parse_modasi(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::pow_:
				return parse_pow(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::powasi_:
				return parse_powasi(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::dot_:
				return parse_dot(parse_expr(it.left()), it.right().data());
				break;
			case signal_types::arrow_:
				return parse_arraw(parse_expr(it.left()), it.right().data());
				break;
			case signal_types::typeid_:
				return parse_typeid(parse_expr(it.right()));
				break;
			case signal_types::new_:
				return parse_new(parse_expr(it.right()));
				break;
			case signal_types::gcnew_:
				return parse_gcnew(parse_expr(it.right()));
				break;
			case signal_types::und_:
				return parse_und(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::abo_:
				return parse_abo(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::asi_:
				return parse_asi(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::choice_:
				return parse_choice(parse_expr(it.left()), it.right());
				break;
			case signal_types::pair_:
				return parse_pair(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::equ_:
				return parse_equ(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::ueq_:
				return parse_ueq(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::aeq_:
				return parse_aeq(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::neq_:
				return parse_neq(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::and_:
				return parse_and(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::or_:
				return parse_or(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::not_:
				return parse_not(parse_expr(it.right()));
				break;
			case signal_types::inc_:
				return parse_inc(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::dec_:
				return parse_dec(parse_expr(it.left()), parse_expr(it.right()));
				break;
			case signal_types::fcall_:
				return parse_fcall(parse_expr(it.left()), it.right().data());
				break;
			case signal_types::access_:
				return parse_access(parse_expr(it.left()), parse_expr(it.right()));
				break;
			}
		}
		}
		throw internal_error("Unrecognized expression.");
	}
}
