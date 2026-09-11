#pragma once

namespace strata
{
	namespace json
	{
		template <typename T> struct span
		{
			T* begin_;
			T* end_;

			span(T* begin, size_t size)
			{
				begin_ = begin;
				end_ = begin_ + size;
			}

			span(T* begin, T* end)
			{
				begin_ = begin;
				end_ = end;
			}

			const T* begin() const noexcept
			{
				return begin_;
			}

			const T* end() const noexcept
			{
				return end_;
			}

			size_t size() const
			{
				return end_ - begin_;
			}

			T& operator[](size_t size)
			{
				return begin_[size];
			}

			const T& operator[](size_t size) const
			{
				return begin_[size];
			}
		};
	} // namespace json
} // namespace strata
