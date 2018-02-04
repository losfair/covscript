#include <covscript/any.hpp>
#include <covscript/core.hpp>
#include <covscript/runtime.hpp>
#include <hexagon/ort.h>
#include <vector>
#include <iostream>

namespace cs_impl {
    any any::from_hvm_value(const ort::Value& v) {
        switch(v.Type()) {
            case ort::ValueType::Bool: {
                return any(v.ExtractBool());
            }
            case ort::ValueType::Float: {
                return any(v.ExtractF64());
            }
            case ort::ValueType::Int: {
                return any(v.ExtractI64());
            }
            case ort::ValueType::Null: {
                throw cs::internal_error("Conversion not supported (null)");
            }
            case ort::ValueType::Object: {
                return any(v.ToString(*cs::get_active_runtime()));
            }
            default: {
                throw cs::internal_error("from_hvm_value: Unknown value type");
            }
        }
    }

    ort::Value any::to_hvm_value() {
        const std::type_info& v_type = type();
        if(v_type == typeid(int) || v_type == typeid(long) || v_type == typeid(long long)) {
            return ort::Value::FromInt(to_integer());
        } else if(v_type == typeid(float)) {
            return ort::Value::FromFloat(const_val<float>());
        } else if(v_type == typeid(double)) {
            return ort::Value::FromFloat(const_val<double>());
        } else if(v_type == typeid(long double)) {
            return ort::Value::FromFloat(const_val<long double>());
        } else if(v_type == typeid(std::string)) {
            return ort::Value::FromString(const_val<std::string>(), *cs::get_active_runtime());
        } else {
            return ort::ObjectProxy(new any(*this)).Pin(*cs::get_active_runtime());
        }
    }

    ort::Value any::Call(const std::vector<ort::Value>& args) {
        try {
            if(type() == typeid(cs::callable)) {
                std::vector<any> call_args;
                call_args.reserve(args.size());
                for(auto& v : args) {
                    call_args.push_back(from_hvm_value(v));
                }
                auto ret = const_val<cs::callable>().call(call_args);
                return ret.to_hvm_value();
            } else if(type() == typeid(cs::object_method)) {
                auto m = const_val<cs::object_method>();

                std::vector<any> call_args;
                call_args.reserve(args.size() + 1);
                call_args.push_back(m.object);
                for(auto& v : args) {
                    call_args.push_back(from_hvm_value(v));
                }

                auto ret = m.callable.const_val<cs::callable>().call(call_args);
                return ret.to_hvm_value();
            } else {
                std::cerr << type().name() << std::endl;
                throw cs::internal_error("Call: Not callable");
            }
        } catch(const cs::lang_error& e) {
            std::cerr << e.what() << std::endl;
            throw e;
        } catch(const std::exception& e) {
            std::cerr << e.what() << std::endl;
            throw e;
        }
    }

    ort::Value any::GetField(const char *name) {
        try {
            if(type() == typeid(cs::extension_t)) {
                return val<cs::extension_t>(true) -> get_var(name).to_hvm_value();
            } else if(type() == typeid(cs::type)) {
                return val<cs::type>(true).get_var(name).to_hvm_value();
            } else {
                any &v = get_ext()->get_var(name);
                if (v.type() == typeid(cs::callable))
                    return any::make_protect<cs::object_method>(*this, v, v.const_val<cs::callable>().is_constant()).to_hvm_value();
                else
                    return v.to_hvm_value();
            }
        } catch(const cs::lang_error& e) {
            std::cerr << e.what() << std::endl;
            throw e;
        } catch(const std::exception& e) {
            std::cerr << e.what() << std::endl;
            throw e;
        }
    }
}
