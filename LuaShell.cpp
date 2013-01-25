#include "stdafx.h"
#include "LuaShell.h"
#include <iostream>
#include <windows.h>

using namespace std;

void* LuaClass::svrAddr;
void* LuaClass::hvcClient;

LuaClass::LuaClass()
{
	/*
	this->svrAddr=(void*)malloc(sizeof(char));
	this->usrName=(void*)malloc(sizeof(char));
	this->hvcClient=(void*)malloc(sizeof(char));
	*/
	this->Init();
	}


int	LuaClass::Init(string serverAddress, int cmdPort, int dataPort)
{
	this->serverAddress=serverAddress;
	this->cmdPort	=cmdPort;
	this->dataPort	=dataPort;

	pMainShell		=lua_open();
	luaopen_base(pMainShell);  
    luaopen_table(pMainShell);  
    luaL_openlibs(pMainShell);  
    luaopen_string(pMainShell);  
    luaopen_math(pMainShell); 
	luaopen_pluto(pMainShell);
	nSend=nRecv=nSucc=nFail=0;

	hVirtualCloudClient=CreateVirtualCloudClient();
	for (int i=0;i<MAX_THREAD_NUM;i++)	pCoroutines[i]=NULL;


	lua_register(pMainShell,"connect", shell_connect);// connect(severaddress,username,cmdport,dataport)
	lua_register(pMainShell,"entrust", shell_entrust);//entrust(account,password,stockcode,entrustamount,entrustprice)
	lua_register(pMainShell,"query_entrust",shell_query_entrust);//query_entrust(account,password,entrustno,queryOnlyCancelable,requestNumber,lastPositionString)
	lua_register(pMainShell,"query_orders",shell_query_orders);
	lua_register(pMainShell,"query_capital",shell_query_capital);
	lua_register(pMainShell,"sleep",shell_sleep);

	lua_pushlightuserdata(pMainShell,this->svrAddr);
	lua_pushstring(pMainShell,serverAddress.c_str());
	lua_settable(pMainShell, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(pMainShell,(void*)this->hvcClient);
	lua_pushlightuserdata(pMainShell,hVirtualCloudClient);
	lua_settable(pMainShell, LUA_REGISTRYINDEX);
	
	return 0;
}


int LuaClass::executeMain(string filename)
{
	int iError;
	iError = luaL_loadfile(pMainShell, filename.c_str());  
	if (iError)
	{
		cout << "Load test script FAILED!" << lua_tostring(pMainShell, -1)<< endl;  
		lua_close(pMainShell);
		return 0;
	//}
	iError = lua_pcall(pMainShell, 0, 0, 0);
	if (iError)
	{
		cout<< "Load test script FAILED!"<< lua_tostring(pMainShell,-1)<< endl;
		return 0;
	}
	return 1;
}
}

int LuaClass::shell_sleep(lua_State *pState)
{
	int seconds;
	seconds=lua_tointeger(pState,1);
	Sleep(seconds);
	return 0;
}

int LuaClass::shell_connect(lua_State *pState)
{

	CCHANDLE hVirtualCloudClient;
	DWORD nSend,nRecv,nSucc,nFail;

	nSend=nRecv=nSucc=nFail=0;

	lua_pushlightuserdata(pState, (void*)hvcClient);
	lua_gettable(pState, LUA_REGISTRYINDEX);
	hVirtualCloudClient=(void*)lua_touserdata(pState,-1);

	if(hVirtualCloudClient==NULL)	return -1;
	SetServerAddress(hVirtualCloudClient,"127.0.0.1");
	SetCmdPort(hVirtualCloudClient,12298);
	SetDataPort(hVirtualCloudClient,12299);
	SetUsername(hVirtualCloudClient,"tester");
	SetPassword(hVirtualCloudClient,"123456");
	if (Connect(hVirtualCloudClient))
	{
		lua_pushboolean(pState,true);
		return 1;
	}
	else
	{
		lua_pushboolean(pState,false);
		return 1;
	}
//	cout<<"now you can execute your own codes"<<endl;
//	return 0;
}


/*
	the function entrust is called with the function entrust.
	entrust(account,password,stockcode,entrustamount,entrustprice)
	entrust(string,string,string,int,int)
*/
int LuaClass::shell_entrust(lua_State *pState)
{
	CCHANDLE hVirtualCloudClient;
	DWORD nSend,nRecv,nSucc,nFail;

	nSend=nRecv=nSucc=nFail=0;

	
	lua_pushlightuserdata(pState, (void*)hvcClient);
	lua_gettable(pState, LUA_REGISTRYINDEX);
	hVirtualCloudClient=(void*)lua_touserdata(pState,-1);

	/*set arguements of postIO*/
	int FuncID=13250;
	int Location=3;
	int Sync=2;
	char DriverName[]="CloudServerB";
	char DeviceName[]="CloudServerB";
	unsigned int Object=0;
	int MajorCmd=1;
	int MinorCmd=0;
	unsigned char *pInBuffer;
	unsigned int InBufferLen=0;
	unsigned char *pOutBuffer=NULL;
	unsigned int OutBufferLen=0;
	unsigned char *pErrorInfo=NULL;
	unsigned int ErrorInfoLen=0;
	unsigned int ErrorCode=0;
	IOState StateCode=IO_STATE_NONE;
	DWORD nHandle=0;
	char outstr[1000];
	ZeroMemory(outstr,1000);

	/*fill in the request data */
	entrustStockRequest *data=new entrustStockRequest();
	entrustStockRequest_call *req=data->add_requests();
	req->set_localid(0);
	req->set_branchno("");
	req->set_fundaccount("test2011");
	req->set_password("123456");
	req->set_exchangetype(enums_exchangeType_shanghai);
	req->set_stockaccount("");
	req->set_stockcode("601988");
	req->set_entrustamount(100);
	req->set_entrustprice(27600);
	req->set_entrustbs(enums_entrustBS_buy);
/*
	req->set_fundaccount(lua_tostring(pState,1));
	req->set_password(lua_tostring(pState,2));
	req->set_stockcode(lua_tostring(pState,3));
	req->set_entrustamount(lua_tointeger(pState,4));
	req->set_entrustprice(lua_tointeger(pState,5));
*/
	InBufferLen=data->ByteSize();
	pInBuffer=(unsigned char*)malloc(data->ByteSize());
	data->SerializeWithCachedSizesToArray(pInBuffer);

	lua_State *usrdata=pState;
	/*post the request and wait for callback */
	CTime t;
	t=CTime::GetCurrentTime();
	if(!PostIO(hVirtualCloudClient, FuncID, Location, Sync, DriverName, DeviceName, Object, MajorCmd, MinorCmd, pInBuffer, InBufferLen, (PIOCOMP)CallBack_Entrust, usrdata ,300000))
	{
		printf("%02d%02d%02d: PostIO  Request:%s Error\n",t.GetHour(),t.GetMinute(),t.GetSecond(),pInBuffer);
	}
//	delete data;
	return lua_yield(pState,0);
}

void LuaClass::CallBack_Entrust(CCHANDLE pSender, IOPCompleteArgs *pIOP)
{
	entrustStockResponse response;
	response.ParsePartialFromArray(pIOP->pOutBuffer,pIOP->OutBufferLen);
	entrustStockResponse_result result=response.responses(0);
	lua_State *L=(lua_State*)(pIOP->UserData);
	lua_pushboolean(L,true);
	lua_pushinteger(L,result.localid());
	lua_pushstring(L,result.entrustno().c_str());
//	delete pIOP;
	lua_resume(L,3);
}


int LuaClass::shell_query_entrust(lua_State *pState)
{
	CCHANDLE hVirtualCloudClient;
	DWORD nSend,nRecv,nSucc,nFail;

	nSend=nRecv=nSucc=nFail=0;

	
	lua_pushlightuserdata(pState, (void*)hvcClient);
	lua_gettable(pState, LUA_REGISTRYINDEX);
	hVirtualCloudClient=(void*)lua_touserdata(pState,-1);

	/*set arguements of postIO*/
	int FuncID=13250;
	int Location=3;
	int Sync=2;
	char DriverName[]="CloudServerB";
	char DeviceName[]="CloudServerB";
	unsigned int Object=0;
	int MajorCmd=2;
	int MinorCmd=0;
	unsigned char *pInBuffer;
	unsigned int InBufferLen=0;
	unsigned char *pOutBuffer=NULL;
	unsigned int OutBufferLen=0;
	unsigned char *pErrorInfo=NULL;
	unsigned int ErrorInfoLen=0;
	unsigned int ErrorCode=0;
	IOState StateCode=IO_STATE_NONE;
	DWORD nHandle=0;
	char outstr[1000];
	ZeroMemory(outstr,1000);

	/*fill in the request data */
	queryEntrustRequest *data=new queryEntrustRequest;
	queryEntrustRequest_call *req=data->add_requests();
	req->set_branchno("");
	req->set_entrustno("10");
	req->set_fundaccount("test2011");
	req->set_lastpositionstring("");
	req->set_password("123456");
	req->set_queryonlycancelable(true);
	req->set_requestnumber(1);

	InBufferLen=data->ByteSize();
	pInBuffer=(unsigned char*)malloc(data->ByteSize());
	data->SerializeWithCachedSizesToArray(pInBuffer);

	lua_State *usrdata=pState;
	/*post the request and wait for callback */
	CTime t;
	t=CTime::GetCurrentTime();
	if(!PostIO(hVirtualCloudClient, FuncID, Location, Sync, DriverName, DeviceName, Object, MajorCmd, MinorCmd, pInBuffer, InBufferLen, (PIOCOMP)CallBack_Query_entrust, usrdata ,300000))
	{
		printf("%02d%02d%02d: PostIO  Request:%s Error\n",t.GetHour(),t.GetMinute(),t.GetSecond(),pInBuffer);
	}
//	delete data;
	return lua_yield(pState,0);
}

void LuaClass::CallBack_Query_entrust(CCHANDLE pSender, IOPCompleteArgs *pIOP)
{
	queryEntrustResponse response;
	response.ParsePartialFromArray(pIOP->pOutBuffer,pIOP->OutBufferLen);
	queryEntrustResponse_result result=response.responses(0);
	queryEntrustResponse_result_entrust entrust=result.entrusts(0);
	lua_State *L=(lua_State*)(pIOP->UserData);
	lua_pushboolean(L,true);
	lua_pushstring(L,entrust.entrustno().c_str());
	lua_pushstring(L,entrust.stockcode().c_str());
	lua_pushstring(L,entrust.stockname().c_str());
	lua_pushinteger(L,entrust.entrustamount());
	lua_pushinteger(L,entrust.entrustprice());
//	lua_pushinteger(L,entrust.businessamount());
//	lua_pushinteger(L,entrust.businessPrice());
	lua_pushinteger(L,entrust.cancelvol());
	lua_pushinteger(L,entrust.internalsid());
//	lua_pushinteger(L,entrust.FEEandTAX());
	lua_pushinteger(L,entrust.tradedone());
	lua_pushinteger(L,entrust.success());
	lua_resume(L,10);
}


int LuaClass::shell_query_orders(lua_State *pState)
{
	CCHANDLE hVirtualCloudClient;
	DWORD nSend,nRecv,nSucc,nFail;

	nSend=nRecv=nSucc=nFail=0;

	
	lua_pushlightuserdata(pState, (void*)hvcClient);
	lua_gettable(pState, LUA_REGISTRYINDEX);
	hVirtualCloudClient=(void*)lua_touserdata(pState,-1);

	/*set arguements of postIO*/
	int FuncID=13250;
	int Location=3;
	int Sync=2;
	char DriverName[]="CloudServerB";
	char DeviceName[]="CloudServerB";
	unsigned int Object=0;
	int MajorCmd=3;
	int MinorCmd=0;
	unsigned char *pInBuffer;
	unsigned int InBufferLen=0;
	unsigned char *pOutBuffer=NULL;
	unsigned int OutBufferLen=0;
	unsigned char *pErrorInfo=NULL;
	unsigned int ErrorInfoLen=0;
	unsigned int ErrorCode=0;
	IOState StateCode=IO_STATE_NONE;
	DWORD nHandle=0;
	char outstr[1000];
	ZeroMemory(outstr,1000);

	/*fill in the request data */
	queryOrdersRequest *data=new queryOrdersRequest;
	queryOrdersRequest_call *req=data->add_requests();
	req->set_date(20120826);
	req->set_username("test2011");
	req->set_password("123456");

	InBufferLen=data->ByteSize();
	pInBuffer=(unsigned char*)malloc(data->ByteSize());
	data->SerializeWithCachedSizesToArray(pInBuffer);

	lua_State *usrdata=pState;
	/*post the request and wait for callback */
	CTime t;
	t=CTime::GetCurrentTime();
	if(!PostIO(hVirtualCloudClient, FuncID, Location, Sync, DriverName, DeviceName, Object, MajorCmd, MinorCmd, pInBuffer, InBufferLen, (PIOCOMP)CallBack_Query_orders, usrdata ,300000))
	{
		printf("%02d%02d%02d: PostIO  Request:%s Error\n",t.GetHour(),t.GetMinute(),t.GetSecond(),pInBuffer);
	}
//	delete data;
	return lua_yield(pState,0);
}

void LuaClass::CallBack_Query_orders(CCHANDLE pSender, IOPCompleteArgs *pIOP)
{
	queryOrdersResponse response;
	response.ParsePartialFromArray(pIOP->pOutBuffer,pIOP->OutBufferLen);
	queryOrdersResponse_result result=response.responses(0);
	queryOrdersResponse_result_query query=result.queries(0);
	lua_State *L=(lua_State*)(pIOP->UserData);
	lua_pushboolean(L,true);
	lua_pushinteger(L,query.returndate());
	lua_pushstring(L,query.secuname().c_str());
	lua_pushstring(L,query.code().c_str());
	lua_pushstring(L,query.direction().c_str());
	lua_pushinteger(L,query.ordervol());
	lua_pushinteger(L,query.orderprice());
	lua_pushinteger(L,query.averageprice());
	lua_pushinteger(L,query.tradevol());
	lua_pushinteger(L,query.cancelvol());
	lua_pushstring(L,query.flag().c_str());
	lua_pushstring(L,query.adddatetime().c_str());
	lua_pushinteger(L,query.acid());
	lua_pushinteger(L,query.oid());
	lua_resume(L,14);
}


int LuaClass::shell_query_capital(lua_State *pState)
{
	CCHANDLE hVirtualCloudClient;
	DWORD nSend,nRecv,nSucc,nFail;

	nSend=nRecv=nSucc=nFail=0;

	
	lua_pushlightuserdata(pState, (void*)hvcClient);
	lua_gettable(pState, LUA_REGISTRYINDEX);
	hVirtualCloudClient=(void*)lua_touserdata(pState,-1);

	/*set arguements of postIO*/
	int FuncID=13250;
	int Location=3;
	int Sync=2;
	char DriverName[]="CloudServerB";
	char DeviceName[]="CloudServerB";
	unsigned int Object=0;
	int MajorCmd=4;
	int MinorCmd=0;
	unsigned char *pInBuffer;
	unsigned int InBufferLen=0;
	unsigned char *pOutBuffer=NULL;
	unsigned int OutBufferLen=0;
	unsigned char *pErrorInfo=NULL;
	unsigned int ErrorInfoLen=0;
	unsigned int ErrorCode=0;
	IOState StateCode=IO_STATE_NONE;
	DWORD nHandle=0;
	char outstr[1000];
	ZeroMemory(outstr,1000);

	/*fill in the request data */
	queryCapitalRequest *data=new queryCapitalRequest;
	queryCapitalRequest_call *req=data->add_requests();
	req->set_accountid(-1);
	InBufferLen=data->ByteSize();
	pInBuffer=(unsigned char*)malloc(data->ByteSize());
	data->SerializeWithCachedSizesToArray(pInBuffer);

	lua_State *usrdata=pState;
	/*post the request and wait for callback */
	CTime t;
	t=CTime::GetCurrentTime();
	if(!PostIO(hVirtualCloudClient, FuncID, Location, Sync, DriverName, DeviceName, Object, MajorCmd, MinorCmd, pInBuffer, InBufferLen, (PIOCOMP)CallBack_Query_capital, usrdata ,300000))
	{
		printf("%02d%02d%02d: PostIO  Request:%s Error\n",t.GetHour(),t.GetMinute(),t.GetSecond(),pInBuffer);
	}
//	delete data;
	return lua_yield(pState,0);
}

void LuaClass::CallBack_Query_capital(CCHANDLE pSender, IOPCompleteArgs *pIOP)
{
	queryCapitalResponse response;
	response.ParsePartialFromArray(pIOP->pOutBuffer,pIOP->OutBufferLen);
	queryCapitalResponse_result result=response.responses(0);
	queryCapitalResponse_result_capital capital=result.capitals(0);
	lua_State *L=(lua_State*)(pIOP->UserData);
	lua_pushboolean(L,true);
	lua_pushinteger(L,capital.id());
	lua_pushinteger(L,capital.exchangeid());
	lua_pushinteger(L,capital.currencytype());
	lua_pushinteger(L,capital.amounts());
	lua_pushinteger(L,capital.freeamounts());
	lua_pushinteger(L,capital.frozenamount());
	lua_resume(L,8);
}

