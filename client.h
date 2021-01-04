/*
	Name	- K.K.P De Alwis
	Reg.No	- IT18106848
	Name	- D.N Munasinghe
	Reg.No	- It18204162
	Module	- Network Programming
	Batch	- 3rd Year June Intake
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ISVALIDSOCKET(s) ((s)>=0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

