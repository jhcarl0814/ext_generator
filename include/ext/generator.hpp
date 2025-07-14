#include <cassert>
#include <coroutine>
#include <exception>
#include <ranges>
#include <type_traits>

#if defined(ext_generator_debug)
    #include <iostream>
    #include <string_view>
    #define if_ext_generator_debug(...) __VA_ARGS__
namespace ext
{
    template<typename T>
    constexpr auto type_name() // https://stackoverflow.com/questions/81870/is-it-possible-to-print-a-variables-type-in-standard-c/56766138#56766138
    {
        std::string_view name, prefix, suffix;
    #ifdef __clang__
        name = __PRETTY_FUNCTION__;
        prefix = "auto type_name() [T = ";
        suffix = "]";
    #elif defined(__GNUC__)
        name = __PRETTY_FUNCTION__;
        prefix = "constexpr auto type_name() [with T = ";
        suffix = "]";
    #elif defined(_MSC_VER)
        name = __FUNCSIG__;
        prefix = "auto __cdecl type_name<";
        suffix = ">(void)";
    #endif
        name.remove_prefix(prefix.size());
        name.remove_suffix(suffix.size());
        return name;
    }
} // namespace ext
#else
    #define if_ext_generator_debug(...)
#endif

namespace ext
{
    template<typename allocator_t>
    concept type_agnostic_allocator_c = std::is_same_v<typename std::allocator_traits<allocator_t>::value_type, void>;

    template<typename reference_t_, typename value_t_ = void, typename yielded_t_ = void> requires (std::is_void_v<yielded_t_> || std::is_reference_v<yielded_t_>)
    struct generator_t;

    template<typename yielded_t>
    struct generator_promise_base_t
    {
        std::add_pointer_t<yielded_t> p_yielded;
        generator_promise_base_t *p_promise_continuation, *p_promise_root_or_current; // Link promises into call tree to implement symmetric transfer. For root, p_promise_continuation = nullptr, p_promise_root_or_current = std::addressof(active_frame's promise). For non-root, p_promise_continuation = std::addressof(caller(parent)'s promise), p_promise_root_or_current = std::addressof(root's promise).
        std::exception_ptr *p_p_exception; // For root, nullptr. For non-root, std::addressof(caller(parent)'s yield_awaitable_t's p_exception).

        generator_promise_base_t() noexcept : p_promise_continuation(nullptr), p_promise_root_or_current(this), p_p_exception(nullptr) {}
        std::suspend_never initial_suspend() const noexcept { return {}; }
        struct final_awaitable_t
        {
            bool await_ready() const noexcept { return false; }
            template<std::derived_from<generator_promise_base_t> promise_type>
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> continuation) const noexcept
            {
                promise_type &promise_continuation = continuation.promise();
                if (generator_promise_base_t *p_promise_root = promise_continuation.p_promise_root_or_current, *p_promise_current = promise_continuation.p_promise_continuation; p_promise_current != nullptr) // non-root
                {
                    p_promise_root->p_promise_root_or_current = p_promise_current; // adjust root's std::addressof(active_frame's promise)
                    return std::coroutine_handle<generator_promise_base_t>::from_promise(*p_promise_current); // !!! not using the original promise but its base subobject to create std::coroutine_handle
                }
                else // root
                    return std::noop_coroutine();
            }
            void await_resume() const noexcept {}
        };
        final_awaitable_t final_suspend() const noexcept { return {}; }
        void await_transform() const noexcept = delete;
        void return_void() const noexcept {}
        void unhandled_exception() const
        {
            if (p_p_exception != nullptr) // non-root
                *p_p_exception = std::current_exception();
            else // root
                throw;
        }

        std::suspend_always yield_value(yielded_t yielded) noexcept
        {
            this->p_yielded = std::addressof(yielded);
            return {};
        }
        template<typename reference_other_t, typename value_other_t, typename yielded_other_t>
        struct yield_awaitable_t
        {
            generator_t<reference_other_t, value_other_t, yielded_other_t> generator;
            std::exception_ptr p_exception;
            bool await_ready() const noexcept { return generator.handle.done(); }
            template<std::derived_from<generator_promise_base_t> promise_type>
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> continuation) noexcept
            {
                auto &promise_generator = generator.handle.promise();
                assert(promise_generator.p_promise_continuation == nullptr); // assert(subtree.p_root->p_parent == nullptr)
                promise_type *p_promise_continuation = std::addressof(continuation.promise());
                promise_generator.p_promise_continuation = p_promise_continuation; // subtree.p_root->p_parent = tree.p_leaf
                generator_promise_base_t *p_promise_continuation_root = p_promise_continuation->p_promise_continuation == nullptr ? p_promise_continuation : p_promise_continuation->p_promise_root_or_current;
                p_promise_continuation_root->p_promise_root_or_current = promise_generator.p_promise_root_or_current; // tree.p_leaf = subtree.p_leaf
                for (generator_promise_base_t *p_promise = promise_generator.p_promise_root_or_current; p_promise != p_promise_continuation; p_promise = p_promise->p_promise_continuation) // for (p = subtree.p_leaf; p != old tree.p_leaf; p = p->p_parent)
                    p_promise->p_promise_root_or_current = p_promise_continuation_root;
                promise_generator.p_p_exception = &p_exception;
                return std::noop_coroutine();
            }
            void await_resume() noexcept
            {
                if (p_exception)
                    std::rethrow_exception(std::move(p_exception));
            }
        };
        template<typename reference_other_t, typename value_other_t, typename yielded_other_t, typename allocator_t> requires std::same_as<typename generator_t<reference_other_t, value_other_t, yielded_other_t>::yielded_t, yielded_t>
        auto yield_value(std::ranges::elements_of<generator_t<reference_other_t, value_other_t, yielded_other_t> &&, allocator_t> generator_and_allocator) const noexcept
        {
            assert(generator_and_allocator.range.joinable());
            return yield_awaitable_t<reference_other_t, value_other_t, yielded_other_t>{.generator = std::move(generator_and_allocator.range)};
        }
        template<std::ranges::input_range range_t, typename allocator_t> requires std::convertible_to<std::ranges::range_reference_t<range_t>, yielded_t>
        auto yield_value(std::ranges::elements_of<range_t, allocator_t> range_and_allocator) const
        {
            return generator_promise_base_t::yield_value(std::ranges::elements_of([](std::allocator_arg_t, decltype(range_and_allocator.allocator), std::ranges::iterator_t<range_t> i, std::ranges::sentinel_t<range_t> s) -> generator_t<yielded_t> {
                for (; i != s; ++i)
                    co_yield static_cast<yielded_t>(*i);
            }(std::allocator_arg, static_cast<decltype(range_and_allocator.allocator) &&>(range_and_allocator.allocator), std::ranges::begin(range_and_allocator.range), std::ranges::end(range_and_allocator.range))));
        }
    };

    template<type_agnostic_allocator_c allocator_t>
    struct generator_promise_needs_to_store_allocator : public std::bool_constant<!(std::allocator_traits<allocator_t>::template rebind_traits<std::byte>::is_always_equal::value && std::is_trivially_default_constructible_v<typename std::allocator_traits<allocator_t>::template rebind_alloc<std::byte>>)>
    {};
    template<type_agnostic_allocator_c allocator_t>
    inline constexpr bool generator_promise_needs_to_store_allocator_v = generator_promise_needs_to_store_allocator<allocator_t>::value;

    template<type_agnostic_allocator_c allocator_t, bool allocator_is_given_explicitly>
    struct generator_promise_base_memory_t;
    template<type_agnostic_allocator_c allocator_t, bool allocator_is_given_explicitly> requires generator_promise_needs_to_store_allocator_v<allocator_t>
    struct generator_promise_base_memory_t<allocator_t, allocator_is_given_explicitly>
    {
        using allocator_traits_byte_t = std::allocator_traits<allocator_t>::template rebind_traits<std::byte>;
        using allocator_byte_t = allocator_traits_byte_t::allocator_type;
        using pointer_traits_byte_t = std::pointer_traits<typename allocator_traits_byte_t::pointer>;
        static constexpr std::size_t allocator_byte_offset(std::size_t frame_size) noexcept
        {
            auto round_up_to_multiple = [](std::size_t frame_size, std::size_t alignment) { return (frame_size + alignment - 1uz) & ~(alignment - 1uz); };
            return round_up_to_multiple(frame_size, alignof(allocator_byte_t));
        }
        static constexpr std::size_t padded_frame_size(std::size_t frame_size) noexcept { return allocator_byte_offset(frame_size) + sizeof(allocator_byte_t); }
        static allocator_byte_t *get_p_allocator_byte(std::byte *p_frame, std::size_t frame_size) noexcept { return reinterpret_cast<allocator_byte_t *>(p_frame + allocator_byte_offset(frame_size)); }

        template<typename... args_t> requires (!allocator_is_given_explicitly)
        static void *operator new(std::size_t frame_size, args_t &&...)
        {
            if_ext_generator_debug(std::cout << __func__ << ", " << __LINE__ << ", allocator_t = " << type_name<allocator_t>() << ", frame_size = " << frame_size << std::endl);
            allocator_byte_t allocator_byte;
            std::byte *p_frame = std::to_address(allocator_traits_byte_t::allocate(allocator_byte, padded_frame_size(frame_size)));
            allocator_traits_byte_t::construct(allocator_byte, get_p_allocator_byte(p_frame, frame_size), allocator_byte); // ::new (static_cast<void *>(get_p_allocator_byte(p_frame, frame_size))) allocator_byte_t(std::move(allocator_byte));
            return p_frame;
        }

        template<typename allocator_t_, typename... args_t> requires allocator_is_given_explicitly
        static void *operator new(std::size_t frame_size, std::allocator_arg_t, allocator_t_ &&allocator, args_t &&...)
        {
            if_ext_generator_debug(std::cout << __func__ << ", " << __LINE__ << ", allocator_t = " << type_name<allocator_t>() << ", allocator_t_ = " << type_name<allocator_t_>() << ", frame_size = " << frame_size << std::endl);
            allocator_byte_t allocator_byte(std::forward<allocator_t_>(allocator));
            std::byte *p_frame = std::to_address(allocator_traits_byte_t::allocate(allocator_byte, padded_frame_size(frame_size)));
            allocator_traits_byte_t::construct(allocator_byte, get_p_allocator_byte(p_frame, frame_size), allocator_byte); // ::new (static_cast<void *>(get_p_allocator_byte(p_frame, frame_size))) allocator_byte_t(std::move(allocator_byte));
            return p_frame;
        }

        template<typename this_t, typename allocator_t_, typename... args_t> requires allocator_is_given_explicitly
        static void *operator new(std::size_t frame_size, this_t &&, std::allocator_arg_t, allocator_t_ &&allocator, args_t &&...args)
        {
            if_ext_generator_debug(std::cout << __func__ << ", " << __LINE__ << ", allocator_t = " << type_name<allocator_t>() << ", allocator_t_ = " << type_name<allocator_t_>() << ", frame_size = " << frame_size << std::endl);
            return generator_promise_base_memory_t::operator new(frame_size, std::allocator_arg, std::forward<allocator_t_>(allocator), std::forward<args_t>(args)...);
        }

        static void operator delete(void *p_frame, std::size_t frame_size) noexcept
        {
            if_ext_generator_debug(std::cout << __func__ << ", " << __LINE__ << ", allocator_t = " << type_name<allocator_t>() << ", frame_size = " << frame_size << std::endl);
            allocator_byte_t &allocator_byte_ = *get_p_allocator_byte(static_cast<std::byte *>(p_frame), frame_size), allocator_byte(std::move(allocator_byte_));
            allocator_traits_byte_t::destroy(allocator_byte, std::addressof(allocator_byte_)); // allocator_byte_.~allocator_byte_t();
            allocator_traits_byte_t::deallocate(allocator_byte, pointer_traits_byte_t::pointer_to(*static_cast<std::byte *>(p_frame)), padded_frame_size(frame_size));
        }
    };
    template<type_agnostic_allocator_c allocator_t, bool allocator_is_given_explicitly> requires (!generator_promise_needs_to_store_allocator_v<allocator_t>)
    struct generator_promise_base_memory_t<allocator_t, allocator_is_given_explicitly>
    {
        using allocator_traits_byte_t = std::allocator_traits<allocator_t>::template rebind_traits<std::byte>;
        using allocator_byte_t = allocator_traits_byte_t::allocator_type;
        using pointer_traits_byte_t = std::pointer_traits<typename allocator_traits_byte_t::pointer>;
        static void *operator new(std::size_t frame_size)
        {
            if_ext_generator_debug(std::cout << __func__ << ", " << __LINE__ << ", allocator_t = " << type_name<allocator_t>() << ", frame_size = " << frame_size << std::endl);
            allocator_byte_t allocator_byte;
            return std::to_address(allocator_traits_byte_t::allocate(allocator_byte, frame_size));
        }
        static void operator delete(void *p_frame, std::size_t frame_size) noexcept
        {
            if_ext_generator_debug(std::cout << __func__ << ", " << __LINE__ << ", allocator_t = " << type_name<allocator_t>() << ", frame_size = " << frame_size << std::endl);
            allocator_byte_t allocator_byte;
            allocator_traits_byte_t::deallocate(allocator_byte, pointer_traits_byte_t::pointer_to(*static_cast<std::byte *>(p_frame)), frame_size);
        }
    };

    template<typename reference_t_, typename value_t_, typename yielded_t_> requires (std::is_void_v<yielded_t_> || std::is_reference_v<yielded_t_>)
    struct generator_t : public std::ranges::view_interface<generator_t<reference_t_, value_t_, yielded_t_>> // https://github.com/cor3ntin/coro_benchmark/blob/main/generator.hpp
    {
        using value_t = std::conditional_t<std::is_void_v<value_t_>, std::remove_cvref_t<reference_t_>, value_t_>;
        using reference_t = std::conditional_t<std::is_void_v<value_t_>, reference_t_ &&, reference_t_>;
        using yielded_t = std::conditional_t<std::is_void_v<yielded_t_>, std::conditional_t<std::is_reference_v<reference_t>, reference_t, reference_t const &>, yielded_t_>;
        using rvalue_reference_t = std::conditional_t<std::is_reference_v<reference_t>, std::remove_reference_t<reference_t> &&, reference_t>;
        static_assert(std::common_reference_with<reference_t &&, value_t &> && std::common_reference_with<reference_t &&, rvalue_reference_t &&> && std::common_reference_with<rvalue_reference_t &&, value_t const &>);
        using promise_t = generator_promise_base_t<yielded_t>;

        std::coroutine_handle<promise_t> handle;
        bool joinable() const noexcept { return handle != nullptr; }
        generator_t() noexcept : handle(nullptr) {}
        generator_t(std::coroutine_handle<promise_t> handle) noexcept : handle(handle) {}
        generator_t(generator_t &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}
        generator_t &operator=(generator_t &&other) noexcept
        {
            if (this != std::addressof(other))
            {
                if (joinable())
                    handle.destroy();
                handle = std::exchange(other.handle, nullptr);
            }
            return *this;
        }
        ~generator_t()
        {
            if (joinable())
                handle.destroy();
        }

        bool empty() const noexcept
        {
            assert(joinable());
            return handle.done();
        }
        struct iterator_t
        {
            std::coroutine_handle<promise_t> handle = nullptr;

            using difference_type = std::ptrdiff_t;
            iterator_t &operator++()
            {
                assert(!is_end());
                std::coroutine_handle<promise_t>::from_promise(*handle.promise().p_promise_root_or_current).resume();
                return *this;
            }
            void operator++(int) { operator++(); }

            using value_type = value_t;
            reference_t operator*() const
            {
                assert(!is_end());
                return static_cast<reference_t>(*handle.promise().p_promise_root_or_current->p_yielded);
            }

            using iterator_concept = std::input_iterator_tag;

            bool is_end() const noexcept
            {
                assert(handle.promise().p_promise_continuation == nullptr); // assert(subtree.p_root->p_parent == nullptr)
                return handle.done();
            }
            friend bool operator==(iterator_t const &iterator, std::default_sentinel_t const &) noexcept { return iterator.is_end(); }
        };
        iterator_t begin() noexcept
        {
            assert(joinable());
            return iterator_t{.handle = handle};
        }
        std::default_sentinel_t end() const noexcept
        {
            assert(joinable());
            return {};
        }
    };
} // namespace ext


template<typename reference_t_, typename value_t_, typename yielded_t_, typename... args_t>
struct std::coroutine_traits<ext::generator_t<reference_t_, value_t_, yielded_t_>, args_t...>
{
    struct promise_type : public ext::generator_promise_base_t<typename ext::generator_t<reference_t_, value_t_, yielded_t_>::yielded_t>, public ext::generator_promise_base_memory_t<std::allocator<void>, false>
    {
        ext::generator_t<reference_t_, value_t_, yielded_t_> get_return_object() noexcept
        {
            return {std::coroutine_handle<typename ext::generator_t<reference_t_, value_t_, yielded_t_>::promise_t>::from_promise(*this)}; // !!! not using the original promise but its base subobject to create std::coroutine_handle
        }
    };
};
template<typename reference_t_, typename value_t_, typename yielded_t_, typename allocator_cvref_t, typename... args_t>
struct std::coroutine_traits<ext::generator_t<reference_t_, value_t_, yielded_t_>, std::allocator_arg_t, allocator_cvref_t, args_t...>
{
    struct promise_type : public ext::generator_promise_base_t<typename ext::generator_t<reference_t_, value_t_, yielded_t_>::yielded_t>, public ext::generator_promise_base_memory_t<typename std::allocator_traits<std::remove_cvref_t<allocator_cvref_t>>::template rebind_alloc<void>, true>
    {
        ext::generator_t<reference_t_, value_t_, yielded_t_> get_return_object() noexcept
        {
            if_ext_generator_debug(std::cout << __func__ << ", " << __LINE__ << ", allocator_cvref_t = " << ext::type_name<allocator_cvref_t>() << std::endl);
            return {std::coroutine_handle<typename ext::generator_t<reference_t_, value_t_, yielded_t_>::promise_t>::from_promise(*this)}; // !!! not using the original promise but its base subobject to create std::coroutine_handle
        }
    };
};
template<typename reference_t_, typename value_t_, typename yielded_t_, typename this_cvref_t, typename allocator_cvref_t, typename... args_t>
struct std::coroutine_traits<ext::generator_t<reference_t_, value_t_, yielded_t_>, this_cvref_t, std::allocator_arg_t, allocator_cvref_t, args_t...>
{
    struct promise_type : public ext::generator_promise_base_t<typename ext::generator_t<reference_t_, value_t_, yielded_t_>::yielded_t>, public ext::generator_promise_base_memory_t<typename std::allocator_traits<std::remove_cvref_t<allocator_cvref_t>>::template rebind_alloc<void>, true>
    {
        ext::generator_t<reference_t_, value_t_, yielded_t_> get_return_object() noexcept
        {
            if_ext_generator_debug(std::cout << __func__ << ", " << __LINE__ << ", allocator_cvref_t = " << ext::type_name<allocator_cvref_t>() << std::endl);
            return {std::coroutine_handle<typename ext::generator_t<reference_t_, value_t_, yielded_t_>::promise_t>::from_promise(*this)}; // !!! not using the original promise but its base subobject to create std::coroutine_handle
        }
    };
};
