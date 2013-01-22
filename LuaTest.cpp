#include "stdafx.h"
#include "LuaShell.h"

int main()
{
	LuaClass *shell=new LuaClass();
	shell->Init();
	shell->executeMain("test.lua");
	return 0;
}