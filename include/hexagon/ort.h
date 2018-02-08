#pragma once

#include "imports.h"
#include <type_traits>
#include <stdexcept>
#include <functional>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

namespace hexagon {

void EnableDebug() {
    hexagon_enable_debug();
}

namespace ort {

class Runtime;
class ObjectHandle;

enum class ValueType {
    Unknown,
    Bool,
    Float,
    Int,
    Null,
    Object
};

class Value {
private:
    HxOrtValue res;
public:
    Value(HxOrtValue v) {
        res = v;
    }

    ValueType Type() const noexcept {
        char t = hexagon_ort_value_get_type(&res);
        switch(t) {
            case 'B': {
                return ValueType::Bool;
            }
            case 'F': {
                return ValueType::Float;
            }
            case 'I': {
                return ValueType::Int;
            }
            case 'N': {
                return ValueType::Null;
            }
            case 'O': {
                return ValueType::Object;
            }
            default: {
                return ValueType::Unknown;
            }
        }
    }

    const HxOrtValue& Extract() const noexcept {
        return res;
    }

    static Value Null() noexcept {
        HxOrtValue place;
        hexagon_ort_value_create_from_null(&place);
        return Value(place);
    }

    template<class T> static Value FromInt(T v) noexcept {
        static_assert(std::is_integral<T>::value, "Parameter must be of an integral type");

        HxOrtValue place;
        hexagon_ort_value_create_from_i64(&place, (long long) v);
        return Value(place);
    }

    template<class T> static Value FromFloat(T v) noexcept {
        static_assert(std::is_floating_point<T>::value, "Parameter must be of a floating point type");

        HxOrtValue place;
        hexagon_ort_value_create_from_f64(&place, (double) v);
        return Value(place);
    }

    static Value FromBool(bool v) noexcept {
        HxOrtValue place;
        hexagon_ort_value_create_from_bool(&place, (unsigned int) v);
        return Value(place);
    }

    static Value FromString(const std::string& s, Runtime& rt);

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

    double ToF64() const {
        switch(Type()) {
            case ValueType::Float: {
                return ExtractF64();
            }
            case ValueType::Int: {
                return (double) ExtractI64();
            }
            default: {
                throw std::runtime_error("Cannot convert to f64");
            }
        }
    }

    long long ToI64() const {
        switch(Type()) {
            case ValueType::Float: {
                return (long long) ExtractF64();
            }
            case ValueType::Int: {
                return ExtractI64();
            }
            default: {
                throw std::runtime_error("Cannot convert to i64");
            }
        }
    }

    bool ExtractBool() const {
        int ret;
        int err = hexagon_ort_value_read_bool(&ret, &res);
        if(err) {
            throw std::runtime_error("Type mismatch");
        }
        return (bool) ret;
    }

    bool IsString(Runtime& rt) const;
    std::string ToString(Runtime& rt) const;
    ObjectHandle ToObjectHandle(Runtime& rt) const;

    bool IsNull() const noexcept {
        int err = hexagon_ort_value_read_null(&res);
        return err == 0;
    }
};

class ProxiedObject;

class ObjectHandle {
private:
    HxOrtObjectHandle res;

    ObjectHandle() = default;
public:
    ObjectHandle(const ObjectHandle& rvalue) = delete;
    ObjectHandle(ObjectHandle&& rvalue) {
        res = rvalue.res;
        rvalue.res = nullptr;
    }
    ~ObjectHandle() {
        if(res != nullptr) {
            hexagon_ort_object_handle_destroy(res);
        }
    }

    ProxiedObject * ToProxiedObject();
    std::string DumpVirtualFunction();
    void DebugPrintVirtualFunction();

    friend class Value;
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

    void EnableOptimization() {
        if(res == nullptr) {
            throw std::runtime_error("Use of dropped function");
        }

        hexagon_ort_function_enable_optimization(res);
    }

    void BindThis(const Value& v) {
        if(res == nullptr) {
            throw std::runtime_error("Use of dropped function");
        }
        hexagon_ort_function_bind_this(res, &v.Extract());
    }

    Value Pin(Runtime& rt);

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
            [](HxOrtValue *ret_place, HxOrtExecutorImpl _exec_impl, void *cb_ptr) -> int {
                std::function<Value ()>& cb = *(std::function<Value ()> *)cb_ptr;
                try {
                    Value ret = cb();
                    *ret_place = ret.Extract();
                    return 0;
                } catch(...) {
                    *ret_place = Value::Null().Extract();
                    return 1;
                }
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

    HxOrtExecutorImpl _impl_handle() {
        return executor;
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

    void SetStackLimit(unsigned int limit) {
        hexagon_ort_executor_impl_set_stack_limit(executor, limit);
    }

    Value GetArgument(unsigned int id) {
        HxOrtValue ret_place;

        int err = hexagon_ort_executor_impl_get_argument(
            &ret_place,
            executor,
            id
        );
        if(err != 0) {
            throw std::runtime_error("Argument index out of bound");
        }

        return Value(ret_place);
    }

    unsigned int GetNArguments() {
        return hexagon_ort_executor_impl_get_n_arguments(executor);
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

class ObjectProxy;

class ProxiedObject {
private:
    HxOrtObjectProxy proxy = nullptr;

protected:
    bool IsInitialized() {
        if(proxy) {
            return true;
        } else {
            return false;
        }
    }
    void Freeze() {
        if(!proxy) {
            throw std::logic_error("Attempting to use an object proxy after drop");
        }
        hexagon_ort_object_proxy_freeze(proxy);
    }

    void AddConstField(const std::string& name) {
        if(!proxy) {
            throw std::logic_error("Attempting to use an object proxy after drop");
        }
        hexagon_ort_object_proxy_add_const_field(proxy, name.c_str());
    }

    void SetStaticField(const std::string& k, const Value& v) {
        if(!proxy) {
            throw std::logic_error("Attempting to use an object proxy after drop");
        }

        hexagon_ort_object_proxy_set_static_field(proxy, k.c_str(), &v.Extract());
    }

public:
    virtual ~ProxiedObject() {};

    void __HxOnAttachToProxy(HxOrtObjectProxy _proxy) {
        proxy = _proxy;
    }

    virtual void Init(ObjectProxy& proxy) {

    }

    virtual Value Call(const std::vector<Value>& args) {
        throw std::runtime_error("Call: Not implemented");
    };

    virtual Value GetField(const char *name) {
        throw std::runtime_error("GetField: Not implemented");
    };
};

class ObjectProxy final {
private:
    HxOrtObjectProxy proxy;

public:
    ObjectProxy(const ObjectProxy& other) = delete;
    ObjectProxy(ObjectProxy&& other) {
        proxy = other.proxy;
        other.proxy = nullptr;
    }

    ObjectProxy(ProxiedObject *proxied) {
        proxy = hexagon_ort_object_proxy_create((void *) &*proxied);
        hexagon_ort_object_proxy_set_destructor(proxy, [](
            void *data
        ) {
            ProxiedObject *proxied = (ProxiedObject *) data;
            delete proxied;
        });
        hexagon_ort_object_proxy_set_on_call(proxy, [](
            HxOrtValue *place,
            void *data,
            unsigned int n_args,
            const HxOrtValue *args
        ) -> int {
            ProxiedObject *proxied = (ProxiedObject *) data;

            std::vector<Value> target_args;
            for(unsigned int i = 0; i < n_args; i++) {
                target_args.push_back(args[i]);
            }
            try {
                Value ret = proxied -> Call(target_args);
                *place = ret.Extract();
                return 0;
            } catch(const std::exception& e) {
                std::cerr << e.what() << std::endl;
                return 1;
            } catch(...) {
                return 1;
            }
        });
        hexagon_ort_object_proxy_set_on_get_field(proxy, [](
            HxOrtValue *place,
            void *data,
            const char *field_name
        ) -> int {
            ProxiedObject *proxied = (ProxiedObject *) data;
            try {
                Value ret = proxied -> GetField(field_name);
                *place = ret.Extract();
                return 0;
            } catch(const std::exception& e) {
                std::cerr << e.what() << std::endl;
                return 1;
            } catch(...) {
                return 1;
            }
        });
        proxied -> __HxOnAttachToProxy(proxy);
        proxied -> Init(*this);
    }

    void SetStaticField(const std::string& k, const Value& v) {
        if(!proxy) {
            throw std::logic_error("Attempting to use an object proxy after drop");
        }

        hexagon_ort_object_proxy_set_static_field(proxy, k.c_str(), &v.Extract());
    }

    Value Pin(Runtime& rt) {
        if(!proxy) {
            throw std::logic_error("Attempting to pin an object proxy that is already dropped");
        }

        HxOrtValue ret;
        hexagon_ort_executor_pin_object_proxy(
            &ret,
            rt._impl_handle(),
            proxy
        );
        proxy = nullptr;
        return Value(ret);
    }

    void Freeze() {
        if(!proxy) {
            throw std::logic_error("Attempting to use an object proxy after drop");
        }
        hexagon_ort_object_proxy_freeze(proxy);
    }

    void AddConstField(const std::string& name) {
        if(!proxy) {
            throw std::logic_error("Attempting to use an object proxy after drop");
        }
        hexagon_ort_object_proxy_add_const_field(proxy, name.c_str());
    }

    ~ObjectProxy() {
        if(proxy) {
            // The proxied object will be destroyed in the destructor callback
            hexagon_ort_object_proxy_destroy(proxy);
        }
    }
};

Value Function::Pin(Runtime& rt) {
    if(res == nullptr) {
        throw std::runtime_error("Use of dropped function");
    }

    HxOrtValue place;

    hexagon_ort_executor_pin_function(
        &place,
        rt._impl_handle(),
        res
    );
    res = nullptr;

    return place;
}

Value Value::FromString(const std::string& s, Runtime& rt) {
    HxOrtValue place;
    hexagon_ort_value_create_from_string(
        &place,
        s.c_str(),
        rt._impl_handle()
    );
    return Value(place);
}

std::string Value::ToString(Runtime& rt) const {
    char *v = hexagon_ort_value_read_string(
        &res,
        rt._impl_handle()
    );
    if(!v) {
        throw std::runtime_error("Cannot convert to string");
    }
    std::string ret = v;
    hexagon_glue_destroy_cstring(v);
    return ret;
}

bool Value::IsString(Runtime& rt) const {
    return (bool) hexagon_ort_value_is_string(&res, rt._impl_handle());
}

ObjectHandle Value::ToObjectHandle(Runtime& rt) const {
    ObjectHandle ret;
    ret.res = hexagon_ort_value_to_object_handle(&res, rt._impl_handle());
    if(ret.res == nullptr) {
        throw std::runtime_error("Cannot convert to object handle");
    }
    return ret;
}

ProxiedObject * ObjectHandle::ToProxiedObject() {
    HxOrtObjectProxy proxy = hexagon_ort_object_handle_to_object_proxy(res);
    if(proxy == nullptr) {
        throw std::runtime_error("Not an object proxy");
    }
    ProxiedObject *obj = (ProxiedObject *) hexagon_ort_object_proxy_get_data(proxy);
    return obj;
}

std::string ObjectHandle::DumpVirtualFunction() {
    HxOrtFunction f = hexagon_ort_object_handle_to_function(res);
    if(f == nullptr) {
        throw std::runtime_error("Not a function");
    }
    char *code = hexagon_ort_function_dump_json(f);
    if(code == nullptr) {
        throw std::runtime_error("The function is not a printable virtual function. Try dump it before any optimizations.");
    }
    std::string ret = code;
    hexagon_glue_destroy_cstring(code);
    return ret;
}

void ObjectHandle::DebugPrintVirtualFunction() {
    HxOrtFunction f = hexagon_ort_object_handle_to_function(res);
    if(f == nullptr) {
        throw std::runtime_error("Not a function");
    }
    hexagon_ort_function_debug_print(f);
}

} // namespace ort
} // namespace hexagon
