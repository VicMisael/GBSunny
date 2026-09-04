#pragma once
namespace cpu {
	class ICPU {
	public:
		virtual ~ICPU() = default;
		virtual void reset() = 0;
		virtual uint32_t step() = 0;
	};
}
