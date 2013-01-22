// LuaScript.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "interface.h"
#include <vector>
#include "HSComm.pb.h"
#include <string>


extern "C"  
{  
    #include "lua.h"  
    #include "lauxlib.h"  
    #include "lualib.h"  
}  

using namespace std;
using namespace HSCommPB;

const int MAX_THREAD_NUM=30;

/*
luaClass is a shell that is responsible for adding new functions and excute lua script
*/

class LuaClass
{
public:
	LuaClass();
	~LuaClass(){};
	int	Init(string serverAddress="127.0.0.1", int cmdPort=12298, int dataPort=12299);
	int	execute(int port,string filename); //the first parameter is the coroutine to execute the file (second parameter)
	int	executeMain(string filename); 
	static void* svrAddr;
	static void* usrName;
	static void* hvcClient;

private:
	DWORD nSend,nRecv,nSucc,nFail;
	CCHANDLE hVirtualCloudClient;
	lua_State *pMainShell;
	lua_State *pCoroutines[MAX_THREAD_NUM];
	string serverAddress,userName;
	int cmdPort,dataPort;


/*
the following procedures are intended to execute the shell command
*/
public:
	static int shell_sleep(lua_State *pState);
	static int shell_connect(lua_State *pState);

	static int shell_entrust(lua_State *pState);
	static void CallBack_Entrust(CCHANDLE pSender, IOPCompleteArgs *pIOP);
	static int shell_query_entrust(lua_State *pState);
	static void CallBack_Query_entrust(CCHANDLE pSender, IOPCompleteArgs *pIOP);
	static int shell_query_orders(lua_State *pState);
	static void CallBack_Query_orders(CCHANDLE pSender, IOPCompleteArgs *pIOP);
	static int shell_query_capital(lua_State *pState);
	static void CallBack_Query_capital(CCHANDLE pSender, IOPCompleteArgs *pIOP);
};