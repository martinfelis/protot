#pragma once

#include "SimpleMath/SimpleMath.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cstring>
#include <sys/stat.h>

struct WriteSerializer {
	enum { IsReading = 0 };
	enum { IsWriting = 1 };

	std::ofstream stream;

	bool SerializeData (const std::string &key, const char *data, size_t size) {
		size_t key_length = key.size();
		stream.write (reinterpret_cast<char*>(&key_length), sizeof (size_t));
		stream << key;
		stream.write (reinterpret_cast<char*>(&size), sizeof(size_t));
		stream.write (data, size);

		return true;
	}

	void Open(const char* filename) {
		stream.open(filename, std::ofstream::trunc);
	}

	void Close() {
		stream.close();
	}
};

struct ReadSerializer {
	enum { IsReading = 1 };
	enum { IsWriting = 0 };

	struct Block {
		size_t size;
		void *pdata;

		Block() : size(0), pdata(nullptr) {}
		Block(const Block& other) :
			size(other.size),
			pdata(other.pdata)
		{}
		Block& operator=(const Block& other) {
			if (this != &other) {
				size = other.size;
				pdata = other.pdata;
			}
			return *this;
		}
	};

	bool SerializeData (const std::string &key, char *data, size_t size) {
		std::unordered_map<std::string, Block>::iterator iter;
		iter = blocks.find(key);
		if (iter == blocks.end()) {
			return false;
		}

		std::cout << "found block for " << key << ", size: " << iter->second.size << std::endl;
		assert (size == iter->second.size);
		std::cout << "copying from " << iter->second.pdata << " to " << (void*) data << std::endl; 
		std::cout << "copying from " << blocks[key].pdata << " to " << (void*) data << std::endl; 
		assert (blocks[key].size == size);
		memcpy (data, blocks[key].pdata, size);
		return true;
	}

	void Open(const char* filename) {
		// early out if file does not (yet) exist
		struct stat fstat;
		if (stat(filename, &fstat) == -1) {
			return;
		}

		std::ifstream stream(filename, std::ios::binary);

		size_t key_size;
		size_t block_size;

		while (!stream.eof()) {
			// read key size
			stream.read(reinterpret_cast<char*>(&key_size), sizeof(size_t));
			std::cout << "read key size " << key_size << std::endl;
			assert (key_size < 1000);
			// read key
			std::string key (key_size, 0);
			stream.read(&key[0], key_size);

			// create block
			blocks[key] = Block();
			Block *block = &blocks[key];

			// read block size
			stream.read(reinterpret_cast<char*>(&block->size), sizeof(size_t));
			std::cout << "read block " << key << ", size = " << block->size << std::endl;
			block->pdata = new char[block->size + 1];
			std::cout << "block addr = " << block->pdata << std::endl;
			stream.read(reinterpret_cast<char*>(block->pdata), block->size);
		}

		stream.close();
}

	void Close() {
	}

	std::unordered_map<std::string, Block> blocks;
};

template <typename Serializer>
bool SerializeBool (Serializer &serializer, const std::string &key, bool& value) {
	return serializer.SerializeData(key, reinterpret_cast<char*>(&value), sizeof(bool));
}

template <typename Serializer>
bool SerializeInt (Serializer &serializer, const std::string &key, int& value) {
	return serializer.SerializeData(key, reinterpret_cast<char*>(&value), sizeof(int));
}

template <typename Serializer>
bool SerializedUint16 (Serializer &serializer, const std::string &key, uint16_t& value) {
	return serializer.SerializeData(key, reinterpret_cast<char*>(&value), sizeof(uint16_t));
}

template <typename Serializer>
bool SerializeVec3 (Serializer &serializer, const std::string &key, SimpleMath::Vector3f& value) {
	return serializer.SerializeData(key, reinterpret_cast<char*>(&value), sizeof(SimpleMath::Vector3f));
}
