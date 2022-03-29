#pragma once
#include <functional>
#include <vector>
#include <algorithm>
#include <string>
#include <stdio.h> 
#include <fcntl.h>
#include <math.h>
#include <unistd.h>

using namespace std;
int bestFit(int sizeInWords, void* list);
int worstFit(int sizeInWords, void* list);
struct MemoryBlock {
	size_t blockSize;
	unsigned offsetLength;
	void* address = nullptr;
};
class MemoryManager {
public:
	MemoryManager(unsigned wordSize, std::function<int(int, void*)> allocator);
	~MemoryManager();
	void initialize(size_t sizeInWords);
	void shutdown();
	void* allocate(size_t sizeInBytes);
	void free(void* address);
	void setAllocator(std::function<int(int, void*)> allocator);
	int dumpMemoryMap(char* fileName);
	void* getList();
	void* getBitmap();
	unsigned getWordSize();
	void* getMemoryStart();
	unsigned getMemoryLimit();
private:
	vector<MemoryBlock> memBlocks;
	MemoryBlock smallBlock;
	vector<bool> holesVector;
	unsigned wordSize;
	std::function<int(int, void*)> allocator;
	char* memoryStart = nullptr;
	size_t memoryLimit;
};
