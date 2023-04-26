/*
MIT License

Copyright (c) 2018 Meng Rao <raomeng1@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#include <atomic>

// alloc/push/front/pop are atomic operations, which is crash safe for shared-memory IPC
template <uint32_t Bytes>
class SPSCVarQueue {
public:
	static constexpr uint32_t BLK_CNT = Bytes / 64;
	static_assert(BLK_CNT && !(BLK_CNT & (BLK_CNT - 1)),
	              "BLK_CNT must be a power of 2");

	struct MsgHeader {
		// size of this msg, including header itself
		// auto set by lib, can be read by user
		uint16_t size;
		uint16_t msg_type;
		// userdata can be used by caller, e.g. save timestamp or other stuff
		// we assume that user_msg is 8 types alligned so there'll be 4 bytes padding anyway, otherwise we can choose to
		// eliminate userdata
		uint32_t userdata;
	};

	MsgHeader* alloc(uint16_t size) {
		size += sizeof(MsgHeader);
		uint32_t blk_sz = (size + sizeof(Block) - 1) / sizeof(Block);
		uint32_t padding_sz = BLK_CNT - (write_idx % BLK_CNT);
		bool rewind = blk_sz > padding_sz;
		// min_read_idx could be a negtive value which results in a large unsigned int
		uint32_t min_read_idx =
		    write_idx + blk_sz + (rewind ? padding_sz : 0) - BLK_CNT;
		if ((int)(read_idx_cach - min_read_idx) < 0) {
			asm volatile("" : "=m"(read_idx) : :); // force read memory
			read_idx_cach = read_idx;
			if ((int)(read_idx_cach - min_read_idx) < 0) { // no enough space
				return nullptr;
			}
		}
		if (rewind) {
			blk[write_idx % BLK_CNT].header.size = 0;
			asm volatile("" : : "m"(blk), "m"(write_idx) :); // memory fence
			write_idx += padding_sz;
		}
		MsgHeader& header = blk[write_idx % BLK_CNT].header;
		header.size = size;
		return &header;
	}

	void push() {
		asm volatile("" : : "m"(blk), "m"(write_idx) :); // memory fence
		uint32_t blk_sz =
		    (blk[write_idx % BLK_CNT].header.size + sizeof(Block) - 1) /
		    sizeof(Block);
		write_idx += blk_sz;
		asm volatile("" : : "m"(write_idx) :); // force write memory
	}

	template <typename Writer>
	bool tryPush(uint16_t size, Writer writer) {
		MsgHeader* header = alloc(size);
		if (!header)
			return false;
		writer(header);
		push();
		return true;
	}

	template <typename Writer>
	void blockPush(uint16_t size, Writer writer) {
		while (!tryPush(size, writer))
			;
	}

	MsgHeader* front() {
		asm volatile("" : "=m"(write_idx), "=m"(blk) : :); // force read memory
		if (read_idx == write_idx) {
			return nullptr;
		}
		uint16_t size = blk[read_idx % BLK_CNT].header.size;
		if (size == 0) { // rewind
			read_idx += BLK_CNT - (read_idx % BLK_CNT);
			if (read_idx == write_idx) {
				return nullptr;
			}
		}
		return &blk[read_idx % BLK_CNT].header;
	}

	void pop() {
		asm volatile("" : "=m"(blk) : "m"(read_idx) :); // memory fence
		uint32_t blk_sz =
		    (blk[read_idx % BLK_CNT].header.size + sizeof(Block) - 1) /
		    sizeof(Block);
		read_idx += blk_sz;
		asm volatile("" : : "m"(read_idx) :); // force write memory
	}

	template <typename Reader>
	bool tryPop(Reader reader) {
		MsgHeader* header = front();
		if (!header)
			return false;
		reader(header);
		pop();
		return true;
	}

private:
	struct Block // size of 64, same as cache line
	{
		alignas(64) MsgHeader header;
	} blk[BLK_CNT] = {};

	alignas(128) uint32_t write_idx = 0;
	uint32_t read_idx_cach = 0; // used only by writing thread

	alignas(128) uint32_t read_idx = 0;
};
