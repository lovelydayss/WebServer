// come from https://github.com/MengRao/SPSC_Queue

#pragma once
#include <atomic>

// T 为队列中数据元素类型，CNT 为队列容量大小
template <class T, uint32_t CNT>
class SPSCQueue {
public:
	// 保证队列容量为 2 的 n 次幂
	// static_assert 编译期检查是否满足条件
	static_assert(CNT && !(CNT & (CNT - 1)), "CNT must be a power of 2");

	// 分配空间
	// 此时生产者可以对该块内存进行操作
	// 但由于其未修改写指针，故不会对消费者产生影响
	T* alloc() {
		if (write_idx - read_idx_cach == CNT) {

			// 内部进行再一次确认，防止未更新状态
			read_idx_cach = ((std::atomic<uint32_t>*)&read_idx)
			                    ->load(std::memory_order_consume);

			// 队列空间不足情况，分支预测
			if (__builtin_expect(write_idx - read_idx_cach == CNT, 0)) {
				return nullptr;
			}
		}

		// 返回当前数据应该使用地址
		return &data[write_idx % CNT];
	}

	// 将此前分配空间的数据添加到队列中
	// 即使用原子操作对写指针进行调整，表示该块可用
	void push() {
		((std::atomic<uint32_t>*)&write_idx)
		    ->store(write_idx + 1, std::memory_order_release);
	}

	// 尝试插入
	// ... 没理解 Writer 是啥意思
	// 使用的时候还是尽量用 alloc + push
	template <typename Writer>
	bool tryPush(Writer writer) {
		T* p = alloc();
		if (!p)
			return false;
		writer(p);
		push();
		return true;
	}

	// 插入多个块 ?
	template <typename Writer>
	void blockPush(Writer writer) {
		while (!tryPush(writer))
			;
	}

	// 当前队列头部
	//
	T* front() {

		// 队列空情况
		// 这里为什么不使用分支预测优化？
		if (__builtin_expect(
		        (read_idx == ((std::atomic<uint32_t>*)&write_idx)
		                         ->load(std::memory_order_acquire)),
		        0)) {
			return nullptr;
		}

		// 此处应该操作数据块指针
		return &data[read_idx % CNT];
	}

	// 弹出队头元素，只需修改指针即可，数据无需修改
	void pop() {
		((std::atomic<uint32_t>*)&read_idx)
		    ->store(read_idx + 1, std::memory_order_release);
	}

	// 尝试弹出
	// 使用时尽量使用 front + pop
	template <typename Reader>
	bool tryPop(Reader reader) {
		T* v = front();
		if (!v)
			return false;
		reader(v);
		pop();
		return true;
	}

private:
	// 这里的内存对其和 cache line 的大小相关
	// 保证变量独占一个 cache line

	// 若多个变量在同一个 cache line 中
	// 若某线程访问其中一个变量使得 cache line 加锁
	// 会导致其他线程访问后续变量时因无法访问 cache line 而造成性能损失
	alignas(128) T data[CNT] = {};

	//  write 时对 read_idx 采取缓存策略
	// 减少访问 read_idx 带来的对于读操作的影响
	alignas(128) uint32_t write_idx = 0;
	uint32_t read_idx_cach = 0;

	// 读指针
	alignas(128) uint32_t read_idx = 0;
};
