#pragma once

#include <string>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <iomanip>
#include "ort.h"

namespace hexagon {
namespace assembly_writer {
	// we skip the `usize` case here because the serializer
	// doesn't care.
	enum class OperandType {
		i64_,
		f64_,
		string_,
		bool_,
	};
	struct Operand {
		OperandType type;

		long long i64_value;
		double f64_value;
		std::string string_value;
		bool bool_value;

		static Operand I64(long long i) {
			Operand v;
			v.type = OperandType::i64_;
			v.i64_value = i;
			return v;
		}

		static Operand F64(double i) {
			Operand v;
			v.type = OperandType::f64_;
			v.f64_value = i;
			return v;
		}

		static Operand String(const std::string& i) {
			Operand v;
			v.type = OperandType::string_;
			v.string_value = i;
			return v;
		}

		static Operand Bool(bool i) {
			Operand v;
			v.type = OperandType::bool_;
			v.bool_value = i;
			return v;
		}

		long long GetI64() {
			if(type != OperandType::i64_) {
				throw std::runtime_error("Type mismatch");
			}
			return i64_value;
		}

		double GetF64() {
			if(type != OperandType::f64_) {
				throw std::runtime_error("Type mismatch");
			}
			return f64_value;
		}

		std::string& GetString() {
			if(type != OperandType::string_) {
				throw std::runtime_error("Type mismatch");
			}
			return string_value;
		}

		bool GetBool() {
			if(type != OperandType::bool_) {
				throw std::runtime_error("Type mismatch");
			}
			return bool_value;
		}
	};

	struct BytecodeOp {
		std::string name;
		std::vector<Operand> operands;

		BytecodeOp(const std::string& _name) {
			name = _name;
		}

		BytecodeOp(
			const std::string& _name,
			const Operand& arg1
		) {
			name = _name;
			operands.push_back(arg1);
		}

		BytecodeOp(
			const std::string& _name,
			const Operand& arg1,
			const Operand& arg2
		) {
			name = _name;
			operands.push_back(arg1);
			operands.push_back(arg2);
		}
	};

	class BasicBlockWriter {
	public:
		std::vector<BytecodeOp> opcodes;

		BasicBlockWriter() = default;
		BasicBlockWriter(const BasicBlockWriter& other) = delete;
		BasicBlockWriter(BasicBlockWriter&& other) = default;

		BasicBlockWriter& Write(const BytecodeOp& op) {
            opcodes.push_back(op);
            return *this;
		}

		void Clear() {
			opcodes.clear();
		}

		BasicBlockWriter Clone() const {
			BasicBlockWriter ret;
			ret.opcodes = opcodes;
			return ret;
		}
	};

	class FunctionWriter {
	private:
        std::vector<BasicBlockWriter> basic_blocks;
        std::function<void (std::vector<BasicBlockWriter>&)> user_translator;

		// https://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c/33799784#33799784
		static std::string escape_json(const std::string& s) {
			std::ostringstream o;
			for (auto c = s.cbegin(); c != s.cend(); c++) {
				if (*c == '"' || *c == '\\' || ('\x00' <= *c && *c <= '\x1f')) {
					o << "\\u"
					  << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
				} else {
					o << *c;
				}
			}
			return o.str();
		}

		static std::string operand_to_string(const Operand& operand) {
			std::ostringstream o;
			
			switch(operand.type) {
				case OperandType::i64_:
					o << operand.i64_value;
					break;
				case OperandType::f64_:
					o << operand.f64_value;
					break;
				case OperandType::string_:
					o << "\"" << escape_json(operand.string_value) << "\"";
					break;
				case OperandType::bool_:
					o << (operand.bool_value ? "true" : "false");
					break;
				default:
					throw 0;
			}

			return o.str();
		}

    public:
        FunctionWriter() {
            user_translator = nullptr;
        }

        FunctionWriter(const std::function<void (std::vector<BasicBlockWriter>&)>& ut) {
            user_translator = ut;
        }

		FunctionWriter& Write(const BasicBlockWriter& bb) {
            basic_blocks.push_back(bb.Clone());
            return *this;
        }
        
        ort::Function Build() {
            if(user_translator != nullptr) {
                user_translator(basic_blocks);
            }

            std::string code = ToJson();

            return ort::Function::LoadVirtual(
                "json",
                (const unsigned char *) code.c_str(),
                code.size()
            );
        }

		std::string ToJson() {
			std::string output;

			output += "{\"basic_blocks\":";

			output += "[";

			bool is_first_bb = true;

			for (auto& bb : basic_blocks) {
				if(is_first_bb) {
					is_first_bb = false;
				} else {
					output += ",";
				}

				output += "{\"opcodes\":";

				output += "[";
				
				bool is_first = true;
	
				for(auto& op : bb.opcodes) {
					if(is_first) {
						is_first = false;
					} else {
						output += ",";
					}
	
					if(op.operands.size() == 0) {
						output += "\"";
						output += escape_json(op.name);
						output += "\"";
					} else {
						output += "{\"";
						output += escape_json(op.name);
						output += "\":";
	
						if(op.operands.size() == 1) {
							output += operand_to_string(op.operands[0]);
						} else {
							output += "[";
	
							bool inner_first = true;
							for(auto& iop : op.operands) {
								if(inner_first) {
									inner_first = false;
								} else {
									output += ",";
								}
	
								output += operand_to_string(iop);
							}
	
							output += "]";
						}
	
						output += "}";
					}
				}
	
				output += "]}";
			}

			output += "]}";
			return output;
		}
    };
} // namespace assembly_writer
} // namespace hexagon
