#pragma once
#include "2Box.h"

namespace RpcServer
{
	int SRPC_API AcptEvent(void* instance,int type,void* usr);

	//int SRPC_API ConnErr(void* instance,void* usr);

	//int MODULE_API HookCompleted( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr);

	int MODULE_API OnNewProcess( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr);

	int MODULE_API OnNewWnd( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr);

	int MODULE_API OnEmbedWnd( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr);

	int MODULE_API OnActiveWnd( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr);

	int MODULE_API OnGetAllWnd(const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr);

	//int MODULE_API IsProcessInBox(const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr);

	//int MODULE_API InitAllRemoteDlls(const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr);
};
