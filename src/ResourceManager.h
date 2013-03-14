#ifndef __RESOURCEMANAGER_H
#define __RESOURCEMANAGER_H

#include "SyncQueue.h"

//#pragma pack(1)

typedef struct Memo {
	short id;
	char code;
	int data1;
	int data2;
	char* bytes;
	int size;
} Memo;

//#pragma pack(4)

class ResourceManager { //:public Thread{
private:
	SyncQueue	*memoQ;
	int 	total;
	int 	size;
	int		initCount;

public:
	ResourceManager(int init, int size);
	~ResourceManager();
	int		*go;

	int 	init(void);
	int 	clear(void);
	int		getTotal(void);
	Memo* 	allocMemo(void);
	Memo* 	allocMemo(int type);

	void 	releaseMemo(Memo* memo);
	int		getNMemosInReserve(void);
	int 	downsize(void);
	//void	*run(void*);
};
#endif
