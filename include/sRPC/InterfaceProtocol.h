/** @file InterfaceProtocol.h
 *  @brief 接口开发约定;
 *
 *  @author maowei3
 *  @date 2016/01/26
 *
 *  @note 历史记录;
 *  @note V1.0.0.0 创建;
 */
#ifndef _INTERFACE_PROTOCOL__
#define _INTERFACE_PROTOCOL__

#if (defined(_WIN32) || defined(_WIN64))
#  define MODULE_EXTERN extern "C" __declspec(dllexport)
#  define MODULE_API __stdcall
#elif defined(__linux__)
#  define MODULE_EXTERN extern "C"
#  define MODULE_API
#else
#  define MODULE_EXTERN
#  define MODULE_API
#endif

#include "sRPCTypeDef.h"

/** @fn typedef int (MODULE_API *Module_Init)()
 *  @brief 初始化模块;
 *  @param szPath [in] 模块目录;
 *  @return 成功返回0，否则返回其他值;
 */
typedef int (MODULE_API *Module_Init)(const char* szPath);

/** @fn typedef int (MODULE_API *Module_Uninit)()
 *  @brief 反初始化模块;
 *  @param void
 *  @return 成功返回0，否则返回其他值;
 */
typedef int (MODULE_API *Module_Uninit)();

//************************************
// Method:    Module_Proc
// Brief:  	  功能过程;
// Returns:   int		
// Parameter: void* data_in				数据（参数）;
// Parameter: unsigned int size_in		数据大小;
// Parameter: COMPLETIONPROC proc		完成回调;
// Parameter: void* usr					框架内部数据，需要用户传给proc;
//note:
// 注意：一般情况下，proc 和 usr 只能在该函数之内使用以同步返回结果！;除非用户返回100;
// 这意味着，如果该函数返回了100，框架内部结构usr的生存权将转移给用户，其不会被释放直到用户调用了proc;
//************************************
typedef int (MODULE_API *Module_Proc)( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr);


//----------- 模块库可提供远程调用的过程地址; ------------//
/*
 *
 * 模块内部可以定义 Module_Proc 形式的过程地址;
 *
 * 使用 MODULE_MAKE_EXPORT_PROC_BEGIN、EXPORT_PROC、MODULE_MAKE_EXPORT_PROC_END 系列宏向外暴露过程函数;
 *
 */

/** @struct _PROCINFO
 *  @brief 过程信息表;
 */
typedef struct _PROCINFO
{
	char			szKey[MODULE_KEY_MAXLENTH];
	Module_Proc		pAddr;
}PROCINFO;

/** @fn typedef int (MODULE_API *Module_RegisterProc)(const PROCINFO* procedures,unsigned int count,void* usr);
 *  @brief 注册过程信息;
 *  @param const PROCINFO* [in]		过程信息表;
 *  @param unsigned int [in]		注册数量;
 *  @param void* [in]				用户数据;
 *  @return 成功返回0，否则返回其他值;
 */
typedef int (MODULE_API *Module_RegisterProc)(const PROCINFO* procedures,unsigned int count,void* usr);

/** @fn typedef int (MODULE_API *Module_GetAllExportProc)(Module_FnInfo fnReg);
 *  @brief 获取所有导出的过程地址;
 *  @param Module_RegisterProc [in] 过程注册地址;
 *  @param void* [in]				用户数据;
 *  @return 成功返回0，否则返回其他值;
 */
typedef int (MODULE_API *Module_GetAllExportProc)(Module_RegisterProc regAddr,void* usr);

// 使用该系列宏可以导出过程地址;
#define MODULE_MAKE_EXPORT_PROC_BEGIN(GetAllExportProcName) \
int MODULE_API GetAllExportProcName(Module_RegisterProc regAddr,void* usr) \
{ \
	if(regAddr) \
	{\
		const PROCINFO proc_info_t[] = \
		{
#define EXPORT_PROC(key,proc) \
			{key,proc},
#define MODULE_MAKE_EXPORT_PROC_END() \
			{NULL,NULL} \
		};\
		unsigned int count = sizeof(proc_info_t) / sizeof(PROCINFO);\
		return regAddr(proc_info_t,count,usr);\
	}\
	return -1;\
}

//----------- 模块库必须导出的接口; ------------//
/*
 *
 * 模块内部必须实现 _MODULE_EXPORT_ADDR_T 定义的一系列接口;
 * 框架通过这些接口与模块进行交互;
 *
 * 模块库必须导出下面声明的接口，框架通过该接口识别动态库是否为合法模块;
 * MODULE_EXTERN int MODULE_API MODULE_GetMEAT(Module_ExportAddr eFunc,void* usr)
 *
 * 模块库可使用 MODULE_MAKE_EXPORT_ADDR_T 宏来导出 MODULE_GetMEAT接口;
 *
 */

/** @struct _MODULE_EXPORT_ADDR_T
 *  @brief 模块与框架交互的接口地址表;
 */
typedef struct _MODULE_EXPORT_ADDR_T
{
    Module_Init					Init;			///< 模块初始化接口地址;
    Module_Uninit				Uninit;			///< 模块反初始化接口地址;
	Module_GetAllExportProc		GetAllProc;		///< 获取所有暴露过程接口地址;
}MEAT;

/** @fn typedef int (MODULE_API *Module_ExportAddr)(MEAT* pMEAT);
 *  @brief 导出接口地址表;
 *  @param MEAT* [in]  地址表;
 *  @param void* [in]  用户数据;
 *  @return 成功返回0，否则返回其他值;
 */
typedef int (MODULE_API *Module_ExportAddr)(const MEAT* pMEAT,void* usr);

/** @fn typedef int (MODULE_API *Module_GetMEAT)();
 *  @brief 获取模块与框架交互的接口地址表;
 *  @param Module_ExportAddr [in]  导出接口地址表的函数地址;
 *  @param void* [in]  用户数据;
 *  @return 成功返回0，否则返回其他值;
 */
typedef int (*Module_GetMEAT)(Module_ExportAddr eFunc,void* usr);

// 使用该宏可以声明并实现模块必须导出的接口 MODULE_GetMEAT;
#define MODULE_MAKE_EXPORT_ADDR_T(Init, Uninit,GetAllProc) \
MODULE_EXTERN int MODULE_GetMEAT(Module_ExportAddr eFunc,void* usr) \
{ \
	if(eFunc) \
	{\
		const MEAT export_addr_t = { \
			Init, \
			Uninit, \
			GetAllProc \
		}; \
		return eFunc(&export_addr_t,usr);\
	}\
    return -1; \
}


#endif // #ifndef _INTERFACE_PROTOCOL__