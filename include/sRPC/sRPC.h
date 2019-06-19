/** @file sRPC.h
 *  @note 
 *  @brief 简易rpc框架;
 *
 *  @author maowei
 *  @date 2016/01/22
 *
 *  @note 历史记录;
 *  @note V1.0.0.0 创建;
 */
#ifndef _SRPC_H__
#define _SRPC_H__

#include "InterfaceProtocol.h"
#include "sRPCTypeDef.h"

 
/** @brief 初始化;
 *  @param FN_INFO* [in] 接口信息;
 *  @param nFnCount [in] 接口个数;
 *  @return 成功返回0，失败返回其他值;
 *	@note 执行模块调用即可，被加载模块无需调用;
 */
SRPC_EXTERN int SRPC_API SRPC_Init(const PROCINFO* procs,unsigned int count);

// 使用该系列宏可以初始化sRPC;
#define SRPC_INIT_BEGIN() \
	const PROCINFO srpc_proc_info_t[] = \
	{
#define EXPORT_PROC(key,proc) \
		{key,proc},
#define SRPC_INIT_END() \
		{NULL,NULL} \
	};\
	unsigned int srpc_proc_count = sizeof(srpc_proc_info_t) / sizeof(PROCINFO);\
	int srpc_ret = SRPC_Init(srpc_proc_info_t,srpc_proc_count);

#define SRPC_INIT_OK	0 == srpc_ret
#define SRPC_INIT_FAIL	0 != srpc_ret

/*  @brief 反初始化;
 *  @param void
 *  @return void
 *	@note 执行模块调用即可，被加载模块无需调用;

	 反初始化工作顺序;
	 1、销毁所有客户端实例;
	 2、销毁所有服务端实例;
	 3、销毁默认线程池;
	 4、调用所有模块库的反初始化接口;
 */
SRPC_EXTERN void SRPC_API SRPC_Uninit();

/** @brief 加载配置文件;
 *  @param cfgXmlPath[in] 配置文件路径;
 *  @param initPipeServer[in] 是否启动内部管道服务;
 *  @return 成功返回0，失败返回其他值;
 *	@note 执行模块调用即可，被加载模块无需调用;
 *		  该接口会加载所有配置文件中的模块，并根据initPipeServer参数是否启动内部的pipe服务;
 *		  若该执行模块需要为其他执行模块服务（在其他执行模块的配置文件中），则initPipeServer需传入非0;
 */
SRPC_EXTERN int SRPC_API SRPC_InitFrame(const char* cfgXmlPath,int initPipeServer = 0);

//************************************
// Method:    SRPC_CreateCodec
// Brief:  	  创建编解码器，用户可自定义交互协议;
// Returns:   SRPC_EXTERN void*			编解码器实例（若创建失败则返回NULL）;	
// Parameter: ENCODEIMPL encoder		编码实现回调;
// Parameter: DECODEIMPL decoder		解码实现回调;
//************************************
SRPC_EXTERN void* SRPC_API SRPC_CreateCodec(ENCODEIMPL encoder, DECODEIMPL decoder);

//************************************
// Method:    SRPC_DestroyCodec
// Brief:  	  销毁编解码器;
// Returns:   SRPC_EXTERN int	
// Parameter: void * codec				编解码器实例;
//************************************
SRPC_EXTERN int SRPC_API SRPC_DestroyCodec(void* codec);

//************************************
// Method:    SRPC_SetConnErrCb
// Brief:     设置连接异常通知回调;
// Parameter: void* instance			保留;传NULL;
// Parameter: CONNERRPROC cb			回调地址;
// Parameter: void * usr				用户参数;
//	@note 
//		当作为客户端连接（调用SRPC_CreateProvider或SRPC_CreateClient）后，连接出现异常时,cb会被调用;
//************************************
SRPC_EXTERN void SRPC_API SRPC_SetConnErrCb(void* instance,CONNERRPROC cb,void* usr);

//************************************
// Method:    SRPC_SetAcptErrCb
// Brief:     设置连接通知回调;
// Parameter: void* instance			保留;传NULL;
// Parameter: ACPTERRPROC cb			回调地址;
// Parameter: void * usr				用户参数;
//	@note 
//		当作为服务端时（SRPC_CreateServer）;
//		1、有客户端连接时，cb会被调用;
//		2、有客户端连接出现异常时,cb会被调用;
//		3、主动关闭服务（SRPC_DestroyServer）时,cb会被调用;
//************************************
SRPC_EXTERN void SRPC_API SRPC_SetAcptEventCb(void* instance,ACPTEVENTPROC cb,void* usr);

//************************************
// Method:    SRPC_CreateProvider
// Brief:  	  创建子进程，并连接其管道;
// Returns:   SRPC_EXTERN void*			客户端实例（若创建失败则返回NULL）;
// Parameter: const char * mkey			子进程key，由配置文件配置;
// Parameter: const char * name_out		输出参数，管道名;
// Parameter: int  out_size				name_out的长度;
// Parameter: void * codec				编解码器;
//************************************
SRPC_EXTERN void* SRPC_API SRPC_CreateProvider(const char* mkey,char* name_out, int out_size,void* codec = 0);

//************************************
// Method:    SRPC_CreateClient
// Brief:  	  创建客户端;
// Returns:   SRPC_EXTERN void*			客户端实例（若创建失败则返回NULL）;
// Parameter: RpcType rpc_type			客户端类型;
// Parameter: const char * chDest		服务端地址或pipe名称;
// Parameter: unsigned int iDest		服务端端口;
// Parameter: void * codec				编解码器;
//************************************
SRPC_EXTERN void* SRPC_API SRPC_CreateClient(FdType fd_type,const char* chDest,unsigned int iDest, void* codec = 0);

//************************************
// Method:    SRPC_DestroyClient
// Brief:  	  销毁客户端（断开与服务端的连接）;
// Returns:   SRPC_EXTERN int 
// Parameter: void * instance			客户端实例;
//************************************
SRPC_EXTERN int SRPC_API SRPC_DestroyClient(void* instance);

//************************************
// Method:    SRPC_AsyncCall
// Brief:  	  异步过程调用;
// Returns:   SRPC_EXTERN int			成功返回0，失败返回-1;
// Parameter: void * instance			客户端实例，若为NULL，则调用本地接口;
// Parameter: const char * method		方法名;
// Parameter: const void * data			数据（参数）;
// Parameter: unsigned int size			数据大小;
// Parameter: COMPLETIONPROC fn			完成回调（远程过程调用结果回调）;
// Parameter: void * usr				fn回调用户数据;
// Parameter: int timeout				超时时间(ms)  -1：无限等待;
//************************************
SRPC_EXTERN int SRPC_API SRPC_AsyncCall(void* instance, 
										const char* method, 
										const void* data, 
										unsigned int size, 
										COMPLETIONPROC fn,
										void* usr,
										int timeout = 30000);

//************************************
// Method:    SRPC_Call
// Brief:  	  同步过程调用;
// Returns:   SRPC_EXTERN int			成功返回0，失败返回-1;
// Parameter: void * instance			客户端实例，若为NULL，则调用本地接口;
// Parameter: const char * method		方法名;
// Parameter: const void * data			数据（参数）;
// Parameter: unsigned int size			数据大小;
// Parameter: COMPLETIONPROC fn			完成回调（远程过程调用结果回调）;
// Parameter: void * usr				fn回调用户数据;
// Parameter: int timeout				超时时间(ms)  -1：无限等待，若instance==NULL，该参数被忽略;
//note:
//注意:当instance为NULL时，框架只是直接调用本进程对应的接口;
//由于对应接口实现者并不关心调用方所在，假设该接口被设计为只能异步回复（函数返回了100，参考InterfaceProtocol.h头文件中的Module_Proc的注意事项);
//此时该函数会返回100，这意味着该接口不支持同步调用，这时请特别注意|usr|的生存期，因为fn会在之后的任何时候都有可能被调用;
//（再次提醒：仅当instance为NULL时需要注意接口是否支持同步调用）;
//************************************
SRPC_EXTERN int SRPC_API SRPC_Call(void* instance,
								   const char* method,
								   const void* data,
								   unsigned int size, 
								   COMPLETIONPROC fn, 
								   void* usr,
								   int timeout = 30000);

//////////////////////////////////////////////////////////////////////////
//线程池相关;

//************************************
// Method:    SRPC_CreateThreadPool
// Brief:  	  创建线程池;
// Returns:   SRPC_EXTERN void*			线程池实例（若创建失败则返回NULL）;
// Parameter: int thread_min			最小线程数量(即初始线程数，若小于等于0，则自动根据CPU核数创建线程);
// Parameter: int thread_max			最大线程数量（若小于thread_min，则线程数量不会扩展）;
// Parameter: int timeout				扩展线程超时时间ms（扩展线程在timeout时间内若没有任务需处理，则自动退出；若timeout为-1，则扩展线程不会自动退出）;
//	@note 
//		不建议创建多余的线程池，可直接调用SRPC_ThreadPoolWork，使用框架内部线程池执行函数;
//************************************
SRPC_EXTERN void* SRPC_API SRPC_CreateThreadPool(int thread_min = 0, int thread_max = 512,int timeout = 30000);

//************************************
// Method:    SRPC_DestroyThreadPool
// Brief:  	  销毁线程池;
// Returns:   SRPC_EXTERN int			
// Parameter: void * pool				线程池实例;
//************************************
SRPC_EXTERN int	SRPC_API SRPC_DestroyThreadPool( void* pool );

//************************************
// Method:    SRPC_ThreadPoolWork
// Brief:  	  使池中空闲线程开始执行func;
// Returns:   SRPC_EXTERN int			成功返回0，失败返回-1;
// Parameter: void * tpool				线程池实例;
// Parameter: WORKERPROC func			回调函数地址;
// Parameter: void * usr				回调用户参数;
//	@note 
//		若tpool为NULL,该接口会向框架默认池中的线程抛送func调用任务;
//
//************************************
SRPC_EXTERN int		SRPC_API	SRPC_ThreadPoolWork( void* tpool,WORKERPROC func,void* usr);

//////////////////////////////////////////////////////////////////////////
//rpc server;

//************************************
// Method:    SRPC_CreateServer
// Brief:  	  创建服务端;
// Returns:   SRPC_EXTERN void*			服务端实例（若创建失败则返回NULL）;
// Parameter: RpcType rpc_type			服务类型;
// Parameter: const char * name			服务地址或名称;
// Parameter: unsigned int port			服务端口;
// Parameter: void * codec				编解码器;
//************************************
SRPC_EXTERN void*	SRPC_API	SRPC_CreateServer(FdType fd_type, const char* name, unsigned int port,void* codec = 0);

//************************************
// Method:    SRPC_DestroyServer
// Brief:  	  销毁服务端;
// Returns:   SRPC_EXTERN void	
// Parameter: void * server				服务端实例;
//************************************
SRPC_EXTERN void	SRPC_API	SRPC_DestroyServer(void* server);

#endif // #ifndef _SRPC_H__