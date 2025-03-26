#pragma once

#include <cstddef>
#include <new>
#include <type_traits>

namespace ovdl::detail {
	class DefaultMemoryResource {
	public:
		static void* allocate(std::size_t bytes, std::size_t alignment) {
			if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
				return ::operator new(bytes, std::align_val_t { alignment });
			} else {
				return ::operator new(bytes);
			}
		}

		static void deallocate(void* ptr, std::size_t bytes, std::size_t alignment) noexcept {
#ifdef __cpp_sized_deallocation
			if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
				::operator delete(ptr, bytes, std::align_val_t { alignment });
			} else {
				::operator delete(ptr, bytes);
			}
#else
			(void)bytes;

			if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
				::operator delete(ptr, std::align_val_t { alignment });
			} else {
				::operator delete(ptr);
			}
#endif
		}

		friend constexpr bool operator==(DefaultMemoryResource, DefaultMemoryResource) noexcept {
			return true;
		}
	};

	template<typename MemoryResource>
	class _MemoryResourcePtrEmpty {
	public:
		constexpr explicit _MemoryResourcePtrEmpty(MemoryResource*) noexcept {}
		constexpr explicit _MemoryResourcePtrEmpty(void*) noexcept {}

		constexpr auto operator*() const noexcept {
			return MemoryResource {};
		}

		constexpr auto operator->() const noexcept {
			struct proxy {
				MemoryResource _resource;

				constexpr MemoryResource* operator->() noexcept {
					return &_resource;
				}
			};

			return proxy {};
		}

		constexpr MemoryResource* get() const noexcept {
			return nullptr;
		}
	};

	template<typename MemoryResource>
	class _MemoryResourcePtr {
	public:
		constexpr explicit _MemoryResourcePtr(MemoryResource* resource) noexcept : _resource(resource) {
			LEXY_PRECONDITION(resource);
		}

		constexpr MemoryResource& operator*() const noexcept {
			return *_resource;
		}

		constexpr MemoryResource* operator->() const noexcept {
			return _resource;
		}

		constexpr MemoryResource* get() const noexcept {
			return _resource;
		}

	private:
		MemoryResource* _resource;
	};

	template<typename MemoryResource>
	struct _MemoryResourcePtrSelector {
		using type = _MemoryResourcePtr<MemoryResource>;
	};

	template<typename MemoryResource>
		requires std::is_empty_v<MemoryResource>
	struct _MemoryResourcePtrSelector<MemoryResource> {
		using type = _MemoryResourcePtrEmpty<MemoryResource>;
	};

	template<>
	struct _MemoryResourcePtrSelector<void> {
		using type = _MemoryResourcePtrEmpty<DefaultMemoryResource>;
	};

	template<typename MemoryResource>
	using MemoryResourcePtr = _MemoryResourcePtrSelector<MemoryResource>::type;

	template<typename MemoryResource>
		requires std::is_void_v<MemoryResource> || std::is_empty_v<MemoryResource>
	constexpr MemoryResource* get_memory_resource() {
		return nullptr;
	}
}