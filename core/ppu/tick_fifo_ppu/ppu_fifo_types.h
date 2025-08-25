#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>


namespace ppu_fifo_types {

	struct fifo_element {
		uint8_t color;
		bool palette;
		uint8_t priority;
		bool bg_priority;
	};

	enum fifo_state {
		GET_TILE,
		GET_TILE_DATA_LOW,
		GET_TILE_DATA_HIGH,
		SLEEP,
		PUSH
	};


	struct OAM_priority_queue_element {
		ppu_types::OAM_Sprite sprite;
		uint16_t oam_index;
	};


	template <size_t ARRAY_SIZE = 10>
	class oam_ring_buffer
	{
	public:
		oam_ring_buffer() :base(0), tail(0), total_elements(0) {};

		class iterator {
		public:
			using difference_type = std::ptrdiff_t;
			using value_type = OAM_priority_queue_element;
			using pointer = value_type*;
			using reference = value_type&;
			using iterator_category = std::random_access_iterator_tag;

			iterator(oam_ring_buffer* buf, size_t idx)
				: buf(buf), idx(idx) {
			}

			reference operator*() { return (*buf)[idx]; }
			pointer   operator->() { return &(*buf)[idx]; }

			// increment / decrement
			iterator& operator++() { ++idx; return *this; }
			iterator& operator--() { --idx; return *this; }

			iterator operator+(difference_type n) const { return iterator(buf, idx + n); }
			iterator operator-(difference_type n) const { return iterator(buf, idx - n); }
			difference_type operator-(const iterator& other) const { return idx - other.idx; }

			iterator& operator+=(difference_type n) { idx += n; return *this; }
			iterator& operator-=(difference_type n) { idx -= n; return *this; }

			reference operator[](difference_type n) { return (*buf)[idx + n]; }

			reference operator*() const { return (*buf)[idx]; }
			pointer   operator->() const { return &(*buf)[idx]; }
			reference operator[](difference_type n) const { return (*buf)[idx + n]; }

			bool operator==(const iterator& other) const { return idx == other.idx; }
			bool operator!=(const iterator& other) const { return idx != other.idx; }
			bool operator<(const iterator& other)  const { return idx < other.idx; }
			bool operator>(const iterator& other)  const { return idx > other.idx; }
			bool operator<=(const iterator& other) const { return idx <= other.idx; }
			bool operator>=(const iterator& other) const { return idx >= other.idx; }

		private:
			oam_ring_buffer* buf;
			size_t idx; // logical index (0..size()-1), NOT the raw array index
		};

		[[nodiscard]] bool empty() const
		{
			return total_elements == 0;
		}

		[[nodiscard]] bool full() const
		{
			return size() == ARRAY_SIZE;
		}

		[[nodiscard]] uint64_t size() const
		{
			return total_elements;
		};

		bool push(const OAM_priority_queue_element element)
		{
			if (full())
			{
				return false;
			}
			elements[tail] = element;
			tail = (tail + 1) % ARRAY_SIZE;
			total_elements++;
			return true;
		}

		const OAM_priority_queue_element& front() const
		{
			return elements[base];
		}

		bool pop() noexcept {
			if (empty()) {
				return false;
			}
			base = (base + 1) % ARRAY_SIZE;
			total_elements--;
			return true;
		}
		void clear() {
			base = tail = total_elements = 0;
		}

		OAM_priority_queue_element& operator[](size_t index) {
			return elements[(base + index) % ARRAY_SIZE];
		}

		const OAM_priority_queue_element& operator[](size_t index) const {
			return elements[(base + index) % ARRAY_SIZE];
		}

		iterator begin() { return iterator(this, 0); }
		iterator end() { return iterator(this, size()); }

		template <typename Compare>
		void sort(Compare comp) {
			std::sort(begin(), end(), comp);
		}

	private:
		std::array<OAM_priority_queue_element, ARRAY_SIZE>  elements{};
		uint64_t base; //Points to the start of the array
		uint64_t tail; // Points to the last element
		uint64_t total_elements;



	};



};