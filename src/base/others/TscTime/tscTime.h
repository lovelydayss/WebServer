#pragma once
#include <atomic>
#include <ios>
#include <limits>
#include <mutex>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>

#ifndef TSCTIME_H
#define TSCTIME_H

namespace tscns {

// 高性能时间类定义
class TSCNS {
public:
	static const int64_t NsPerSec = 1000000000; // 默认纳秒到秒的转换进率

	// 初始化
	void init(int64_t init_calibrate_ns = 20000000,
	          int64_t calibrate_interval_ns = 3 * NsPerSec) {
		calibate_interval_ns_ = calibrate_interval_ns;
		int64_t base_tsc, base_ns;
		syncTime(base_tsc, base_ns);				 // 执行起始 CPU 时间戳和系统 ns 时间同步
		int64_t expire_ns = base_ns + init_calibrate_ns;			// 执行首次校准
		while (rdsysns() < expire_ns)
			std::this_thread::yield();								// 释放资源等待首次校准
		int64_t delayed_tsc, delayed_ns;
		syncTime(delayed_tsc, delayed_ns);
		double init_ns_per_tsc =
		    (double)(delayed_ns - base_ns) / (delayed_tsc - base_tsc);		// 利用差值计算每 CPU 时间戳 包含纳秒数
		saveParam(base_tsc, base_ns, base_ns, init_ns_per_tsc);	// 参数设置
	}

	// 校准时间
	void calibrate() {
		if (rdtsc() < next_calibrate_tsc_)
			return;
		int64_t tsc, ns;
		syncTime(tsc, ns); // 执行同步
		
		int64_t calulated_ns = tsc2ns(tsc);
		int64_t ns_err = calulated_ns - ns;
		int64_t expected_err_at_next_calibration =
		    ns_err + (ns_err - base_ns_err_) * calibate_interval_ns_ /
		                 (ns - base_ns_ + base_ns_err_);

		// 误差修正
		double new_ns_per_tsc =
		    ns_per_tsc_ * (1.0 - (double)expected_err_at_next_calibration /
		                             calibate_interval_ns_);
		saveParam(tsc, calulated_ns, ns, new_ns_per_tsc);
	}

	// 读取系统时间戳
	static inline int64_t rdtsc() { return __builtin_ia32_rdtsc(); }

	// 将 CPU 时间戳转换为 ns 时间
	inline int64_t tsc2ns(int64_t tsc) const {
		while (true) {
			uint32_t before_seq =
			    param_seq_.load(std::memory_order_acquire) & ~1;
			std::atomic_signal_fence(std::memory_order_acq_rel);					// 内存屏障，保证内存操作连续进行
			int64_t ns = base_ns_ + (int64_t)((tsc - base_tsc_) * ns_per_tsc_);
			std::atomic_signal_fence(std::memory_order_acq_rel);
			uint32_t after_seq = param_seq_.load(std::memory_order_acquire);
			if (before_seq == after_seq)											// 使用序列号判断状态是否有更新
				return ns;
		}
	}

	// 使用 CPU 时间戳计算系统纳秒时间
	inline int64_t rdns() const { return tsc2ns(rdtsc()); }

	// 使用 std::chrono 读取系统纳秒时间
	static inline int64_t rdsysns() {
		using namespace std::chrono;
		return duration_cast<nanoseconds>(
		           system_clock::now().time_since_epoch())
		    .count();
	}

	// 获取 CPU 时钟频率
	double getTscGhz() const { return 1.0 / ns_per_tsc_; }

	// 同步系统时钟和 CPU 时间戳
	static void syncTime(int64_t& tsc_out, int64_t& ns_out) {
		const int N = 3;

		int64_t tsc[N + 1];
		int64_t ns[N + 1];

		tsc[0] = rdtsc();

		// 获取 N 个时间戳及系统对应 ns 时间
		for (int i = 1; i <= N; i++) {
			ns[i] = rdsysns();
			tsc[i] = rdtsc();
		}

		int best = 1;
		for (int i = 2; i < N + 1; i++) {
			if (tsc[i] - tsc[i - 1] < tsc[best] - tsc[best - 1])
				best = i; // 保留差距最小的时间戳
		}
		tsc_out = (tsc[best] + tsc[best - 1]) >> 1; // 平均降低随机误差
		ns_out = ns[best];
	}

	// 保存参数
	void saveParam(int64_t base_tsc,
	               int64_t base_ns,
	               int64_t sys_ns,
	               double new_ns_per_tsc) {
		base_ns_err_ = base_ns - sys_ns;
		next_calibrate_tsc_ =
		    base_tsc +
		    (int64_t)((calibate_interval_ns_ - 1000) / new_ns_per_tsc);

		// 修改操作序列号
		uint32_t seq = param_seq_.load(std::memory_order_relaxed);
		param_seq_.store(++seq, std::memory_order_release);
		
		std::atomic_signal_fence(std::memory_order_acq_rel);			// 内存屏障
		base_tsc_ = base_tsc;
		base_ns_ = base_ns;
		ns_per_tsc_ = new_ns_per_tsc;
		std::atomic_signal_fence(std::memory_order_acq_rel);

		// 修改操作序列号
		param_seq_.store(++seq, std::memory_order_release);
	}

	alignas(64) std::atomic<uint32_t> param_seq_ =
	    0; // 系统时间戳及参数序列号，保证访问有效
	double ns_per_tsc_;            // 单个 CPU 时间戳对应 ns 数目
	int64_t base_tsc_;             // 起始时钟戳计数
	int64_t base_ns_;              // 起始纳秒计数
	int64_t calibate_interval_ns_; // 校准间隔（纳秒计数）
	int64_t base_ns_err_;          // 起始纳秒误差
	int64_t next_calibrate_tsc_;   // 下一次校准所在的时间戳
};
}; // namespace tscns

#endif