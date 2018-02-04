var obj = builtin.new_dynamic(null)
obj.a = 42

var inherited = builtin.new_dynamic(obj)
var overrided = builtin.new_dynamic(obj)
overrided.a = 100

var ok = true

if inherited.a != 42
    system.out.println("inherited.a mismatch")
    ok = false
end

if overrided.a != 100
    system.out.println("overrided.a mismatch")
    ok = false
end

if ok
    system.out.println("OK")
end
