function test(...)
    print("--- test my lib")
    package.cpath = "./?.so"
    local f = require "Memory"
	
	--succ,content = f.Write("iiiiiiiiii.out",0x7fff7a295ef0,"jklfdsafs","String")
	succ,content = f.Write("iiiiiiiiii.out",0x7fff7a295ef0,567,"U32")
	print(succ)
	print(content)
    

	
end

test()
