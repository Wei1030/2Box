/** @file InterfaceProtocol.h
 *  @brief �ӿڿ���Լ��;
 *
 *  @author maowei3
 *  @date 2016/01/26
 *
 *  @note ��ʷ��¼;
 *  @note V1.0.0.0 ����;
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
 *  @brief ��ʼ��ģ��;
 *  @param szPath [in] ģ��Ŀ¼;
 *  @return �ɹ�����0�����򷵻�����ֵ;
 */
typedef int (MODULE_API *Module_Init)(const char* szPath);

/** @fn typedef int (MODULE_API *Module_Uninit)()
 *  @brief ����ʼ��ģ��;
 *  @param void
 *  @return �ɹ�����0�����򷵻�����ֵ;
 */
typedef int (MODULE_API *Module_Uninit)();

//************************************
// Method:    Module_Proc
// Brief:  	  ���ܹ���;
// Returns:   int		
// Parameter: void* data_in				���ݣ�������;
// Parameter: unsigned int size_in		���ݴ�С;
// Parameter: COMPLETIONPROC proc		��ɻص�;
// Parameter: void* usr					����ڲ����ݣ���Ҫ�û�����proc;
//note:
// ע�⣺һ������£�proc �� usr ֻ���ڸú���֮��ʹ����ͬ�����ؽ����;�����û�����100;
// ����ζ�ţ�����ú���������100������ڲ��ṹusr������Ȩ��ת�Ƹ��û����䲻�ᱻ�ͷ�ֱ���û�������proc;
//************************************
typedef int (MODULE_API *Module_Proc)( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr);


//----------- ģ�����ṩԶ�̵��õĹ��̵�ַ; ------------//
/*
 *
 * ģ���ڲ����Զ��� Module_Proc ��ʽ�Ĺ��̵�ַ;
 *
 * ʹ�� MODULE_MAKE_EXPORT_PROC_BEGIN��EXPORT_PROC��MODULE_MAKE_EXPORT_PROC_END ϵ�к����Ⱪ¶���̺���;
 *
 */

/** @struct _PROCINFO
 *  @brief ������Ϣ��;
 */
typedef struct _PROCINFO
{
	char			szKey[MODULE_KEY_MAXLENTH];
	Module_Proc		pAddr;
}PROCINFO;

/** @fn typedef int (MODULE_API *Module_RegisterProc)(const PROCINFO* procedures,unsigned int count,void* usr);
 *  @brief ע�������Ϣ;
 *  @param const PROCINFO* [in]		������Ϣ��;
 *  @param unsigned int [in]		ע������;
 *  @param void* [in]				�û�����;
 *  @return �ɹ�����0�����򷵻�����ֵ;
 */
typedef int (MODULE_API *Module_RegisterProc)(const PROCINFO* procedures,unsigned int count,void* usr);

/** @fn typedef int (MODULE_API *Module_GetAllExportProc)(Module_FnInfo fnReg);
 *  @brief ��ȡ���е����Ĺ��̵�ַ;
 *  @param Module_RegisterProc [in] ����ע���ַ;
 *  @param void* [in]				�û�����;
 *  @return �ɹ�����0�����򷵻�����ֵ;
 */
typedef int (MODULE_API *Module_GetAllExportProc)(Module_RegisterProc regAddr,void* usr);

// ʹ�ø�ϵ�к���Ե������̵�ַ;
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

//----------- ģ�����뵼���Ľӿ�; ------------//
/*
 *
 * ģ���ڲ�����ʵ�� _MODULE_EXPORT_ADDR_T �����һϵ�нӿ�;
 * ���ͨ����Щ�ӿ���ģ����н���;
 *
 * ģ�����뵼�����������Ľӿڣ����ͨ���ýӿ�ʶ��̬���Ƿ�Ϊ�Ϸ�ģ��;
 * MODULE_EXTERN int MODULE_API MODULE_GetMEAT(Module_ExportAddr eFunc,void* usr)
 *
 * ģ����ʹ�� MODULE_MAKE_EXPORT_ADDR_T �������� MODULE_GetMEAT�ӿ�;
 *
 */

/** @struct _MODULE_EXPORT_ADDR_T
 *  @brief ģ�����ܽ����Ľӿڵ�ַ��;
 */
typedef struct _MODULE_EXPORT_ADDR_T
{
    Module_Init					Init;			///< ģ���ʼ���ӿڵ�ַ;
    Module_Uninit				Uninit;			///< ģ�鷴��ʼ���ӿڵ�ַ;
	Module_GetAllExportProc		GetAllProc;		///< ��ȡ���б�¶���̽ӿڵ�ַ;
}MEAT;

/** @fn typedef int (MODULE_API *Module_ExportAddr)(MEAT* pMEAT);
 *  @brief �����ӿڵ�ַ��;
 *  @param MEAT* [in]  ��ַ��;
 *  @param void* [in]  �û�����;
 *  @return �ɹ�����0�����򷵻�����ֵ;
 */
typedef int (MODULE_API *Module_ExportAddr)(const MEAT* pMEAT,void* usr);

/** @fn typedef int (MODULE_API *Module_GetMEAT)();
 *  @brief ��ȡģ�����ܽ����Ľӿڵ�ַ��;
 *  @param Module_ExportAddr [in]  �����ӿڵ�ַ��ĺ�����ַ;
 *  @param void* [in]  �û�����;
 *  @return �ɹ�����0�����򷵻�����ֵ;
 */
typedef int (*Module_GetMEAT)(Module_ExportAddr eFunc,void* usr);

// ʹ�øú����������ʵ��ģ����뵼���Ľӿ� MODULE_GetMEAT;
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