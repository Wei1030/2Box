/** @file sRPC.h
 *  @note 
 *  @brief ����rpc���;
 *
 *  @author maowei
 *  @date 2016/01/22
 *
 *  @note ��ʷ��¼;
 *  @note V1.0.0.0 ����;
 */
#ifndef _SRPC_H__
#define _SRPC_H__

#include "InterfaceProtocol.h"
#include "sRPCTypeDef.h"

 
/** @brief ��ʼ��;
 *  @param FN_INFO* [in] �ӿ���Ϣ;
 *  @param nFnCount [in] �ӿڸ���;
 *  @return �ɹ�����0��ʧ�ܷ�������ֵ;
 *	@note ִ��ģ����ü��ɣ�������ģ���������;
 */
SRPC_EXTERN int SRPC_API SRPC_Init(const PROCINFO* procs,unsigned int count);

// ʹ�ø�ϵ�к���Գ�ʼ��sRPC;
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

/*  @brief ����ʼ��;
 *  @param void
 *  @return void
 *	@note ִ��ģ����ü��ɣ�������ģ���������;

	 ����ʼ������˳��;
	 1���������пͻ���ʵ��;
	 2���������з����ʵ��;
	 3������Ĭ���̳߳�;
	 4����������ģ���ķ���ʼ���ӿ�;
 */
SRPC_EXTERN void SRPC_API SRPC_Uninit();

/** @brief ���������ļ�;
 *  @param cfgXmlPath[in] �����ļ�·��;
 *  @param initPipeServer[in] �Ƿ������ڲ��ܵ�����;
 *  @return �ɹ�����0��ʧ�ܷ�������ֵ;
 *	@note ִ��ģ����ü��ɣ�������ģ���������;
 *		  �ýӿڻ�������������ļ��е�ģ�飬������initPipeServer�����Ƿ������ڲ���pipe����;
 *		  ����ִ��ģ����ҪΪ����ִ��ģ�����������ִ��ģ��������ļ��У�����initPipeServer�贫���0;
 */
SRPC_EXTERN int SRPC_API SRPC_InitFrame(const char* cfgXmlPath,int initPipeServer = 0);

//************************************
// Method:    SRPC_CreateCodec
// Brief:  	  ��������������û����Զ��彻��Э��;
// Returns:   SRPC_EXTERN void*			�������ʵ����������ʧ���򷵻�NULL��;	
// Parameter: ENCODEIMPL encoder		����ʵ�ֻص�;
// Parameter: DECODEIMPL decoder		����ʵ�ֻص�;
//************************************
SRPC_EXTERN void* SRPC_API SRPC_CreateCodec(ENCODEIMPL encoder, DECODEIMPL decoder);

//************************************
// Method:    SRPC_DestroyCodec
// Brief:  	  ���ٱ������;
// Returns:   SRPC_EXTERN int	
// Parameter: void * codec				�������ʵ��;
//************************************
SRPC_EXTERN int SRPC_API SRPC_DestroyCodec(void* codec);

//************************************
// Method:    SRPC_SetConnErrCb
// Brief:     ���������쳣֪ͨ�ص�;
// Parameter: void* instance			����;��NULL;
// Parameter: CONNERRPROC cb			�ص���ַ;
// Parameter: void * usr				�û�����;
//	@note 
//		����Ϊ�ͻ������ӣ�����SRPC_CreateProvider��SRPC_CreateClient�������ӳ����쳣ʱ,cb�ᱻ����;
//************************************
SRPC_EXTERN void SRPC_API SRPC_SetConnErrCb(void* instance,CONNERRPROC cb,void* usr);

//************************************
// Method:    SRPC_SetAcptErrCb
// Brief:     ��������֪ͨ�ص�;
// Parameter: void* instance			����;��NULL;
// Parameter: ACPTERRPROC cb			�ص���ַ;
// Parameter: void * usr				�û�����;
//	@note 
//		����Ϊ�����ʱ��SRPC_CreateServer��;
//		1���пͻ�������ʱ��cb�ᱻ����;
//		2���пͻ������ӳ����쳣ʱ,cb�ᱻ����;
//		3�������رշ���SRPC_DestroyServer��ʱ,cb�ᱻ����;
//************************************
SRPC_EXTERN void SRPC_API SRPC_SetAcptEventCb(void* instance,ACPTEVENTPROC cb,void* usr);

//************************************
// Method:    SRPC_CreateProvider
// Brief:  	  �����ӽ��̣���������ܵ�;
// Returns:   SRPC_EXTERN void*			�ͻ���ʵ����������ʧ���򷵻�NULL��;
// Parameter: const char * mkey			�ӽ���key���������ļ�����;
// Parameter: const char * name_out		����������ܵ���;
// Parameter: int  out_size				name_out�ĳ���;
// Parameter: void * codec				�������;
//************************************
SRPC_EXTERN void* SRPC_API SRPC_CreateProvider(const char* mkey,char* name_out, int out_size,void* codec = 0);

//************************************
// Method:    SRPC_CreateClient
// Brief:  	  �����ͻ���;
// Returns:   SRPC_EXTERN void*			�ͻ���ʵ����������ʧ���򷵻�NULL��;
// Parameter: RpcType rpc_type			�ͻ�������;
// Parameter: const char * chDest		����˵�ַ��pipe����;
// Parameter: unsigned int iDest		����˶˿�;
// Parameter: void * codec				�������;
//************************************
SRPC_EXTERN void* SRPC_API SRPC_CreateClient(FdType fd_type,const char* chDest,unsigned int iDest, void* codec = 0);

//************************************
// Method:    SRPC_DestroyClient
// Brief:  	  ���ٿͻ��ˣ��Ͽ������˵����ӣ�;
// Returns:   SRPC_EXTERN int 
// Parameter: void * instance			�ͻ���ʵ��;
//************************************
SRPC_EXTERN int SRPC_API SRPC_DestroyClient(void* instance);

//************************************
// Method:    SRPC_AsyncCall
// Brief:  	  �첽���̵���;
// Returns:   SRPC_EXTERN int			�ɹ�����0��ʧ�ܷ���-1;
// Parameter: void * instance			�ͻ���ʵ������ΪNULL������ñ��ؽӿ�;
// Parameter: const char * method		������;
// Parameter: const void * data			���ݣ�������;
// Parameter: unsigned int size			���ݴ�С;
// Parameter: COMPLETIONPROC fn			��ɻص���Զ�̹��̵��ý���ص���;
// Parameter: void * usr				fn�ص��û�����;
// Parameter: int timeout				��ʱʱ��(ms)  -1�����޵ȴ�;
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
// Brief:  	  ͬ�����̵���;
// Returns:   SRPC_EXTERN int			�ɹ�����0��ʧ�ܷ���-1;
// Parameter: void * instance			�ͻ���ʵ������ΪNULL������ñ��ؽӿ�;
// Parameter: const char * method		������;
// Parameter: const void * data			���ݣ�������;
// Parameter: unsigned int size			���ݴ�С;
// Parameter: COMPLETIONPROC fn			��ɻص���Զ�̹��̵��ý���ص���;
// Parameter: void * usr				fn�ص��û�����;
// Parameter: int timeout				��ʱʱ��(ms)  -1�����޵ȴ�����instance==NULL���ò���������;
//note:
//ע��:��instanceΪNULLʱ�����ֻ��ֱ�ӵ��ñ����̶�Ӧ�Ľӿ�;
//���ڶ�Ӧ�ӿ�ʵ���߲������ĵ��÷����ڣ�����ýӿڱ����Ϊֻ���첽�ظ�������������100���ο�InterfaceProtocol.hͷ�ļ��е�Module_Proc��ע������);
//��ʱ�ú����᷵��100������ζ�Ÿýӿڲ�֧��ͬ�����ã���ʱ���ر�ע��|usr|�������ڣ���Ϊfn����֮����κ�ʱ���п��ܱ�����;
//���ٴ����ѣ�����instanceΪNULLʱ��Ҫע��ӿ��Ƿ�֧��ͬ�����ã�;
//************************************
SRPC_EXTERN int SRPC_API SRPC_Call(void* instance,
								   const char* method,
								   const void* data,
								   unsigned int size, 
								   COMPLETIONPROC fn, 
								   void* usr,
								   int timeout = 30000);

//////////////////////////////////////////////////////////////////////////
//�̳߳����;

//************************************
// Method:    SRPC_CreateThreadPool
// Brief:  	  �����̳߳�;
// Returns:   SRPC_EXTERN void*			�̳߳�ʵ����������ʧ���򷵻�NULL��;
// Parameter: int thread_min			��С�߳�����(����ʼ�߳�������С�ڵ���0�����Զ�����CPU���������߳�);
// Parameter: int thread_max			����߳���������С��thread_min�����߳�����������չ��;
// Parameter: int timeout				��չ�̳߳�ʱʱ��ms����չ�߳���timeoutʱ������û�������账�����Զ��˳�����timeoutΪ-1������չ�̲߳����Զ��˳���;
//	@note 
//		�����鴴��������̳߳أ���ֱ�ӵ���SRPC_ThreadPoolWork��ʹ�ÿ���ڲ��̳߳�ִ�к���;
//************************************
SRPC_EXTERN void* SRPC_API SRPC_CreateThreadPool(int thread_min = 0, int thread_max = 512,int timeout = 30000);

//************************************
// Method:    SRPC_DestroyThreadPool
// Brief:  	  �����̳߳�;
// Returns:   SRPC_EXTERN int			
// Parameter: void * pool				�̳߳�ʵ��;
//************************************
SRPC_EXTERN int	SRPC_API SRPC_DestroyThreadPool( void* pool );

//************************************
// Method:    SRPC_ThreadPoolWork
// Brief:  	  ʹ���п����߳̿�ʼִ��func;
// Returns:   SRPC_EXTERN int			�ɹ�����0��ʧ�ܷ���-1;
// Parameter: void * tpool				�̳߳�ʵ��;
// Parameter: WORKERPROC func			�ص�������ַ;
// Parameter: void * usr				�ص��û�����;
//	@note 
//		��tpoolΪNULL,�ýӿڻ�����Ĭ�ϳ��е��߳�����func��������;
//
//************************************
SRPC_EXTERN int		SRPC_API	SRPC_ThreadPoolWork( void* tpool,WORKERPROC func,void* usr);

//////////////////////////////////////////////////////////////////////////
//rpc server;

//************************************
// Method:    SRPC_CreateServer
// Brief:  	  ���������;
// Returns:   SRPC_EXTERN void*			�����ʵ����������ʧ���򷵻�NULL��;
// Parameter: RpcType rpc_type			��������;
// Parameter: const char * name			�����ַ������;
// Parameter: unsigned int port			����˿�;
// Parameter: void * codec				�������;
//************************************
SRPC_EXTERN void*	SRPC_API	SRPC_CreateServer(FdType fd_type, const char* name, unsigned int port,void* codec = 0);

//************************************
// Method:    SRPC_DestroyServer
// Brief:  	  ���ٷ����;
// Returns:   SRPC_EXTERN void	
// Parameter: void * server				�����ʵ��;
//************************************
SRPC_EXTERN void	SRPC_API	SRPC_DestroyServer(void* server);

#endif // #ifndef _SRPC_H__