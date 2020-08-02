local function factorial(n)
	local result = 1
	for i = 2, 5 do
		result = result * i
	end
	
	return result
end

n = 5
print(factorial(n))
