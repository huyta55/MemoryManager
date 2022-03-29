#include "MemoryManager.h"

using namespace std;

// Destructor
MemoryManager::~MemoryManager() {
	delete[] memoryStart;
	memBlocks.clear();
	holesVector.clear();
}
// Constructors
MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void*)> allocator) {
	this->wordSize = wordSize;
	this->allocator = allocator;
	this->memoryStart = nullptr;
}
// Initialize
void MemoryManager::initialize(size_t sizeInWords) {
	// deleting old pointer to clean up old allocated memory before allocating new memory with new
	delete[] memoryStart;
	this->memoryStart = new char[sizeInWords * wordSize];
	this->memoryLimit = sizeInWords;
	// adding the blocks to the holes Vector filled with 0s;
	for (int i = 0; i < sizeInWords; i++) {
		this->holesVector.push_back(0);
	}
}
// Shutdown
void MemoryManager::shutdown() {
	// MAKE A LOOP TO FREE EACH BLOCK IN THE MEMBLOCKS VECTOR
	memBlocks.clear();
	holesVector.clear();
}

// Allocate
void* MemoryManager::allocate(size_t sizeInBytes) {
	// if size required is too big, return nullptr
	if (sizeInBytes > getMemoryLimit()) {
		return nullptr;
	}
	size_t sizeInWords = (size_t)ceil((double)sizeInBytes / (double)getWordSize());
	// else we try to find the offset to calculate how many bytes away we are from the start of the memory chunk
	uint16_t* list = (uint16_t*)getList();
	int offset = allocator(sizeInWords, list);
	delete[] list;
	// if the offset < 0, then there is no space in the memory so return nullptr
	if (offset < 0) {
		return nullptr;
	}
	// Then use the offset to add sizeInWords amounts of memoryBlocks and point its address to (getMemoryStart() + offset * wordSize)
	void* newAddress = this->memoryStart + ((size_t) (offset * getWordSize()));
	// Adding the new MemoryBlock node to the memBlocks array
	smallBlock.address = newAddress;
	smallBlock.blockSize = sizeInWords;
	smallBlock.offsetLength = offset;
	this->memBlocks.push_back(smallBlock);
	// Updates the holes vector with a loop
	for (int i = 0; i < sizeInWords; ++i) {
		// flipping the binary at the indexes that the words takes up in memory with a 1 to indicate allocated memory
		this->holesVector[offset + i] = 1;
	}
	return newAddress;
}

// Free
void MemoryManager::free(void* address) {
	// loop through the vectory of memBlocks and if a block has the same address, frees it.
	int offset = 0;
	int size = 0;
	for (int i = 0; i < this->memBlocks.size(); i++) {
		if (this->memBlocks[i].address == address) {
			offset = this->memBlocks[i].offsetLength;
			size = this->memBlocks[i].blockSize;
			this->memBlocks.erase(memBlocks.begin() + i);
		}
	}
	// loop through the holes vector and updates the holes
	for (int i = 0; i < size; ++i) {
		this->holesVector[offset + i] = 0;
	}
}

// set Function
void MemoryManager::setAllocator(std::function<int(int, void*)> allocator) {
	this->allocator = allocator;
}

// get Functions
void* MemoryManager::getList() {
	vector<uint16_t> listVector;
	// if the array of words in memory is empty, then return the default list
	if (this->memBlocks.empty()) {
		return nullptr;
	}
	int holeCount = 0;
	int holeSize = 0;
	int holeStart = 0;

	// Loop through the holes vector and create listVector with the desired format
	for (int i = 0; i < this->holesVector.size(); ++i) {
		// if at a hole, add it to the currentHole or create a newHole
		if (this->holesVector[i] == 0) {
			if (holeSize++ == 0) {
				holeStart = i;
			}
			// check if this is the last block in memory
			if (i == this->holesVector.size() - 1) {
				listVector.push_back(holeStart);
				listVector.push_back(holeSize);
				holeCount++;
				holeStart = -1;
				holeSize = 0;
			}
		}
		// else if at an allocated block, then add the hole to the holesVector, then increase holes count
		else if ((holeStart >= 0) && (holeSize > 0)) {
			listVector.push_back(holeStart);
			listVector.push_back(holeSize);
			holeCount++;
			holeStart = -1;
			holeSize = 0;
		}
	}
	// Add the number of holes onto the front of the holes Vector after the loop
	listVector.insert(listVector.begin(), holeCount);
	uint16_t* holes = new uint16_t[listVector.size()];
	for (int i = 0; i < listVector.size(); ++i) {
		uint16_t current = listVector[i];
		holes[i] = current;
	}
	listVector.clear();
	return holes;
}
void* MemoryManager::getBitmap() {
	// loop right to left on the holes vector and every 16 bits, translate to hex and store it in a bitmap
	uint16_t size = (uint16_t)ceil((double)holesVector.size() / 8.0);
	uint16_t* bitMap = new uint16_t[size + 1];
	uint16_t bitCount = 0;
	uint16_t decimal = 0;
	uint16_t index = 1;
	for (int i = 0; i < holesVector.size(); ++i) {
		// Add to decimal if the current word in memory is allocated
		if (holesVector[i]) {
			decimal += (uint16_t)pow(2.0, bitCount);
		}
		// check that the binary string is not over 8 bits or if it's at the end
		if ((bitCount == 15) || (i == holesVector.size() - 1)) {
			bitMap[index] = decimal;
			index++;
			bitCount = 0;
			decimal = 0;
			continue;
		}
		bitCount++;
	}
	bitMap[0] = size;

	return bitMap;
}
int MemoryManager::dumpMemoryMap(char* fileName) {
	// if there's no holes, then return -1;
	vector<int> listVector;
	int holeCount = 0;
	int holeSize = 0;
	int holeStart = 0;

	// Loop through the holes vector and create listVector with the desired format
	for (int i = 0; i < this->holesVector.size(); ++i) {
		// if at a hole, add it to the currentHole or create a newHole
		if (this->holesVector[i] == 0) {
			if (holeSize++ == 0) {
				holeStart = i;
			}
			// check if this is the last block in memory
			if (i == this->holesVector.size() - 1) {
				listVector.push_back(holeStart);
				listVector.push_back(holeSize);
				holeCount++;
				holeStart = -1;
				holeSize = 0;
			}
		}
		// else if at an allocated block, then add the hole to the holesVector, then increase holes count
		else if ((holeStart >= 0) && (holeSize > 0)) {
			listVector.push_back(holeStart);
			listVector.push_back(holeSize);
			holeCount++;
			holeStart = -1;
			holeSize = 0;
		}
	}
	string holesString = "";
	// iterate though the vector and make the hole string
	for (int i = 1; i < listVector.size(); i += 2) {
		holesString += "[";
		holesString += to_string(listVector[i - 1]);
		holesString += ", ";
		holesString += to_string(listVector[i]);
		holesString += "]";
		if (i + 2 < listVector.size()) {
			holesString += " - ";
		}
	}
	// clearing the vector after use to avoid memory leaks
	listVector.clear();
	int file = open(fileName, O_WRONLY | O_CREAT | O_TRUNC , S_IROTH | S_IRUSR | S_IWUSR | S_IRGRP);
	// if file doesn't open
	if (file == -1) {
		close(file);
		return -1;
	}
	// POSIX System Calls
	char* buffer = new char[holesString.length()];
	// Iterate through holes string and copy over characters into the buffer
	for (int i = 0; i < holesString.length(); ++i) {
		buffer[i] = holesString.at(i);
	}
	// Calling write
	write(file, buffer, holesString.length());
	// Calling close to close the file
	close(file);
	// deleting memory to prevent memory leak
	delete[] buffer;
	// return 0 for success;
	
	return 0;
}
// The 3 easy get functions
unsigned MemoryManager::getWordSize() {
	return this->wordSize;
}
void* MemoryManager::getMemoryStart() {
	return this->memoryStart;
}
unsigned MemoryManager::getMemoryLimit() {
	return this->memoryLimit * this->wordSize;
}

// Algorithms
int bestFit(int sizeInWords, void* list) {
	if (list == nullptr) {
		return 0;
	}
	// Go through the sizeInWords (memSize) if the list of memory blocks and track what the smallest hole possible is
	int minHole = -1;
	int minIndex = -1;
	uint16_t* holes = (uint16_t*)list;
	int size = holes[0];
	// if size == 0, there is no fit, so return -1
	if (size == 0) {
		return -1;
	}
	// else if number of holes == 1, then just return the hole if it fits 
	else if (size == 1) {
		return holes[1];
	}
	// iterate through the list of uint16_ts for info about the holes and determine the smallest one that fits
	for (int i = 2; i < holes[0] * 2; i += 2) {
		// if the difference between the value and the sizeInWords is < than current Min and >= 0; or if minHole < 0 and the current hole >= 0; then make it the current Min
		int currentHoleSize = holes[i] - sizeInWords;
		int minHoleSize = minHole - sizeInWords;
		if (((currentHoleSize < minHoleSize) && (currentHoleSize >= 0)) || ((currentHoleSize >= 0) && (minHoleSize < 0))) {
			minHole = holes[i];
			minIndex = i - 1;
		}
	}
	// At the end of the loop, if minHole is still < 0, then that means there no space for the word, so return -1;
	if (minHole < 0) {
		return -1;
	}
	// else return the minHole(wordOffset) of the hole
	else {
		return holes[minIndex];
	}
}
int worstFit(int sizeInWords, void* list) {
	if (list == nullptr) {
		return 0;
	}
	// Go through the sizeInWords (memSize) of the list of memory blocks and track what the biggest hole possible is
	int maxHole = -1;
	int maxIndex = -1;
	uint16_t* holes = (uint16_t*)list;
	int size = holes[0];
	// if size == 0, there is no fit, so return -1
	if (size == 0) {
		return -1;
	}
	// else if number of holes == 1, then just return the hole if it fits 
	else if (size == 1) {
		return holes[1];
	}
	// iterate through the list of uint16_ts for info about the holes and determine the smallest one that fits
	for (int i = 2; i < holes[0] * 2; i += 2) {
		// if the difference between the value and sizeInWords is > than currentMax, and actually fits the sizeInWords then make it the currentMax
		int currentHoleSize = holes[i] - sizeInWords;
		int maxHoleSize = maxHole - sizeInWords;
		if ((currentHoleSize > maxHoleSize) && (currentHoleSize >= 0)); {
			maxHole = holes[i];
			maxIndex = i - 1;
		}
	}
	// if after the loop the maxHole is still -1, then that means there are no space for the word, so return -1;
	if (maxHole < 0) {
		return -1;
	}
	// else return the maxHole(wordOffset) of the hole
	else {
		return holes[maxIndex];
	}
}
