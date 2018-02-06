#pragma once
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
#include <covscript/symbols.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>
#include <hexagon/ort.h>
#include <hexagon/ort_assembly_writer.h>

namespace cs {
	hexagon::ort::Runtime* get_active_runtime();

	class hvm_runtime_guard {
	public:
		hvm_runtime_guard(hexagon::ort::Runtime *rt);
		~hvm_runtime_guard();
	};

	class domain_manager {
		std::deque<spp::sparse_hash_set<string>> m_set;
		std::deque<domain_t> m_data;
	public:
		domain_manager()
		{
			m_set.emplace_front();
			m_data.emplace_front(std::make_shared<spp::sparse_hash_map<string, var >>());
		}

		domain_manager(const domain_manager &) = delete;

		~domain_manager() = default;

		void add_set()
		{
			m_set.emplace_front();
		}

		void add_domain()
		{
			m_data.emplace_front(std::make_shared<spp::sparse_hash_map<string, var >>());
		}

		domain_t &get_domain()
		{
			return m_data.front();
		}

		domain_t &get_global()
		{
			return m_data.back();
		}

		void remove_set()
		{
			m_set.pop_front();
		}

		void remove_domain()
		{
			m_data.pop_front();
		}

		void clear_set()
		{
			m_set.front().clear();
		}

		void clear_domain()
		{
			m_data.front()->clear();
		}

		bool exist_record(const string &name)
		{
			return m_set.front().count(name) > 0;
		}

		bool exist_record_in_struct(const string &name)
		{
			for (auto &set:m_set) {
				if (set.count("__PRAGMA_CS_STRUCT_DEFINITION__") > 0)
					return set.count(name) > 0;
			}
			return false;
		}

		bool var_exist(const string &name)
		{
			for (auto &domain:m_data)
				if (domain->count(name) > 0)
					return true;
			return false;
		}

		bool var_exist_current(const string &name)
		{
			return m_data.front()->count(name) > 0;
		}

		bool var_exist_global(const string &name)
		{
			return m_data.back()->count(name) > 0;
		}

		var &get_var(const string &name)
		{
			for (auto &domain:m_data)
				if (domain->count(name) > 0)
					return (*domain)[name];
			throw syntax_error("Use of undefined variable \"" + name + "\".");
		}

		var &get_var_current(const string &name)
		{
			if (m_data.front()->count(name) > 0)
				return (*m_data.front())[name];
			throw syntax_error("Use of undefined variable \"" + name + "\" in current domain.");
		}

		var &get_var_global(const string &name)
		{
			if (m_data.back()->count(name) > 0)
				return (*m_data.back())[name];
			throw syntax_error("Use of undefined variable \"" + name + "\" in global domain.");
		}

		void add_record(const string &name)
		{
			if (exist_record(name))
				throw syntax_error("Redefinition of variable \"" + name + "\".");
			else
				m_set.front().emplace(name);
		}

		void mark_set_as_struct()
		{
			add_record("__PRAGMA_CS_STRUCT_DEFINITION__");
		}

		void add_var(const string &name, const var &var)
		{
			if (var_exist_current(name))
				throw syntax_error("Target domain exist variable \"" + name + "\".");
			else
				m_data.front()->emplace(name, var);
		}

		void add_var_global(const string &name, const var &var)
		{
			if (var_exist_global(name))
				throw syntax_error("Target domain exist variable \"" + name + "\".");
			else
				m_data.back()->emplace(name, var);
		}

		void add_buildin_var(const string &name, const var &var)
		{
			add_record(name);
			add_var_global(name, var);
		}

		void add_struct(const std::string &name, const struct_builder &builder)
		{
			add_var(name, var::make_protect<type>(builder, builder.get_hash()));
		}

		void add_type(const std::string &name, const std::function<var()> &func, std::size_t hash)
		{
			add_var(name, var::make_protect<type>(func, hash));
		}

		void add_type(const std::string &name, const std::function<var()> &func, std::size_t hash, extension_t ext)
		{
			add_var(name, var::make_protect<type>(func, hash, ext));
		}

		void add_buildin_type(const std::string &name, const std::function<var()> &func, std::size_t hash)
		{
			add_record(name);
			add_var(name, var::make_protect<type>(func, hash));
		}

		void
		add_buildin_type(const std::string &name, const std::function<var()> &func, std::size_t hash, extension_t ext)
		{
			add_record(name);
			add_var(name, var::make_protect<type>(func, hash, ext));
		}

		void involve_domain(const domain_t &domain)
		{
			for (auto &it:*domain)
				add_var(it.first, it.second);
		}
	};

	class function_builder;

	class builder_var_scope {
	public:
		function_builder *builder;
		builder_var_scope(function_builder *b);
		~builder_var_scope();
	};

	class var_scope_impl {
	public:
		std::unordered_map<std::string, int> locals;
	};

	class global_registry : public hexagon::ort::ProxiedObject {
	public:
		std::unordered_map<std::string, std::pair<hexagon::ort::Value, bool /* escalated */>> globals;

		void add(const std::string& k, const hexagon::ort::Value& v, bool escalated = false) {
			globals.insert(std::make_pair(k, std::make_pair(v, escalated)));
		}

		std::unordered_map<std::string, hexagon::ort::Value> get_escalated() {
			std::unordered_map<std::string, hexagon::ort::Value> ret;

			for(auto& p : globals) {
				if(p.second.second) {
					ret.insert(std::make_pair(p.first, p.second.first));
				}
			}

			return ret;
		}

		virtual void Init(hexagon::ort::ObjectProxy& proxy) override {
			using namespace hexagon;

			for(auto& p : globals) {
				proxy.SetStaticField(p.first, p.second.first);
			}
		}
	};

	class function_builder {
	public:
		function_builder *parent;
		std::unordered_map<std::string, std::unique_ptr<function_builder>> children;
		std::vector<std::unique_ptr<hexagon::assembly_writer::BasicBlockWriter>> blocks;
		std::unordered_map<std::string, var> external_vars;
		std::vector<var_scope_impl> vars;
		int next_local_id;
		std::vector<std::string> arg_names;
		std::vector<std::pair<int, int>> loop_control_target_blocks; // (continue, break)
		int current;

		function_builder(const function_builder& other) = delete;
		function_builder(function_builder&& other) = delete;

		function_builder() {
			parent = nullptr;

			vars.push_back(var_scope_impl());
			next_local_id = 0;

			// Initializations
			blocks.push_back(std::unique_ptr<hexagon::assembly_writer::BasicBlockWriter>(new hexagon::assembly_writer::BasicBlockWriter()));

			// The first block for `real` code
			blocks.push_back(std::unique_ptr<hexagon::assembly_writer::BasicBlockWriter>(new hexagon::assembly_writer::BasicBlockWriter()));
			current = 1;
		}

		hexagon::assembly_writer::BasicBlockWriter& get_current() {
			return *blocks.at(current);
		}

		function_builder& create_child(const std::string& name) {
			std::unique_ptr<function_builder> child = std::unique_ptr<function_builder>(new function_builder());
			child -> parent = this;

			function_builder& ref = *child;
			children[name] = std::move(child);
			return ref;
		}

		void clear_arguments() {
			arg_names.clear();
		}

		void add_argument(const std::string& name) {
			arg_names.push_back(name);
		}

		void push_loop_control_info(int continue_block, int break_block) {
			loop_control_target_blocks.push_back(std::make_pair(continue_block, break_block));
		}

		void pop_loop_control_info() {
			loop_control_target_blocks.pop_back();
		}

		std::pair<int, int> get_loop_control_info() {
			if(loop_control_target_blocks.size() == 0) {
				throw syntax_error("Invalid break statement");
			}
			return loop_control_target_blocks[loop_control_target_blocks.size() - 1];
		}

		// expected stack state: ... args* target
		void complete_call(int n_args) {
			using namespace hexagon::assembly_writer;

			auto& current = get_current();

			if(current.opcodes.size() == 0) {
				throw internal_error("No opcodes");
			}

			BytecodeOp& last = current.opcodes[current.opcodes.size() - 1];
			if(last.name == "GetField") {
				// original: ... key obj -> ... field
				// expected: ... key this obj -> ... ret
				last = BytecodeOp("LoadNull");

				// last is NOT safe to use any more after this!
				current
					.Write(BytecodeOp("Rotate2"))
					.Write(BytecodeOp("CallField", Operand::I64(n_args)));
			} else {
				current
					.Write(BytecodeOp("LoadNull"))
					.Write(BytecodeOp("Rotate2"))
					.Write(BytecodeOp("Call", Operand::I64(n_args)));
			}
		}

		void transform_last_op_to_modify(const std::function<void ()>& modifier) {
			using namespace hexagon::assembly_writer;
			
			auto& current = get_current();

			if(current.opcodes.size() == 0) {
				throw internal_error("No opcodes");
			}

			int last_id = current.opcodes.size() - 1;
			BytecodeOp last = current.opcodes[last_id];

			if(last.name == "GetLocal") {
				// original: ... -> ... [b] (Pushes the value onto stack)
				// new: ... -> ... (Moves the value on stack to local)
				modifier();

				Operand local_id = last.operands.at(0);
				current.opcodes.push_back(BytecodeOp("SetLocal", local_id));
			} else if(last.name == "GetArrayElement") {
				// original: ... a key obj -> a [b]
				// new: ... a key obj -> ... a
				current.opcodes[last_id] = BytecodeOp("Rotate2"); // (a, obj, key)
				current.opcodes.push_back(BytecodeOp("Dup")); // (a, obj, key, key)
				current.opcodes.push_back(BytecodeOp("Rotate3")); // (a, key, key, obj)
				current.opcodes.push_back(BytecodeOp("Dup")); // (a, key, key, obj, obj)
				current.opcodes.push_back(BytecodeOp("Rotate3")); // (a, key, obj, obj, key)
				current.opcodes.push_back(BytecodeOp("Rotate2")); // (a, key, obj, key, obj)
				current.opcodes.push_back(last); // (a, key, obj, item)

				modifier(); // (a, key, obj, v)

				current.opcodes.push_back(BytecodeOp("Rotate3")); // (a, obj, v, key)
				current.opcodes.push_back(BytecodeOp("Rotate3")); // (a, v, key, obj)
				current.opcodes.push_back(BytecodeOp("SetArrayElement")); // (a)
			} else if(last.name == "GetField") {
				// original: ... a key obj -> a [b]
				// new: ... a key obj -> ... a
				current.opcodes[last_id] = BytecodeOp("Rotate2"); // (a, obj, key)
				current.opcodes.push_back(BytecodeOp("Dup")); // (a, obj, key, key)
				current.opcodes.push_back(BytecodeOp("Rotate3")); // (a, key, key, obj)
				current.opcodes.push_back(BytecodeOp("Dup")); // (a, key, key, obj, obj)
				current.opcodes.push_back(BytecodeOp("Rotate3")); // (a, key, obj, obj, key)
				current.opcodes.push_back(BytecodeOp("Rotate2")); // (a, key, obj, key, obj)
				current.opcodes.push_back(last); // (a, key, obj, item)

				modifier(); // (a, key, obj, v)

				current.opcodes.push_back(BytecodeOp("Rotate3")); // (a, obj, v, key)
				current.opcodes.push_back(BytecodeOp("Rotate3")); // (a, v, key, obj)
				current.opcodes.push_back(BytecodeOp("SetField")); // (a)
			} else {
				throw internal_error(std::string("Transformation not implemented: ") + last.name);
			}
		}

		void transform_last_op_to_set() {
			using namespace hexagon::assembly_writer;

			auto& current = get_current();

			if(current.opcodes.size() == 0) {
				throw internal_error("No opcodes");
			}

			BytecodeOp& last = current.opcodes[current.opcodes.size() - 1];
			if(last.name == "GetLocal") {
				// original: ... a -> ... a [b] (Pushes the value onto stack)
				// new: ... a -> ... (Moves the value on stack to local)
				Operand local_id = last.operands.at(0);
				last = BytecodeOp("SetLocal", local_id);
			} else if(last.name == "GetArrayElement") {
				// original: ... a id arr -> ... a [b]
				// new: ... a id arr -> ...
				last = BytecodeOp("SetArrayElement");
			} else if(last.name == "GetField") {
				// original: ... a key obj -> a [b]
				// new: ... a key obj -> ...
				last = BytecodeOp("SetField");
			} else {
				throw internal_error(std::string("Transformation not implemented: ") + last.name);
			}
		}

		void write_get_from_global_registry() {
			using namespace hexagon::assembly_writer;

			// pops: key
			// pushes: element

			get_current()
				.Write(BytecodeOp("LoadString", Operand::String("__global_registry")))
				.Write(BytecodeOp("LoadThis"))
				.Write(BytecodeOp("GetField"))
				.Write(BytecodeOp("GetField"));
		}

		void map_arg_names() {
			for(auto& name : arg_names) {
				map_local(name);
			}
		}

		hexagon::ort::Function build(hexagon::ort::Runtime& rt, global_registry& registry, bool debug = false) {
			using namespace hexagon;
			using namespace hexagon::assembly_writer;

			for(auto& child : children) {
				hexagon::ort::Function cf = child.second -> build(rt, registry, debug);
				registry.add(child.first, cf.Pin(rt), true);
			}

			for(auto& v : external_vars) {
				registry.add(v.first, v.second.to_hvm_value(), false);
			}

			auto& init_blk = *blocks[0];
			init_blk.Clear();
			init_blk.Write(BytecodeOp("InitLocal", Operand::I64(next_local_id)));
			for(int i = 0; i < arg_names.size(); i++) {
				init_blk
					.Write(BytecodeOp("GetArgument", Operand::I64(i)))
					.Write(BytecodeOp("SetLocal", Operand::I64(
						map_local(arg_names[i])
					)));
			}
			init_blk.Write(BytecodeOp("Branch", Operand::I64(1)));

			FunctionWriter fwriter([](
				std::vector<BasicBlockWriter>& blocks
			) {
				for(auto& blk : blocks) {
					std::vector<BytecodeOp> new_ops;
					for(auto& op : blk.opcodes) {
						if(op.name == "GetArrayElement") {
							// pops: array, index
							// pushes: element

							new_ops.push_back(BytecodeOp("LoadString", Operand::String("__get__")));
							new_ops.push_back(BytecodeOp("LoadNull"));

							// Rotate: (array, method_name, this) => (method_name, this, array)
							new_ops.push_back(BytecodeOp("Rotate3"));

							new_ops.push_back(BytecodeOp("CallField", Operand::I64(1)));
						} else if(op.name == "SetArrayElement") {
							// pops: array, index, value
							// pushes nothing

							new_ops.push_back(BytecodeOp("LoadString", Operand::String("__set__")));
							new_ops.push_back(BytecodeOp("LoadNull"));

							// Rotate: (array, method_name, this) => (method_name, this, array)
							new_ops.push_back(BytecodeOp("Rotate3"));

							new_ops.push_back(BytecodeOp("CallField", Operand::I64(2)));
							new_ops.push_back(BytecodeOp("Pop"));
						} else {
							new_ops.push_back(op);
						}
					}
					blk.opcodes = new_ops;
				}
			});
			for(auto& blk : blocks) {
				fwriter.Write(*blk);
			}

			if(debug) {
				std::cerr << fwriter.ToJson() << std::endl;
			}

			auto target_fn = fwriter.Build();
			target_fn.EnableOptimization();

			return target_fn;
		}

		void terminate_current() {
			blocks.push_back(std::unique_ptr<hexagon::assembly_writer::BasicBlockWriter>(new hexagon::assembly_writer::BasicBlockWriter()));
			current++;
		}

		int current_id() const {
			return current;
		}

		bool try_map_local(const std::string& name, int& out) {
			for(auto it = vars.rbegin(); it != vars.rend(); it++) {
				auto& locals = it -> locals;

				if(locals.find(name) != locals.end()) {
					out = locals[name];
					return true;
				}
			}
			return false;
		}

		int map_local(const std::string& name) {
			int id = -1;
			if(try_map_local(name, id)) {
				return id;
			}

			auto last = vars.rbegin();
			int new_id = next_local_id++;

			last -> locals[name] = new_id;
			return new_id;
		}

		int anonymous_local() {
			return next_local_id++;
		}
	};

	builder_var_scope::builder_var_scope(function_builder *b) {
		builder = b;
		builder -> vars.push_back(var_scope_impl());
	}

	builder_var_scope::~builder_var_scope() {
		builder -> vars.pop_back();
	}

	class runtime_type {
	public:
		domain_manager storage;

		var parse_add(const var &, const var &);

		var parse_addasi(var, const var &);

		var parse_sub(const var &, const var &);

		var parse_subasi(var, const var &);

		var parse_minus(const var &);

		var parse_mul(const var &, const var &);

		var parse_mulasi(var, const var &);

		var parse_escape(const var &);

		var parse_div(const var &, const var &);

		var parse_divasi(var, const var &);

		var parse_mod(const var &, const var &);

		var parse_modasi(var, const var &);

		var parse_pow(const var &, const var &);

		var parse_powasi(var, const var &);

		var parse_dot(const var &, token_base *);

		var parse_arraw(const var &, token_base *);

		var parse_typeid(const var &);

		var parse_new(const var &);

		var parse_gcnew(const var &);

		var parse_und(const var &, const var &);

		var parse_abo(const var &, const var &);

		var parse_ueq(const var &, const var &);

		var parse_aeq(const var &, const var &);

		var parse_asi(var, const var &);

		var parse_choice(const var &, const cov::tree<token_base *>::iterator &);

		var parse_pair(const var &, const var &);

		var parse_equ(const var &, const var &);

		var parse_neq(const var &, const var &);

		var parse_and(const var &, const var &);

		var parse_or(const var &, const var &);

		var parse_not(const var &);

		var parse_inc(var, var);

		var parse_dec(var, var);

		var parse_fcall(const var &, token_base *);

		var parse_access(var, const var &);

		var parse_expr(const cov::tree<token_base *>::iterator &);

		void generate_code_from_expr(const cov::tree<token_base *>::iterator &it, function_builder& builder);
	};
}
