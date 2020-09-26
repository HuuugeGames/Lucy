function fibo(n)
	if n < 2 then
		return n
	end

	local f1, f2, f3 = 0, 1
	for i = 2, n do
		f3 = f1 + f2
		f1 = f2
		f2 = f3
	end

	return f3
end

print(fibo(15))
