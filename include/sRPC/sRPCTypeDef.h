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
//枚举定义;
enum CodecRet
{
	Codec_Error = -1,	//编解码出错，数据有误;
	Codec_Complete,		//编解码完成;
	Codec_Continue		//数据不足;
};

enum CodecType
{
	Codec_Unknown,
	Codec_Request,	//请求;
	Codec_Response	//回复;
};

enum FdType
{
	FD_TCP,
	FD_UDP,
	FD_LOCAL  //named pipe on windows, AF_UNIX socket on linux
};

//////////////////////////////////////////////////////////////////////////
//结构体定义;
//用户自定义的协议必须包含以下信息;
//请求编解码信息;
typedef struct _REQUEST_CODEC
{
	//description : 过程名称;
	//编码[in]: 远程过程（方法）名称，即SRPC_Call等接口的method参数;
	//解码[out]:指定本地已经暴露的过程（方法）名称;
	char description[MODULE_KEY_MAXLENTH];

	//seq ： 序号，唯一标识一次未完成的远程调用;
	//编码[in]: 该字段自动生成;
	//解码[out]:指定为client请求时的值;
	unsigned long long seq;		

}REQUEST_CODEC,SERVER_DECODEINFO,CLIENT_ENCODEINFO;

//回复编解码信息;
typedef struct _RESPONSE_CODEC
{
	//code : 错误代码;
	//解码[out]: 此字段会被传递给COMPLETIONPROC的err参数;
	//编码[in]: 此字段为COMPLETIONPROC的err值;
	//			若COMPLETIONPROC没有被调用，则为过程函数Module_Proc的返回值;
	//			若Module_Proc调用前就发生的其他内部错误情况，则此值为-1;
	unsigned int code;						

	//seq ： 序号;
	//解码[out]:唯一标识一次未完成的远程调用;
	//编码[in]: server解码时获得的值;
	unsigned long long seq;

}RESPONSE_CODEC,SERVER_ENCODEINFO,CLIENT_DECODEINFO;

typedef struct _CODECINFO
{
	//tag_size : _CODECINFO大小;
	//用于版本兼容;
	unsigned int tag_size;

	//type 类型;req or rsp;
	//编码:[in];
	//解码:[out];
	CodecType type;

	//src_size : 数据长度;
	//编码 : [in],裸数据长度;
	//解码 : [in,out];
	//[in]: 已经接收的数据总长度;
	//[out]:返回本次解码所需的总大小;		
	//		若本次交互数据大小还未知或者数据不足，需要返回Codec_Continue,该字段会使框架预申请本次交互需要的内存来接收数据;
	//		因此: 若本次大小还未知，可以返回输入值;
	//			  若已知大小但数据不足,可指定此值为本次交互数据所需的总大小;
	unsigned int src_size;

	//src
	//编码 : [in],裸数据;
	//解码 : [in],待解析数据;
	char* src;

	union CODEC_TYPE
	{
		REQUEST_CODEC req;
		RESPONSE_CODEC rsp;
	}codec;

}CODECINFO;

//////////////////////////////////////////////////////////////////////////
//函数体定义;
 
/** @brief 连接断开通知;
 *	@param instance  客户端实例;
 *	@param usr		 用户数据;
 *  @return 成功返回0，失败返回其他值;
 *	@note 

	本回调发生时机;:
		1、远程服务端关闭连接或出现异常;
		2、本地主动断开连接（调用SRPC_DestroyClient）;
		
		*该事件发生时，用户需调用SRPC_DestroyClient关闭已经无效的instance;
		*若该事件是由于用户主动调用SRPC_DestroyClient而产生的,用户仍旧可以调用SRPC_DestroyClient关闭客户端;
		这时其很有可能返回失败，但并不会产生任何负面影响;

 */
typedef int (SRPC_API *CONNERRPROC)(void* instance,void* usr);

/** @brief 连接事件通知;
 *	@param instance  保留，无意义;
 *	@param type		 事件类型;
 *	@param usr		 用户数据;
 *  @return 成功返回0，失败返回其他值;
 *	@note 

	本回调发生时机;:
		1、有客户端连接;type = 0
		2、远程客户端主动关闭或出现异常导致的连接断开;type = 1
		3、本地主动关闭服务（调用SRPC_DestroyServer）;type = 1
		
		*该事件发生时的instance为保留参数，无意义;
		用户绝不能对该instance进行任何操作;
 */
typedef int (SRPC_API *ACPTEVENTPROC)(void* instance,int type,void* usr);

//************************************
// Method:    CODECPROC
// Brief:  	  将编解码结果回调给框架，必须在ENCODEIMPL或DECODEIMPL中回调，不支持异步回调;
// Returns:   int						成功返回0，失败返回-1;
// Parameter: const char* result		编解码结果数据;
// Parameter: unsigned int size			数据大小;
// Parameter: void* usr					用户数据;
//************************************
typedef int (SRPC_API *CODECPROC)(const char* result, unsigned int size, void* usr);

//************************************
// Method:    ENCODEIMPL
// Brief:  	  编码;
// Returns:   CodecRet					 编码成功返回Codec_Complete，失败返回Codec_Error;
// Parameter: LPENCODE_INFO lpEncodeInfo 待编码数据信息;
// Parameter: CODECPROC cb				 结果回调;
// Parameter: void* usr					 用户数据;
//************************************
typedef CodecRet(SRPC_API *ENCODEIMPL)(const CODECINFO* pEncodeInfo, CODECPROC cb, void* usr);

//************************************
// Method:    DECODEIMPL
// Brief:  	  解码;
// Returns:   CodecRet					 成功返回Codec_Complete，失败返回Codec_Error，若数据不足，请返回Codec_Continue，当接收到更多数据后，该回调会再次被调用;
// Parameter: LPDECODE_INFO lpDecodeInfo 待解码数据信息;
// Parameter: CODECPROC cb				 结果回调;
// Parameter: void* usr					 用户数据;
//************************************
typedef CodecRet(SRPC_API *DECODEIMPL)(CODECINFO* pDecodeInfo, CODECPROC cb, void* usr);

/** @brief 功能接口调用完成通知;
 *  @param error [in] 错误码;
 *  @param data [in] 数据结果;
 *  @param size [in]  数据长度;
 *  @param usr [in]  用户数据;
 *  @return 成功返回0，失败返回其他值;
 */
typedef int (SRPC_API *COMPLETIONPROC)(unsigned int error,const void* data,unsigned int size,void* usr);

/** @brief 线程池的执行体。见ThreadPoolWork
 *  @param usr [in]  用户数据;
 *  @return
 */
typedef void* (SRPC_API* WORKERPROC)( void* usr);

#endif //_SRPC_TYPE_DEF_