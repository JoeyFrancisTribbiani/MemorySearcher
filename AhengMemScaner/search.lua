function test(...)
    print("--- test my lib")
    package.cpath = "./?.so"
    local f = require "Memory"

    addrs = f.Search("ConsoleApplication1.out",{7131126,-4,23},"U32")
	--addrs = f.Search("iiiiiiiiii.out","caocaocao   ,1,899,87,888,-1,32","String")
	--addrs =f.Search("iiiiiiiiii.out","caocaocao","String")
	

	print(string.format("addr size:%d\n",#addrs))
	for i =1, #addrs do
			str = string.format("res:%d\n",addrs[i])
			print(str)
	end
	
end

test()
