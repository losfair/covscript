var obj = builtin.new_dynamic(null)
obj.a = 20
obj.a += (obj.a += 1)
if obj.a == 42
    system.out.println("OK")
else
    system.out.println("obj.a mismatch")
end
