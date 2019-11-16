function test(...)
    print("--- test my lib")
    package.cpath = "./?.so"
    local f = require "Memory"
	
	succ,content = f.Read("ConsoleApplication1.out",0x7ffc85b75504,"U32")
	print(succ)
	print(string.format("res:%s\n",content))
	--succ,content = f.Read("iiiiiiiiii.out",7fff16da9ce0,"String")
	--print(succ)
	--print(content)
    --addrs = f.Search("iiiiiiiiii.out","caocaocao","String")
	--print(#addrs)
	--for i =1, #addrs do
			--str = string.format("res:%x\n",addrs[i])
			--print(str)
	--end

	
end

test()

