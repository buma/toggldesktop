//
// NamedEvent_UNIX.cpp
//
// $Id: //poco/1.4/Foundation/src/NamedEvent_UNIX.cpp#2 $
//
// Library: Foundation
// Package: Processes
// Module:  NamedEvent
//
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/NamedEvent_UNIX.h"
#include "Poco/Exception.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX)
#include <semaphore.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#endif


namespace Poco {


#if (POCO_OS == POCO_OS_LINUX) || (POCO_OS == POCO_OS_CYGWIN)
	union semun
	{
		int                 val;
		struct semid_ds*    buf;
		unsigned short int* array;
		struct seminfo*     __buf;
	};
#elif (POCO_OS == POCO_OS_HPUX)
	union semun
	{
		int              val;
		struct semid_ds* buf;
		ushort*          array;
	};
#endif


NamedEventImpl::NamedEventImpl(const std::string& name):
	_name(name)
{
	std::string fileName = getFileName();
#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX)
	_sem = sem_open(fileName.c_str(), O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO, 0);
	if ((long) _sem == (long) SEM_FAILED) 
		throw SystemException("cannot create named event (sem_open() failed)", _name);
#else
	int fd = open(fileName.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd != -1)
		close(fd);
	else 
		throw SystemException("cannot create named event (lockfile)", _name);
	key_t key = ftok(fileName.c_str(), 0);
	if (key == -1)
		throw SystemException("cannot create named event (ftok() failed)", _name);
	_semid = semget(key, 1, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | IPC_CREAT | IPC_EXCL);
	if (_semid >= 0)
	{
		union semun arg;
		arg.val = 0;
		semctl(_semid, 0, SETVAL, arg);
	}
	else if (errno == EEXIST)
	{
		_semid = semget(key, 1, 0);
	}
	else throw SystemException("cannot create named event (semget() failed)", _name);
#endif // defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX)
}


NamedEventImpl::~NamedEventImpl()
{
#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX)
	sem_close(_sem);
#endif
}


void NamedEventImpl::setImpl()
{
#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX)
	if (sem_post(_sem) != 0)
	   	throw SystemException("cannot set named event", _name);
#else
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op  = 1;
	op.sem_flg = 0;
	if (semop(_semid, &op, 1) != 0)
	   	throw SystemException("cannot set named event", _name);
#endif
}


void NamedEventImpl::waitImpl()
{
#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX)
	int err;
	do
	{
		err = sem_wait(_sem);
	}
	while (err && errno == EINTR);
	if (err) throw SystemException("cannot wait for named event", _name);
#else
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op  = -1;
	op.sem_flg = 0;
	int err;
	do
	{
		err = semop(_semid, &op, 1);
	}
	while (err && errno == EINTR); 
	if (err) throw SystemException("cannot wait for named event", _name);
#endif
}


std::string NamedEventImpl::getFileName()
{
#if defined(sun) || defined(__APPLE__) || defined(__QNX__)
	std::string fn = "/";
#else
	std::string fn = "/tmp/";
#endif
	fn.append(_name);
	fn.append(".event");
	return fn;
}


} // namespace Poco