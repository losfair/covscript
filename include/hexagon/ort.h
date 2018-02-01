#pragma once

#include "imports.h"
#include <type_traits>
#include <stdexcept>
#include <functional>
#include <vector>

namespace hexagon {
namespace ort {

class Value {
private:
    HxOrtValue res;
public:
    Value(HxOrtValue v) {
        res = v;
    }

    const HxOrtValue& Extract() const {
        return res;
    }

    static Value Null() {
        HxOrtValue place;
        hexagon_ort_value_create_from_null(&place);
        return Value(place);
    }

    template<class T> static Value FromInt(T v) {
        static_assert(std::is_integral<T>::value, "Parameter must be of an integral type");

        HxOrtValue place;
        hexagon_ort_value_create_from_i64(&place, (long long) v);
        return Value(place);
    }

    template<class T> static Value FromFloat(T v) {
        static_assert(std::is_floating_point<T>::value, "Parameter must be of a floating point type");

        HxOrtValue place;
        hexagon_ort_value_create_from_f64(&place, (double) v);
        return Value(place);
    }

    long long ExtractI64() const {
        long long ret;
        int err = hexagon_ort_value_read_i64(&ret, &res);
        if(err) {
            throw std::runtime_error("Type mismatch");
        }
        return ret;
    }

    double ExtractF64() const {
        double ret;
        int err = hexagon_ort_value_read_f64(&ret, &res);
        if(err) {
            throw std::runtime_error("Type mismatch");
        }
        return ret;
    }

    bool ExtractBool() const {
        int ret;
        int err = hexagon_ort_value_read_bool(&ret, &res);
        if(err) {
            throw std::runtime_error("Type mismatch");
        }
        return (bool) ret;
    }

    bool IsNull() const {
        int err = hexagon_ort_value_read_null(&res);
        return err == 0;
    }
};

class Function {
private:
    HxOrtFunction res;

    Function() = default;
    Function(const Function& rvalue) = delete;

public:
    Function(Function&& rvalue) = default;

    ~Function() {
        if(res != nullptr) {
            hexagon_ort_function_destroy(res);
        }
    }

    static Function LoadVirtual(
        const char *encoding,
        const unsigned char *code,
        unsigned int len
    ) {
        HxOrtFunction v = hexagon_ort_function_load_virtual(
            encoding,
            code,
            len
        );
        if(!v) {
            throw std::runtime_error("Unable to load virtual function");
        }
        Function ret;
        ret.res = v;
        return ret;
    }

    static Function LoadNative(
        const std::function<Value ()>& cb_ref
    ) {
        std::function<Value ()> *cb_handle = new std::function<Value ()>(cb_ref);
        HxOrtFunction v = hexagon_ort_function_load_native(
            [](HxOrtValue *ret_place, HxOrtExecutorImpl _exec_impl, void *cb_ptr) {
                std::function<Value ()>& cb = *(std::function<Value ()> *)cb_ptr;
                Value ret = cb();
                *ret_place = ret.Extract();
            },
            [](void *cb_ptr) {
                delete (std::function<Value ()> *)cb_ptr;
            },
            cb_handle
        );
        if(!v) {
            throw std::runtime_error("Unable to load native function");
        }
        Function ret;
        ret.res = v;
        return ret;
    }

    friend class Runtime;
};

class Runtime {
private:
    HxOrtExecutor _executor_res;
    HxOrtExecutorImpl executor;
public:
    Runtime() {
        _executor_res = hexagon_ort_executor_create();
        executor = hexagon_ort_executor_get_impl(_executor_res);
    }

    ~Runtime() {
        if(_executor_res != nullptr) {
            hexagon_ort_executor_destroy(_executor_res);
        }
    }

    Runtime& AttachFunction(const char *key, Function& f) {
        HxOrtFunction fn_res = f.res;
        f.res = nullptr;

        int ret = hexagon_ort_executor_impl_attach_function(
            executor,
            key,
            fn_res
        );
        if(ret != 0) {
            throw std::runtime_error("AttachFunction: Rejected by backend");
        }

        return *this;
    }

    Value GetStaticObject(const char *key) {
        HxOrtValue ret_place;
        hexagon_ort_executor_impl_get_static_object(
            &ret_place,
            executor,
            key
        );
        return Value(ret_place);
    }

    Value Invoke(Value obj, const std::vector<Value>& params) {
        HxOrtValue ret_place;

        std::vector<HxOrtValue> args(params.size());
        HxOrtValue *raw_args = NULL;

        if(params.size() > 0) {
            for(size_t i = 0; i < params.size(); i++) {
                args[i] = params[i].Extract();
            }
            raw_args = &args[0];
        }

        HxOrtValue target = obj.Extract();

        hexagon_ort_executor_impl_invoke(
            &ret_place,
            executor,
            &target,
            nullptr,
            raw_args,
            args.size()
        );
        return Value(ret_place);
    }
};

} // namespace ort
} // namespace hexagon
