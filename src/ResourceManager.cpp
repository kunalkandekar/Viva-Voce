#include "ResourceManager.h"

ResourceManager::ResourceManager(int init, int size) {
	memoQ =  new SyncQueue();
	this->initCount = init;
	this->size = size;
	this->total = 0;
}

ResourceManager::~ResourceManager() {
	clear();
	delete memoQ;
}

int ResourceManager::init(void) {
	Memo* memo;
	for(total=0;
		total < initCount;
		total++) {

		memo = new Memo();
		if(memo==NULL) {
			//printf("\nOut of memory!! %p ",memo);
			return -1;
		}
		memo->id = total;
		memo->size = size;
		memo->bytes = (char*)malloc(memo->size);
		memoQ->enQ(memo);
	}
	return total;
}

int ResourceManager::clear(void) {
	Memo* memo;
	int emptied=0;
	while((memo=(Memo*)memoQ->deQ())) {
		if(memo->bytes)
			free(memo->bytes);
		delete memo;
		emptied++;
	}
	return emptied;
}

int ResourceManager::downsize(void) {
	Memo* memo;
	int emptied=0;
	int ret;
	if(memoQ->count() <=initCount * 2)
		return 0;
	emptied = memoQ->count()/20;
	ret = emptied;
	while(emptied) {
		memo=(Memo*)memoQ->deQ();
		if(memo) {
			if(memo->bytes)
				free(memo->bytes);
			delete memo;
			total--;
			emptied--;
		}
		else
			break;
	}
	return ret;
}

int 	ResourceManager::getTotal(void) {
	return total;
}
Memo* 	ResourceManager::allocMemo(void) {
	Memo* memo=(Memo*)memoQ->deQ();
	if(memo==NULL) {
		memo = new Memo();
		memo->id = total;
		memo->size = size;
		memo->bytes = (char*)malloc(memo->size);
		//(char*)malloc(memo->size);
		//if(size > 750) printf("\nNew memo---%p ",memo);
		total++;
	}
	return memo;
}

//not implemented
Memo* 	ResourceManager::allocMemo(int type) {
	return NULL;
}

void 	ResourceManager::releaseMemo(Memo* memo) {
	if(memo!=NULL) {
		memoQ->enQ(memo);
	}
}

int		ResourceManager::getNMemosInReserve(void) {
	return memoQ->count();
}
