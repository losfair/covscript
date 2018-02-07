var a = 2
var arr = {1}
var arr2 = {a}
arr.push_back(arr2[0])
arr.push_back(3)
arr.push_back(0)

arr[3] = 4
if arr[0] == 1 && arr[1] == 2 && arr[2] == 3 && arr[3] == 4
    system.out.println("OK")
else
    system.out.println("Value mismatch")
end
