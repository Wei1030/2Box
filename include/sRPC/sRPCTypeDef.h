#ifndef _SRPC_TYPE_DEF_
#define _SRPC_TYPE_DEF_

#if (defined(_WIN32) || defined(_WIN64))
#  if defined(SRPC_STATICLIB)
#	 define SRPC_EXTERN
#  elif defined(SRPC_EXPORTS)
#    define SRPC_EXTERN extern "C" __declspec(dllexport)
#  else
#    define SRPC_EXTERN extern "C" __declspec(dllimport)
#  endif
#  define SRPC_API __stdcall
#elif defined(__linux__)
#  define SRPC_EXTERN extern "C"
#  define SRPC_API
#else
#  define SRPC_EXTERN
#  define SRPC_API
#endif

#define MODULE_KEY_MAXLENTH			64

//////////////////////////////////////////////////////////////////////////
//ö�ٶ���;
enum CodecRet
{
	Codec_Error = -1,	//����������������;
	Codec_Complete,		//��������;
	Codec_Continue		//���ݲ���;
};

enum CodecType
{
	Codec_Unknown,
	Codec_Request,	//����;
	Codec_Response	//�ظ�;
};

enum FdType
{
	FD_TCP,
	FD_UDP,
	FD_LOCAL  //named pipe on windows, AF_UNIX socket on linux
};

//////////////////////////////////////////////////////////////////////////
//�ṹ�嶨��;
//�û��Զ����Э��������������Ϣ;
//����������Ϣ;
typedef struct _REQUEST_CODEC
{
	//description : ��������;
	//����[in]: Զ�̹��̣����������ƣ���SRPC_Call�Ƚӿڵ�method����;
	//����[out]:ָ�������Ѿ���¶�Ĺ��̣�����������;
	char description[MODULE_KEY_MAXLENTH];

	//seq �� ��ţ�Ψһ��ʶһ��δ��ɵ�Զ�̵���;
	//����[in]: ���ֶ��Զ�����;
	//����[out]:ָ��Ϊclient����ʱ��ֵ;
	unsigned long long seq;		

}REQUEST_CODEC,SERVER_DECODEINFO,CLIENT_ENCODEINFO;

//�ظ��������Ϣ;
typedef struct _RESPONSE_CODEC
{
	//code : �������;
	//����[out]: ���ֶλᱻ���ݸ�COMPLETIONPROC��err����;
	//����[in]: ���ֶ�ΪCOMPLETIONPROC��errֵ;
	//			��COMPLETIONPROCû�б����ã���Ϊ���̺���Module_Proc�ķ���ֵ;
	//			��Module_Proc����ǰ�ͷ����������ڲ�������������ֵΪ-1;
	unsigned int code;						

	//seq �� ���;
	//����[out]:Ψһ��ʶһ��δ��ɵ�Զ�̵���;
	//����[in]: server����ʱ��õ�ֵ;
	unsigned long long seq;

}RESPONSE_CODEC,SERVER_ENCODEINFO,CLIENT_DECODEINFO;

typedef struct _CODECINFO
{
	//tag_size : _CODECINFO��С;
	//���ڰ汾����;
	unsigned int tag_size;

	//type ����;req or rsp;
	//����:[in];
	//����:[out];
	CodecType type;

	//src_size : ���ݳ���;
	//���� : [in],�����ݳ���;
	//���� : [in,out];
	//[in]: �Ѿ����յ������ܳ���;
	//[out]:���ر��ν���������ܴ�С;		
	//		�����ν������ݴ�С��δ֪�������ݲ��㣬��Ҫ����Codec_Continue,���ֶλ�ʹ���Ԥ���뱾�ν�����Ҫ���ڴ�����������;
	//		���: �����δ�С��δ֪�����Է�������ֵ;
	//			  ����֪��С�����ݲ���,��ָ����ֵΪ���ν�������������ܴ�С;
	unsigned int src_size;

	//src
	//���� : [in],������;
	//���� : [in],����������;
	char* src;

	union CODEC_TYPE
	{
		REQUEST_CODEC req;
		RESPONSE_CODEC rsp;
	}codec;

}CODECINFO;

//////////////////////////////////////////////////////////////////////////
//�����嶨��;
 
/** @brief ���ӶϿ�֪ͨ;
 *	@param instance  �ͻ���ʵ��;
 *	@param usr		 �û�����;
 *  @return �ɹ�����0��ʧ�ܷ�������ֵ;
 *	@note 

	���ص�����ʱ��;:
		1��Զ�̷���˹ر����ӻ�����쳣;
		2�����������Ͽ����ӣ�����SRPC_DestroyClient��;
		
		*���¼�����ʱ���û������SRPC_DestroyClient�ر��Ѿ���Ч��instance;
		*�����¼��������û���������SRPC_DestroyClient��������,�û��Ծɿ��Ե���SRPC_DestroyClient�رտͻ���;
		��ʱ����п��ܷ���ʧ�ܣ�������������κθ���Ӱ��;

 */
typedef int (SRPC_API *CONNERRPROC)(void* instance,void* usr);

/** @brief �����¼�֪ͨ;
 *	@param instance  ������������;
 *	@param type		 �¼�����;
 *	@param usr		 �û�����;
 *  @return �ɹ�����0��ʧ�ܷ�������ֵ;
 *	@note 

	���ص�����ʱ��;:
		1���пͻ�������;type = 0
		2��Զ�̿ͻ��������رջ�����쳣���µ����ӶϿ�;type = 1
		3�����������رշ��񣨵���SRPC_DestroyServer��;type = 1
		
		*���¼�����ʱ��instanceΪ����������������;
		�û������ܶԸ�instance�����κβ���;
 */
typedef int (SRPC_API *ACPTEVENTPROC)(void* instance,int type,void* usr);

//************************************
// Method:    CODECPROC
// Brief:  	  ����������ص�����ܣ�������ENCODEIMPL��DECODEIMPL�лص�����֧���첽�ص�;
// Returns:   int						�ɹ�����0��ʧ�ܷ���-1;
// Parameter: const char* result		�����������;
// Parameter: unsigned int size			���ݴ�С;
// Parameter: void* usr					�û�����;
//************************************
typedef int (SRPC_API *CODECPROC)(const char* result, unsigned int size, void* usr);

//************************************
// Method:    ENCODEIMPL
// Brief:  	  ����;
// Returns:   CodecRet					 ����ɹ�����Codec_Complete��ʧ�ܷ���Codec_Error;
// Parameter: LPENCODE_INFO lpEncodeInfo ������������Ϣ;
// Parameter: CODECPROC cb				 ����ص�;
// Parameter: void* usr					 �û�����;
//************************************
typedef CodecRet(SRPC_API *ENCODEIMPL)(const CODECINFO* pEncodeInfo, CODECPROC cb, void* usr);

//************************************
// Method:    DECODEIMPL
// Brief:  	  ����;
// Returns:   CodecRet					 �ɹ�����Codec_Complete��ʧ�ܷ���Codec_Error�������ݲ��㣬�뷵��Codec_Continue�������յ��������ݺ󣬸ûص����ٴα�����;
// Parameter: LPDECODE_INFO lpDecodeInfo ������������Ϣ;
// Parameter: CODECPROC cb				 ����ص�;
// Parameter: void* usr					 �û�����;
//************************************
typedef CodecRet(SRPC_API *DECODEIMPL)(CODECINFO* pDecodeInfo, CODECPROC cb, void* usr);

/** @brief ���ܽӿڵ������֪ͨ;
 *  @param error [in] ������;
 *  @param data [in] ���ݽ��;
 *  @param size [in]  ���ݳ���;
 *  @param usr [in]  �û�����;
 *  @return �ɹ�����0��ʧ�ܷ�������ֵ;
 */
typedef int (SRPC_API *COMPLETIONPROC)(unsigned int error,const void* data,unsigned int size,void* usr);

/** @brief �̳߳ص�ִ���塣��ThreadPoolWork
 *  @param usr [in]  �û�����;
 *  @return
 */
typedef void* (SRPC_API* WORKERPROC)( void* usr);

#endif //_SRPC_TYPE_DEF_