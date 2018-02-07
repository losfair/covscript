var ok = true
var s = "hello"
var arr = {42, 99}

for i = 1 to 1000000
    var obj = builtin.new_dynamic(null)
    obj.a = 1
end

if s != "hello" || arr[0] != 42 || arr[1] != 99
    system.out.println("Bad value(s)")
else
    system.out.println("OK")
end
