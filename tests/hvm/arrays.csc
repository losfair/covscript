var n = 100000
var a = {}
while a.size() < n
    a.push_back(a.size())
end

var i = 0
var invalid = false
while i < n
    if a[i] != i
        invalid = true
        break
    end
    i++
end

if invalid
    system.out.println("Bad array")
else
    system.out.println("OK")
end
