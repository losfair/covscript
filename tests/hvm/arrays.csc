var n = 100000
var a = {}
while a.len() < n
    a.push(a.len())
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
