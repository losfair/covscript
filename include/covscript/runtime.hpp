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
#include <hexagon/ort.h>
#include <hexagon/ort_assembly_writer.h>

namespace cs {
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

		bool exsist_record(const string &name)
		{
			return m_set.front().count(name) > 0;
		}

		bool exsist_record_in_struct(const string &name)
		{
			for (auto &set:m_set) {
				if (set.count("__PRAGMA_CS_STRUCT_DEFINITION__") > 0)
					return set.count(name) > 0;
			}
			return false;
		}

		bool var_exsist(const string &name)
		{
			for (auto &domain:m_data)
				if (domain->count(name) > 0)
					return true;
			return false;
		}

		bool var_exsist_current(const string &name)
		{
			return m_data.front()->count(name) > 0;
		}

		bool var_exsist_global(const string &name)
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
			if (exsist_record(name))
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
			if (var_exsist_current(name))
				throw syntax_error("Target domain exist variable \"" + name + "\".");
			else
				m_data.front()->emplace(name, var);
		}

		void add_var_global(const string &name, const var &var)
		{
			if (var_exsist_global(name))
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

	class function_builder {
	public:
		std::vector<std::unique_ptr<hexagon::assembly_writer::BasicBlockWriter>> blocks;
		std::unordered_map<std::string, int> locals;
		std::vector<std::string> locals_reverse;
		int current;

		function_builder(const function_builder& other) = delete;

		function_builder() {
			// Initializations
			blocks.push_back(std::unique_ptr<hexagon::assembly_writer::BasicBlockWriter>(new hexagon::assembly_writer::BasicBlockWriter()));

			// The first block for `real` code
			blocks.push_back(std::unique_ptr<hexagon::assembly_writer::BasicBlockWriter>(new hexagon::assembly_writer::BasicBlockWriter()));
			current = 1;
		}

		hexagon::assembly_writer::BasicBlockWriter& get_current() {
			return *blocks.at(current);
		}

		hexagon::assembly_writer::FunctionWriter build() {
			using namespace hexagon::assembly_writer;

			auto& init_blk = *blocks[0];
			init_blk.Clear();
			init_blk.Write(BytecodeOp("InitLocal", Operand::I64(locals_reverse.size())));
			init_blk.Write(BytecodeOp("Branch", Operand::I64(1)));

			hexagon::assembly_writer::FunctionWriter fwriter;
			for(auto& blk : blocks) {
				fwriter.Write(*blk);
			}

			return fwriter;
		}

		void terminate_current() {
			blocks.push_back(std::unique_ptr<hexagon::assembly_writer::BasicBlockWriter>(new hexagon::assembly_writer::BasicBlockWriter()));
			current++;
		}

		int current_id() const {
			return current;
		}

		int map_local(const std::string& name) {
			if(locals.find(name) == locals.end()) {
				int new_id = locals_reverse.size();
				locals_reverse.push_back(name);
				locals[name] = new_id;
				return new_id;
			} else {
				return locals[name];
			}
		}
	};

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
