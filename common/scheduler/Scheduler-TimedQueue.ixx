export module Scheduler:TimedQueue;

import std;

namespace sched
{
	using Task = std::move_only_function<void()>;
	using Slot = std::list<Task>;

	export struct NextTaskTimePoint
	{
		bool bHasTask = false;
		std::chrono::steady_clock::time_point tp;
	};

	export
	class MultiTimingWheel;

	template <typename T, std::uint8_t Level>
	concept timing_wheel_concept = requires
	{
		T::levelCount;
		{ T::template getSlotBits<Level>() } -> std::same_as<std::uint8_t>;
		{ T::template getBitOffset<Level>() } -> std::same_as<std::uint8_t>;
		{ T::template getSlotMask<Level>() } -> std::same_as<size_t>;
		{ T::template getSlotCount<Level>() } -> std::same_as<size_t>;
		{ T::template getBoundary<Level>() } -> std::same_as<std::uint64_t>;
		{ T::template hasPrevWheel<Level>() } -> std::same_as<bool>;
		{ T::template hasNextWheel<Level>() } -> std::same_as<bool>;
	};

	export
	template <typename T, std::uint8_t Level>
		requires timing_wheel_concept<T, Level>
	class TimingWheel;

	template <typename T, std::uint8_t Level, bool HasPrev, bool HasNext>
	struct CascadeInfo
	{
		static constexpr bool bHasPrev = HasPrev;
		static constexpr bool bHasNext = HasNext;
		TimingWheel<T, Level - 1>* prevWheel;
		TimingWheel<T, Level + 1>* nextWheel;
	};

	template <typename T, std::uint8_t Level>
	struct CascadeInfo<T, Level, false, true>
	{
		static constexpr bool bHasPrev = false;
		static constexpr bool bHasNext = true;
		TimingWheel<T, Level + 1>* nextWheel;

		std::chrono::steady_clock::time_point currentTime{};
		Slot immediateTaskSlot{};
	};

	template <typename T, std::uint8_t Level>
	struct CascadeInfo<T, Level, true, false>
	{
		static constexpr bool bHasPrev = true;
		static constexpr bool bHasNext = false;
		TimingWheel<T, Level - 1>* prevWheel;
	};

	template <typename T, std::uint8_t Level>
	struct CascadeInfo<T, Level, false, false>
	{
		static constexpr bool bHasPrev = false;
		static constexpr bool bHasNext = false;

		std::chrono::steady_clock::time_point currentTime{};
		Slot immediateTaskSlot{};
	};

	template <typename T, std::uint8_t Level = 0>
		requires timing_wheel_concept<T, Level>
	class TimingWheel
	{
	public:
		static constexpr std::uint8_t slotBits = T::template getSlotBits<Level>();
		static constexpr std::uint8_t bitOffset = T::template getBitOffset<Level>();
		static constexpr size_t levelCount = T::levelCount;
		static constexpr size_t slotMask = T::template getSlotMask<Level>();
		static constexpr size_t slotCount = T::template getSlotCount<Level>();
		static constexpr std::uint64_t boundary = T::template getBoundary<Level>();
		static constexpr bool bHasPrevWheel = T::template hasPrevWheel<Level>();
		static constexpr bool bHasNextWheel = T::template hasNextWheel<Level>();
		static constexpr size_t flagBits = 6;
		static constexpr size_t flagMask = (1 << flagBits) - 1;
		static constexpr size_t flagCount = std::max(slotCount >> flagBits, static_cast<size_t>(1));
		static constexpr size_t flagArrayIndexMask = flagCount - 1;

		static constexpr size_t timePoint2Index(std::chrono::steady_clock::time_point tp)
		{
			const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
			return (ms >> bitOffset) & slotMask;
		}

		explicit TimingWheel(std::chrono::steady_clock::time_point initialTime)
			requires (!bHasPrevWheel && !bHasNextWheel)
			: m_currentSlotIndex(timePoint2Index(initialTime))
			  , m_cascadeInfo{.currentTime = initialTime}
		{
		}

		TimingWheel(auto& nextWheel, std::chrono::steady_clock::time_point initialTime)
			requires (!bHasPrevWheel && bHasNextWheel)
			: m_currentSlotIndex(timePoint2Index(initialTime))
			  , m_cascadeInfo{.nextWheel = &nextWheel, .currentTime = initialTime}
		{
		}

		TimingWheel(auto& prevWheel, auto& nextWheel, std::chrono::steady_clock::time_point initialTime)
			requires (bHasPrevWheel && bHasNextWheel)
			: m_currentSlotIndex(timePoint2Index(initialTime))
			  , m_cascadeInfo{.prevWheel = &prevWheel, .nextWheel = &nextWheel}
		{
		}

		TimingWheel(auto& prevWheel, std::chrono::steady_clock::time_point initialTime)
			requires (bHasPrevWheel && !bHasNextWheel)
			: m_currentSlotIndex(timePoint2Index(initialTime))
			  , m_cascadeInfo{.prevWheel = &prevWheel}
		{
		}

		TimingWheel(const TimingWheel&) = delete;
		TimingWheel(TimingWheel&&) = delete;
		TimingWheel& operator=(const TimingWheel&) = delete;
		TimingWheel& operator=(TimingWheel&&) = delete;

	public:
		void addTimer(std::chrono::steady_clock::time_point expireTime, Task task)
		{
			const size_t slotIndex = timePoint2Index(expireTime);
			Slot& slot = m_slots[slotIndex];
			if constexpr (bHasPrevWheel)
			{
				auto addTimerToPrevWheel = [prevWheel = m_cascadeInfo.prevWheel, expireTime, task = std::move(task)]() mutable
				{
					prevWheel->addTimer(expireTime, std::move(task));
				};
				slot.push_back(std::move(addTimerToPrevWheel));
			}
			else
			{
				slot.push_back(std::move(task));
			}

			if (slot.size() == 1)
			{
				setFlag(slotIndex);
			}
		}

		void addTask(Task task)
			requires (!bHasPrevWheel)
		{
			Slot& slot = m_cascadeInfo.immediateTaskSlot;
			slot.push_back(std::move(task));
		}

		NextTaskTimePoint nextTaskTimePoint()
			requires (!bHasPrevWheel)
		{
			if (m_cascadeInfo.immediateTaskSlot.size() > 0)
			{
				return {true, m_cascadeInfo.currentTime};
			}
			std::uint64_t deltaIndex = deltaToNextTimer();
			if (deltaIndex != 0)
			{
				return {true, m_cascadeInfo.currentTime + std::chrono::milliseconds{deltaIndex}};
			}
			return {};
		}

		void advanceUntil(std::chrono::steady_clock::time_point tp)
			requires (!bHasPrevWheel)
		{
			const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp - m_cascadeInfo.currentTime).count();
			if (ms > 0)
			{
				advance(ms);
				m_cascadeInfo.currentTime = tp;
			}
		}

		void advanceFor(std::chrono::steady_clock::duration duration)
			requires (!bHasPrevWheel)
		{
			const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
			if (ms > 0)
			{
				advance(ms);
				m_cascadeInfo.currentTime += duration;
			}
		}

		Slot pull()
			requires (bHasPrevWheel)
		{
			Slot temp;
			m_slots[m_currentSlotIndex].swap(temp);

			if (temp.size() > 0)
			{
				clearFlag(m_currentSlotIndex);
			}
			return temp;
		}

		Slot pull()
			requires (!bHasPrevWheel)
		{
			Slot temp;
			m_slots[m_currentSlotIndex].swap(temp);

			Slot ret;
			m_cascadeInfo.immediateTaskSlot.swap(ret);

			if (temp.size() > 0)
			{
				clearFlag(m_currentSlotIndex);
				ret.splice(ret.end(), temp);
			}
			return ret;
		}

		std::chrono::steady_clock::time_point getCurrentTime()
			requires (!bHasPrevWheel)
		{
			return m_cascadeInfo.currentTime;
		}

	protected:
		template <typename U, std::uint8_t Ulv>
			requires timing_wheel_concept<U, Ulv>
		friend class TimingWheel;
		friend class MultiTimingWheel;

		void setFlag(size_t slotIndex)
		{
			const size_t bitArrayIndex = slotIndex >> flagBits;
			const size_t bitOffset = slotIndex & flagMask;
			m_flags[bitArrayIndex] |= (static_cast<std::uint64_t>(1) << bitOffset);
		}

		void clearFlag(size_t slotIndex)
		{
			const size_t bitArrayIndex = slotIndex >> flagBits;
			const size_t bitOffset = slotIndex & flagMask;
			m_flags[bitArrayIndex] &= ~(static_cast<std::uint64_t>(1) << bitOffset);
		}

		void advance(std::uint64_t deltaIndex)
		{
			const std::uint64_t currentIndex = m_currentSlotIndex + deltaIndex;
			const std::uint64_t cycles = currentIndex >> slotBits;

			m_currentSlotIndex = currentIndex & slotMask;

			if constexpr (bHasNextWheel)
			{
				if (cycles > 0)
				{
					m_cascadeInfo.nextWheel->advance(cycles);
					Slot tasks = m_cascadeInfo.nextWheel->pull();
					for (Task& task : tasks)
					{
						if (task)
						{
							task();
						}
					}
				}
			}
		}

		// 获取到下一个定时任务的slot间隔，因此精度为该任务所在层的槽的精度
		// 层级越大精度越低
		// 返回0表示当前没有定时任务
		std::uint64_t deltaToNextTimer() const
		{
			std::uint64_t deltaIndex = 0;
			// 256槽的时间轮由4个 std::uint64_t 合并作为 flag, flagCount 是4
			// 64槽的时间轮只有1个 std::uint64_t 作为 flag, flagCount 是1
			// 但处理逻辑可以相同，以下面的循环进行。
			// startIndex 是当前槽位所在的flag数组的index, 比如对于64槽来说就是0，因为其只有一个flag，
			// indexOffset 则是当前槽位所在的flag中的表示当前槽的bit位
			const size_t startIndex = m_currentSlotIndex >> flagBits;
			const size_t indexOffset = m_currentSlotIndex & flagMask;
			// 循环要做的事情是：
			// 把表示当前槽的bit位的 后一位 作为最低位，一直转到最后以当前槽的bit位作为最高位结束组成一个flag
			// 这样一来只要判断这个flag的右边(低位)的0的个数,就能知道离当前槽最近的有任务的槽了
			// 标准库有std::countr_zero可以直接做该判断，一般平台实现都是内部函数，可能只需一条指令，效率很高。
			// 但问题是，最大一次只能判断64位。
			// 对于64位槽来说很好，这其实不是个循环，执行一次就结束了
			// 对于256位槽来说，可以由低到高最多判断4次即可，只要找到了有数据的那64位即可结束，注意最终结果要加上前面已经判断过的位数
			for (size_t i = startIndex; i < flagCount + startIndex; ++i)
			{
				const size_t index = i & flagArrayIndexMask;
				const size_t nextIndex = (index + 1) & flagArrayIndexMask;
				const std::uint64_t currentFlag = m_flags[index];
				const std::uint64_t nextFlag = m_flags[nextIndex];
				const std::uint64_t combined = combineFlag(indexOffset, currentFlag, nextFlag);
				// 组合出的combined,即是从indexOffset+1作为最低位开始到下一个flag的indexOffset位，以它作为最高位结束
				// 不为0 就说明有数据
				if (combined != 0)
				{
					// 最近的数据即是 右边第一个1的右边的0的个数 + 1 
					const std::uint64_t rightZeroCount = std::countr_zero(combined);
					deltaIndex = rightZeroCount + 1 + 64 * (i - startIndex);
					break;
				}
			}

			if (deltaIndex == 0)
			{
				// 如果当前层没有任务，继续从后一层获取
				if constexpr (bHasNextWheel)
				{
					deltaIndex = m_cascadeInfo.nextWheel->deltaToNextTimer();
					// 从后一层得到的deltaIndex由于精度不一样,需要换算
					if (deltaIndex != 0)
					{
						// 后一层的一个index,代表本层的一圈，即一个 slotCount
						// deltaIndex再乘以slotCount之前,要先减去 1
						// 因为本层可能已经转了一半了，不足一圈，所以先减1，再把本层剩余的加上
						const std::uint64_t remainingIndex = slotCount - m_currentSlotIndex;
						deltaIndex = (deltaIndex - 1) * slotCount + remainingIndex;
					}
				}
			}
			return deltaIndex;
		}

		// 以currentFlag的第 indexOffset+1 位到63位作为低位，以nextFlag的第0位到第 indexOffset 位作为高位组成最终的flag
		static constexpr std::uint64_t combineFlag(size_t indexOffset, std::uint64_t currentFlag, std::uint64_t nextFlag)
		{
			if (indexOffset >= 63)
			{
				return nextFlag;
			}
			//////////////////
			// 提取 currentFlag 中从 indexOffset+1 位开始到结束的部分（作为低位）
			const size_t highBitStart = indexOffset + 1;
			// 高位全1，低位全0
			const std::uint64_t lowBitsMask = ~((static_cast<std::uint64_t>(1) << highBitStart) - 1);
			const std::uint64_t extractedHighBits = currentFlag & lowBitsMask;
			// 右移到低位，高位补0
			const std::uint64_t lowBits = extractedHighBits >> highBitStart;

			///////////////////
			// 提取 nextFlag 中从第0位到 indexOffset 位的部分（作为高位）

			// 低位全1，高位全0
			const std::uint64_t highBitsMask = (static_cast<std::uint64_t>(1) << highBitStart) - 1;
			const std::uint64_t highBits = nextFlag & highBitsMask;
			// 组合两部分
			const std::uint64_t combined = (highBits << (64 - highBitStart)) | lowBits;
			return combined;
		}

	private:
		std::array<Slot, slotCount> m_slots;
		size_t m_currentSlotIndex;
		std::array<std::uint64_t, flagCount> m_flags{};
		CascadeInfo<T, Level, bHasPrevWheel, bHasNextWheel> m_cascadeInfo;
	};

	template <size_t LevelCount>
	struct MultiTimingWheelInfoBuilder
	{
		struct WheelInfo
		{
			std::uint8_t slotBits;
			std::uint8_t bitOffset;
			size_t slotCount;
			size_t slotMask;
			std::uint64_t boundary;
		};

		std::array<WheelInfo, LevelCount> info;

		consteval MultiTimingWheelInfoBuilder(const std::array<std::uint8_t, LevelCount>& slotBitsInfos)
		{
			std::uint8_t currentBitOffset = 0;
			for (size_t i = 0; i < slotBitsInfos.size(); ++i)
			{
				std::uint8_t slotBits = slotBitsInfos[i];
				size_t slotCount = static_cast<size_t>(1) << slotBits;
				size_t slotMask = slotCount - 1;
				std::uint8_t bitOffset = currentBitOffset;
				WheelInfo& wi = info[i];
				wi.slotBits = slotBits;
				wi.bitOffset = bitOffset;
				wi.slotCount = slotCount;
				wi.slotMask = slotMask;

				currentBitOffset += slotBits;
				wi.boundary = static_cast<std::uint64_t>(1) << currentBitOffset;
			}
		}
	};

	struct MultiTimingWheelInfo
	{
		static constexpr std::uint8_t rootWheelBits = 8;
		static constexpr std::uint8_t subWheelBits = 6;

		static constexpr MultiTimingWheelInfoBuilder wheelInfo
		{
			std::array{rootWheelBits, subWheelBits, subWheelBits, subWheelBits, subWheelBits}
		};
		static constexpr size_t levelCount = wheelInfo.info.size();

		template <std::uint8_t Level>
		static constexpr std::uint8_t getSlotBits()
		{
			return wheelInfo.info[Level].slotBits;
		}

		template <std::uint8_t Level>
		static constexpr std::uint8_t getBitOffset()
		{
			return wheelInfo.info[Level].bitOffset;
		}

		template <std::uint8_t Level>
		static constexpr size_t getSlotMask()
		{
			return wheelInfo.info[Level].slotMask;
		}

		template <std::uint8_t Level>
		static constexpr size_t getSlotCount()
		{
			return wheelInfo.info[Level].slotCount;
		}

		template <std::uint8_t Level>
		static constexpr std::uint64_t getBoundary()
		{
			return wheelInfo.info[Level].boundary;
		}

		template <std::uint8_t Level>
		static constexpr bool hasPrevWheel()
		{
			return Level - 1 >= 0;
		}

		template <std::uint8_t Level>
		static constexpr bool hasNextWheel()
		{
			return Level + 1 < levelCount;
		}
	};

	class MultiTimingWheel
	{
	public:
		explicit MultiTimingWheel(std::chrono::steady_clock::time_point initialTime = std::chrono::steady_clock::now())
			: m_rootWheel(m_wheelLevel1, initialTime)
			  , m_wheelLevel1(m_rootWheel, m_wheelLevel2, initialTime)
			  , m_wheelLevel2(m_wheelLevel1, m_wheelLevel3, initialTime)
			  , m_wheelLevel3(m_wheelLevel2, m_wheelLevel4, initialTime)
			  , m_wheelLevel4(m_wheelLevel3, initialTime)
		{
		}

	public:
		void addTimer(std::chrono::steady_clock::time_point expireTime, Task task)
		{
			std::chrono::steady_clock::time_point now = m_rootWheel.getCurrentTime();
			if (expireTime <= now)
			{
				addTask(std::move(task));
				return;
			}
			const std::uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(expireTime - now).count();
			if (ms < m_rootWheel.boundary)
			{
				m_rootWheel.addTimer(expireTime, std::move(task));
			}
			else if (ms < m_wheelLevel1.boundary)
			{
				m_wheelLevel1.addTimer(expireTime, std::move(task));
			}
			else if (ms < m_wheelLevel2.boundary)
			{
				m_wheelLevel2.addTimer(expireTime, std::move(task));
			}
			else if (ms < m_wheelLevel3.boundary)
			{
				m_wheelLevel3.addTimer(expireTime, std::move(task));
			}
			else if (ms < m_wheelLevel4.boundary)
			{
				m_wheelLevel4.addTimer(expireTime, std::move(task));
			}
			else
			{
				throw std::runtime_error("expireTime is out of supported range");
			}
		}

		void addTask(Task task)
		{
			m_rootWheel.addTask(std::move(task));
		}

		NextTaskTimePoint nextTaskTimePoint()
		{
			return m_rootWheel.nextTaskTimePoint();
		}

		void advanceUntil(std::chrono::steady_clock::time_point tp)
		{
			m_rootWheel.advanceUntil(tp);
		}

		void advanceFor(std::chrono::steady_clock::duration duration)
		{
			m_rootWheel.advanceFor(duration);
		}

		Slot pull()
		{
			return m_rootWheel.pull();
		}

		std::chrono::steady_clock::time_point getCurrentTime()
		{
			return m_rootWheel.getCurrentTime();
		}

	private:
		TimingWheel<MultiTimingWheelInfo> m_rootWheel;
		TimingWheel<MultiTimingWheelInfo, 1> m_wheelLevel1;
		TimingWheel<MultiTimingWheelInfo, 2> m_wheelLevel2;
		TimingWheel<MultiTimingWheelInfo, 3> m_wheelLevel3;
		TimingWheel<MultiTimingWheelInfo, 4> m_wheelLevel4;
	};
}
