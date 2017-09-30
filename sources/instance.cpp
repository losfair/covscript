#include "headers/instance.hpp"
#include "headers/statement.hpp"
namespace cs {
	const std::string & statement_base::get_file_path() const noexcept
	{
		return context->get_file_path;
	}

	const std::string & statement_base::get_package_name() const noexcept
	{
		return context->runtime.package_name;
	}

	const std::string & statement_base::get_raw_code() const noexcept
	{
		return context->get_raw_code(line_num);
	}
	void instance::opt_expr(cov::tree<token_base *> &tree, cov::tree<token_base *>::iterator it)
	{
		if (!it.usable())
			return;
		token_base *token = it.data();
		if (token == nullptr)
			return;
		switch (token->get_type()) {
		default:
			break;
		case token_types::id: {
			const std::string &id = static_cast<token_id *>(token)->get_id();
			if (runtime.storage.exsist_record(id)) {
				if (runtime.storage.var_exsist_current(id))
					it.data() = new token_value(runtime.storage.get_var(id));
			}
			else if (runtime.storage.var_exsist(id) && runtime.storage.get_var(id).is_protect())
				it.data() = new token_value(runtime.storage.get_var(id));
			return;
			break;
		}
		case token_types::expr: {
			cov::tree<token_base *> &t = static_cast<token_expr *>(it.data())->get_tree();
			optimize_expression(t);
			if (optimizable(t.root())) {
				it.data() = t.root().data();
			}
			return;
			break;
		}
		case token_types::array: {
			bool is_optimizable = true;
			for (auto &tree:static_cast<token_array *>(token)->get_array()) {
				optimize_expression(tree);
				if (is_optimizable && !optimizable(tree.root()))
					is_optimizable = false;
			}
			if (is_optimizable) {
				array arr;
				bool is_map = true;
				token_value *t = nullptr;
				for (auto &tree:static_cast<token_array *>(token)->get_array()) {
					const var &val = runtime.parse_expr(tree.root());
					if (is_map && val.type() != typeid(pair))
						is_map = false;
					arr.push_back((new token_value(copy(val)))->get_value());
				}
				if (arr.empty())
					is_map = false;
				if (is_map) {
					hash_map map;
					for (auto &it:arr) {
						pair &p = it.val<pair>(true);
						if (map.count(p.first) == 0)
							map.emplace(p.first, p.second);
						else
							map[p.first] = p.second;
					}
					t = new token_value(var::make<hash_map>(std::move(map)));
				}
				else
					t = new token_value(var::make<array>(std::move(arr)));
				it.data() = t;
			}
			return;
			break;
		}
		case token_types::arglist: {
			for (auto &tree:static_cast<token_arglist *>(token)->get_arglist())
				optimize_expression(tree);
			return;
			break;
		}
		case token_types::signal: {
			switch (static_cast<token_signal *>(token)->get_signal()) {
			default:
				break;
			case signal_types::new_:
				if (it.left().data() != nullptr)
					throw syntax_error("Wrong grammar for new expression.");
				break;
			case signal_types::gcnew_:
				if (it.left().data() != nullptr)
					throw syntax_error("Wrong grammar for gcnew expression.");
				opt_expr(tree, it.right());
				return;
				break;
			case signal_types::typeid_:
				if (it.left().data() != nullptr)
					throw syntax_error("Wrong grammar for typeid expression.");
				break;
			case signal_types::not_:
				if (it.left().data() != nullptr)
					throw syntax_error("Wrong grammar for not expression.");
				break;
			case signal_types::sub_:
				if (it.left().data() == nullptr)
					it.data() = new token_signal(signal_types::minus_);
				break;
			case signal_types::mul_:
				if (it.left().data() == nullptr)
					it.data() = new token_signal(signal_types::escape_);
				break;
			case signal_types::asi_:
				if (it.left().data() == nullptr || it.right().data() == nullptr)
					throw syntax_error("Wrong grammar for assign expression.");
				opt_expr(tree, it.left());
				opt_expr(tree, it.right());
				return;
				break;
			case signal_types::vardef_: {
				if (it.left().data() != nullptr)
					throw syntax_error("Wrong grammar for variable definition.");
				token_base *rptr = it.right().data();
				if (rptr == nullptr || rptr->get_type() != token_types::id)
					throw syntax_error("Wrong grammar for variable definition.");
				runtime.storage.add_record(static_cast<token_id *>(rptr)->get_id());
				it.data() = rptr;
				return;
				break;
			}
			case signal_types::dot_: {
				opt_expr(tree, it.left());
				token_base *lptr = it.left().data();
				token_base *rptr = it.right().data();
				if (rptr == nullptr || rptr->get_type() != token_types::id)
					throw syntax_error("Wrong grammar for dot expression.");
				if (lptr != nullptr && lptr->get_type() == token_types::value) {
					var &a = static_cast<token_value *>(lptr)->get_value();
					token_base *orig_ptr = it.data();
					try {
						var v = runtime.parse_dot(a, rptr);
						if (v.is_protect())
							it.data() = new token_value(v);
					}
					catch (const syntax_error &se) {
						it.data() = orig_ptr;
					}
				}
				return;
				break;
			}
			case signal_types::fcall_: {
				opt_expr(tree, it.left());
				opt_expr(tree, it.right());
				token_base *lptr = it.left().data();
				token_base *rptr = it.right().data();
				if (lptr != nullptr && lptr->get_type() == token_types::value && rptr != nullptr && rptr->get_type() == token_types::arglist) {
					var &a = static_cast<token_value *>(lptr)->get_value();
					if (a.type() == typeid(callable) && a.const_val<callable>().is_constant()) {
						bool is_optimizable = true;
						for (auto &tree:static_cast<token_arglist *>(rptr)->get_arglist()) {
							if (is_optimizable && !optimizable(tree.root()))
								is_optimizable = false;
						}
						if (is_optimizable) {
							array arr;
							for (auto &tree:static_cast<token_arglist *>(rptr)->get_arglist())
								arr.push_back(runtime.parse_expr(tree.root()));
							it.data() = new token_value(a.val<callable>(true).call(arr));
						}
					}
				}
				return;
				break;
			}
			case signal_types::emb_: {
				opt_expr(tree, it.left());
				opt_expr(tree, it.right());
				token_base *lptr = it.left().data();
				token_base *rptr = it.right().data();
				if (!inside_lambda || lptr != nullptr || rptr == nullptr || rptr->get_type() != token_types::arglist)
					throw syntax_error("Wrong grammar for lambda expression.");
				it.data() = rptr;
				return;
				break;
			}
			case signal_types::lambda_: {
				inside_lambda = true;
				opt_expr(tree, it.left());
				inside_lambda = false;
				opt_expr(tree, it.right());
				token_base *lptr = it.left().data();
				token_base *rptr = it.right().data();
				if (lptr == nullptr || rptr == nullptr || lptr->get_type() != token_types::arglist)
					throw syntax_error("Wrong grammar for lambda expression.");
				std::deque<std::string> args;
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
				it.data() = new token_value(var::make_protect<callable>(function(args, std::deque<statement_base *> {new statement_return(cov::tree<token_base *>{it.right()}, new token_endline(0))})));
				return;
				break;
			}
			}
		}
		}
		opt_expr(tree, it.left());
		opt_expr(tree, it.right());
		if (optimizable(it.left()) && optimizable(it.right())) {
			token_value *token = new token_value(runtime.parse_expr(it));
			tree.erase_left(it);
			tree.erase_right(it);
			it.data() = token;
		}
	}
	void instance::parse_define_var(cov::tree<token_base *> &tree, define_var_profile &dvp)
	{
		const auto &it = tree.root();
		token_base *root = it.data();
		if (root == nullptr || root->get_type() != token_types::signal || static_cast<token_signal *>(root)->get_signal() != signal_types::asi_)
			throw syntax_error("Wrong grammar for variable definition.");
		token_base *left = it.left().data();
		const auto &right = it.right();
		if (left == nullptr || right.data() == nullptr || left->get_type() != token_types::id)
			throw syntax_error("Wrong grammar for variable definition.");
		dvp.id = static_cast<token_id *>(left)->get_id();
		dvp.expr = right;
	}
}