function cs(a, b)
    return a + b * 2
end

if cs(20, 11) != 42
    system.out.println("Bad return value")
else
    system.out.println("OK")
end
