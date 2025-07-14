# ext_generator

[goto examples](#examples)

## `struct ext::generator_t`

```C++
template<typename reference_t_, typename value_t_ = void, typename yielded_t_ = void> requires (std::is_void_v<yielded_t_> || std::is_reference_v<yielded_t_>)
struct ext::generator_t: public std::ranges::view_interface<ext::generator_t<reference_t_, value_t_, yielded_t_>>
```

### Template parameters
<table>
  <tr>
    <td><code>reference_t_</code></td><td>the reference type (<code>std::ranges::range_reference_t</code>) of the generator</td>
  </tr>
  <tr>
    <td><code>value_t_ = void</code></td><td>the value type (<code>std::ranges::range_value_t</code>) of the generator, or <code>void</code></td>
  </tr>
  <tr>
    <td><code>yielded_t_ = void</code></td><td>the parameter type of <code>yield_value()</code> of the generator's promise, or <code>void</code></td>
  </tr>
</table>

`requires (std::is_void_v<yielded_t_> || std::is_reference_v<yielded_t_>)`

### Member types
<table>
  <tr>
    <td><code>value_t</code></td><td><code>std::conditional_t&lt;std::is_void_v&lt;value_t_&gt;, std::remove_cvref_t&lt;reference_t_&gt;, value_t_&gt;</code><br>
Is a cv-unqualified object type: <code>static_assert(std::is_object_v&lt;value_t&gt; && std::is_same_v&lt;std::remove_cv_t&lt;value_t&gt;, value_t&gt;);</code></td>
  </tr>
  <tr>
    <td><code>reference_t</code></td><td><code>std::conditional_t&lt;std::is_void_v&lt;value_t_&gt;, reference_t_ &&, reference_t_&gt;</code><br>
Is a reference type or a proxy reference type: <code>static_assert(std::is_reference_v&lt;reference_t&gt; || std::is_object_v&lt;reference_t&gt; && std::is_same_v&lt;std::remove_cv_t&lt;reference_t&gt;, reference_t&gt; && std::copy_constructible&lt;reference_t&gt;);</code></td>
  </tr>
  <tr>
    <td><code>yielded_t</code></td><td><code>std::conditional_t&lt;std::is_void_v&lt;yielded_t_&gt;, std::conditional_t&lt;std::is_reference_v&lt;reference_t&gt;, reference_t, reference_t const &&gt;, yielded_t_&gt;</code></td>
  </tr>
  <tr>
    <td><code>rvalue_reference_t</code></td><td><code>std::conditional_t&lt;std::is_reference_v&lt;reference_t&gt;, std::remove_reference_t&lt;reference_t&gt; &&, reference_t&gt;</code></td>
  </tr>
</table>

`static_assert(std::common_reference_with<reference_t &&, value_t &> && std::common_reference_with<reference_t &&, rvalue_reference_t &&> && std::common_reference_with<rvalue_reference_t &&, value_t const &>);`

### Data members
<table>
  <tr>
    <td><code>std::coroutine_handle&lt;promise_t&gt; handle;</code></td><td></td>
  </tr>
</table>

### Member functions
<table>
  <tr>
    <td><code>generator_t() noexcept;</code><br>
<code>generator_t(generator_t &&other) noexcept;</code><br>
<code>generator_t &operator=(generator_t &&other) noexcept;</code><br>
<code>~generator_t();</code></td><td>An <code>ext::generator_t</code> is like a <code>std::unique_ptr</code>, owning the coroutine state.</td>
  </tr>
  <tr>
    <td><code>bool joinable() const noexcept;</code></td><td><code>handle != nullptr</code></td>
  </tr>
  <tr>
    <td><code>bool empty() const noexcept;</code></td><td>Precondition: <code>joinable()</code>.<br>
<code>handle.done()</code></td>
  </tr>
  <tr>
    <td><code>iterator_t begin() noexcept;</code></td><td>Precondition: <code>joinable()</code>.<br>
<b>Note: A <code>iterator_t</code> is like a plain pointer, becomes dangling when the coroutine state is destroyed.</b><br>
Note: this member function can be made <code>const</code> but here it's made non-<code>const</code> to prevent potential confusion.</td>
  </tr>
  <tr>
    <td><code>std::default_sentinel_t end() const noexcept;</code></td><td>Precondition: <code>joinable()</code>.</td>
  </tr>
</table>

----

## `struct actual-promise-t`

```C++
struct actual-promise-t; // (derived from struct ext::generator_t<reference_t_, value_t_, yielded_t_>::promise_t)
```

### Member functions
(These member functions are in `struct ext::generator_t<reference_t_, value_t_, yielded_t_>::promise_t` and only need `yielded_t` to work:)
<table>
  <tr>
    <td><code>std::suspend_never initial_suspend() const noexcept;</code></td><td><b>Note: on <code>ext::generator_t</code> construction, the coroutine function body is executed until it suspends at the first <code>co_yield</code> or (possibly-implicit) <code>co_return;</code>.</b></td>
  </tr>
  <tr>
    <td><code>final-awaitable-t final_suspend() const noexcept;</code></td><td>If <code>*this</code> is nested in another <code>ext::generator_t</code>, transfers execution back to the resumer (i.e. the <code>ext::generator_t</code> which suspends at <code>co_yield std::ranges::elements_of(std::move(source-of-*this))</code>); otherwise (<code>*this</code> is at top-level), transfers execution back to the caller (i.e. caller of the coroutine function) or resumer (i.e. <code>iterator_t &iterator_t::operator++()</code>).</td>
  </tr>
  <tr>
    <td><code>void await_transform() const noexcept = delete;</code></td><td>(<code>co_await</code> is not allowed.)</td>
  </tr>
  <tr>
    <td><code>void return_void() const noexcept {}</code></td><td>Does nothing on (possibly-implicit) <code>co_return;</code>.</td>
  </tr>
  <tr>
    <td><code>void unhandled_exception() const;</code></td><td>If <code>*this</code> is nested in another <code>ext::generator_t</code>, stores <code>std::exception_ptr</code> in the resumer (i.e. the <code>ext::generator_t</code> which suspends at <code>co_yield std::ranges::elements_of(std::move(source-of-*this))</code>) 's <code>yield-awaitable</code>; otherwise (<code>*this</code> is at top-level), propagates the exception back to the caller (i.e. caller of the coroutine function) or resumer (i.e. <code>iterator_t &iterator_t::operator++()</code>) by executing <code>throw;</code>.</td>
  </tr>
</table>

<table>
  <tr>
    <td><code>std::suspend_always yield_value(yielded_t yielded) noexcept;</code></td><td>Stores <code>std::addressof(yielded)</code>.<br>
Note: <code>yielded_t</code> is always a reference type.</td>
  </tr>
  <tr>
    <td><code>template&lt;typename reference_other_t, typename value_other_t, typename yielded_other_t, typename allocator_t&gt; requires std::same_as&lt;typename generator_t&lt;reference_other_t, value_other_t, yielded_other_t&gt;::yielded_t, yielded_t&gt;
yield-awaitable-t yield_value(std::ranges::elements_of&lt;generator_t&lt;reference_other_t, value_other_t, yielded_other_t&gt; &&, allocator_t&gt; generator_and_allocator) const noexcept;</code></td><td>Precondition: <code>generator_and_allocator.range.joinable()</code>.<br>
Constructs and returns a <code>yield-awaitable</code> which acquires ownership of the coroutine state from <code>generator_and_allocator.range</code>.<br>
(<code>generator_and_allocator.allocator</code> is ignored.)</td>
  </tr>
  <tr>
    <td><code>template&lt;std::ranges::input_range range_t, typename allocator_t&gt; requires std::convertible_to&lt;std::ranges::range_reference_t&lt;range_t&gt;, yielded_t&gt;
auto yield_value(std::ranges::elements_of&lt;range_t, allocator_t&gt; range_and_allocator) const;</code></td><td>

```C++
return generator_promise_base_t::yield_value(std::ranges::elements_of([](std::allocator_arg_t, decltype(range_and_allocator.allocator), std::ranges::iterator_t<range_t> i, std::ranges::sentinel_t<range_t> s) -> generator_t<yielded_t> {
    for (; i != s; ++i)
        co_yield static_cast<yielded_t>(*i);
}(std::allocator_arg, static_cast<decltype(range_and_allocator.allocator) &&>(range_and_allocator.allocator), std::ranges::begin(range_and_allocator.range), std::ranges::end(range_and_allocator.range))));
```
(<code>range_and_allocator.allocator</code> is forwarded according to <code>allocator_t</code>.)</td>
  </tr>
</table>

(These member functions are not in `struct ext::generator_t<reference_t_, value_t_, yielded_t_>::promise_t` but in `struct actual-promise-t` and need `allocator_t` (provided by `std::coroutine_traits`'s template parameter list or defaults to `std::allocator<void>`) to work:)

<table>
  <tr>
    <td>(When <code>generator-promise-needs-to-store-allocator-v&lt;allocator_t&gt; && !allocator-is-given-explicitly</code>)<br>
<code>template&lt;typename... args_t&gt;
static void *operator new(std::size_t frame_size, args_t &&...);</code></td><td>Constructs an allocator on stack, uses it to allocate memory and copies it to coroutine state.</td>
  </tr>
  <tr>
    <td>(When <code>generator-promise-needs-to-store-allocator-v&lt;allocator_t&gt; && allocator-is-given-explicitly</code>)<br>
<code>template&lt;typename allocator_t_, typename... args_t&gt;
static void *operator new(std::size_t frame_size, std::allocator_arg_t, allocator_t_ &&allocator, args_t &&...);</code><br>
<code>template&lt;typename this_t, typename allocator_t_, typename... args_t&gt;
static void *operator new(std::size_t frame_size, this_t &&, std::allocator_arg_t, allocator_t_ &&allocator, args_t &&...args);</code></td><td>Uses <code>allocator</code> to allocate memory and copies it to coroutine state.</td>
  </tr>
  <tr>
    <td>(When <code>generator-promise-needs-to-store-allocator-v&lt;allocator_t&gt;</code>)<br>
<code>static void operator delete(void *p_frame, std::size_t frame_size) noexcept;</code></td><td>Moves the allocator to stack and uses it to deallocate memory.</td>
  </tr>
  <tr>
    <td>(When <code>!generator-promise-needs-to-store-allocator-v&lt;allocator_t&gt;</code>)<br>
<code>static void *operator new(std::size_t frame_size);</code></td><td>Constructs an allocator on stack and uses it to allocate memory.</td>
  </tr>
  <tr>
    <td>(When <code>!generator-promise-needs-to-store-allocator-v&lt;allocator_t&gt;</code>)<br>
<code>static void operator delete(void *p_frame, std::size_t frame_size) noexcept;</code></td><td>Constructs an allocator on stack and uses it to deallocate memory.</td>
  </tr>
</table>

(These member functions are not in `struct ext::generator_t<reference_t_, value_t_, yielded_t_>::promise_t` but in `struct actual-promise-t` and need `reference_t_`, `value_t_ `, `yielded_t_` and types of arguments (provided by `std::coroutine_traits`'s template parameter list) to work:)

<table>
  <tr>
    <td><code>ext::generator_t&lt;reference_t_, value_t_, yielded_t_&gt; get_return_object() noexcept;</code></td><td>...</td>
  </tr>
</table>

----

## `struct ext::generator_t<reference_t_, value_t_, yielded_t_>::iterator_t`

```C++
struct ext::generator_t<reference_t_, value_t_, yielded_t_>::iterator_t;
```

### Member types
<table>
  <tr>
    <td><code>value_type</code></td><td><code>value_t</code></td>
  </tr>
  <tr>
    <td><code>difference_type</code></td><td><code>std::ptrdiff_t</code></td>
  </tr>
  <tr>
    <td><code>iterator_concept</code></td><td><code>std::input_iterator_tag</code></td>
  </tr>
</table>

### Data members
<table>
  <tr>
    <td><code>std::coroutine_handle&lt;promise_t&gt; handle = nullptr;</code></td><td></td>
  </tr>
</table>

### Member functions
<table>
  <tr>
    <td><code>bool is_end() const noexcept;</code></td><td>Precondition: the <code>ext::generator_t</code> is at top-level.<br><code>handle.done()</code><br><b>Note: does not check whether <code>*this</code> is singular.</b></td>
  </tr>
  <tr>
    <td><code>iterator_t &operator++();</code><br>
<code>void operator++(int);</code></td><td>Precondition: <code>!is_end()</code>.<br><code>coroutine-handle-of-innermost-coroutine.resume()</code><br>Note: these member functions can be made <code>const</code> since member functions of <code>std::coroutine_handle&lt;promise_t&gt;</code> are <code>const</code>, but here they are made non-<code>const</code> to prevent potential confusion.</td>
  </tr>
  <tr>
    <td><code>reference_t operator*() const;</code></td><td>Precondition: <code>!is_end()</code>.<br><code>static_cast&lt;reference_t&gt;(*promise-of-innermost-coroutine.stored-address)</code></td>
  </tr>
</table>

### Non-member functions
<table>
  <tr>
    <td><code>friend bool operator==(iterator_t const &iterator, std::default_sentinel_t const &) noexcept;</code></td><td><code>iterator.is_end()</code></td>
  </tr>
</table>

**Note: there's no way to check whether a <code>iterator_t</code> is singular.**

**Note: there's no way to check whether two <code>iterator_t</code>s point to the same coroutine state.**

----

## Examples

[Compiler Explorer](https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGe1wAyeAyYAHI%2BAEaYxCBmZgBspAAOqAqETgwe3r56KWmOAkEh4SxRMXGJdpgOGUIETMQEWT5%2BXLaY9gUMdQ0ERWGR0bEJtvWNzTltCmN9wQOlQxUAlLaoXsTI7BzmAMzByN5YANQmO24IBARJCiAA9LfETADuAHTAhAheEV5KG7KMBBeaBYt20CFExFoGgAHFJbphVAQAPrARjRJhEYi3FhMYK3faHTDwxG3VEhR6Yl4IJJJU7YEwaACCDOZZj2DAOXmOpzcTmmxEwrDpLJZBEwLCSBjFPIRBBAIAIAE8kpgkUxgAxUo5kGraLRUKJMUjkEcxPrDSQkQRhUyAG6oPDoI4I1iS1WEdGYiBmg0Yy0EE4JH0W4hLEUAdisTKOMediPlZM9/p5wWtO2wR0TFMtLol9CRHuzxBOOwAIicAKxWCuliDTdDy4N%2B4hq4jAK2kU1631GgNoBjTQPxJuYpZHAC0dLjcpAWebVpTginJkjLNj66OaCRirwHSdGlOUcZG9jsoTaKLC7OqanTC4JfLJirT9rY8n6en5/J87TvKXH5XI8TxPfhiwgVNnSOEAjnreVbV3J4bhAJx6ggLgNE7Lh4iWMNo2A/Ctx3PcYIIBsQBYVBbUwCBMDDHYgOAldX0PNdgMI3daCdAAqO8Xiid4GAgOiLCOe4jnQ1iT3Y4ieK4PjMAEoTD1E25xIPPCpNQbcOO46xrF4/jgiU%2BiVPErhJI3aTOKOLi9MsAyFKM4TTK4MwLPXOyLAcxThPc2MxLnSkEEMdB6BeJJiFQFg8CUITwu0vdMCdGKjhSVNgmAI4iHEnY/JjTzvKcliNI3AKL2bKkQrCiKopi6ilniojOKSo4UrSwQMqy1BxMkPLNy0pqnXM%2Bi%2BtAo5wMEIdzHiTAoJIsjHiMTAkIFKgkXgzAnm9LgxxXNx5rghCkPqABraidrHMSpFMi4rhue5piYZATso6IqH1V5gVuABHLxlq6BRbnDaEzHDHZ4jiW4EFQJ5x3QVBxzwcccTO8cmHHDaniYCJ6HHRbUXHCImCUdBxwERGGCSLwCERsUi0B4HQckLCAE5dkZ8NmfiFmjlu647luR7nte4h3phoEop%2Bv7pgyBmdgrHZw3DCsK1uJ5goIBRxwIBBMHHdqadQKgMYQ7HcfxvX%2B02JICCB%2BXJB2FmuFytkwYrB2ndykqCIGnSDvI16aN873Yysp03JGkOY1glA1gDHkeUDMxePFG3FSEh8k5LNxE5TiUlQzhOzn91xaGK49gLGib44SabZugmOLZWzA1sx7bdvDfaY8x46mDOiAzBwlyKz69cw/9iiqKD8v8JjtBqezxPzGTuTU4L3ayyzov9rztPC7OROY9Lme2N94jcsjiv8vDWsY5HS0GmATt79DE%2BYyriDprrubApzVRXXzIWZsuEr4bjnnHRexdZrbxLgwUKb9/bz3jgfYuR84G0EgV3Ui8pj6XyYiKJkYo8wYkwDKeMCplSqnVJqGWOoX7Gi7OaH8NpGT2kdHGABqpaB4CoJgRwbBvTdhDFaIcL8QGAQsmeWc5VeyLjTBmX%2BLZcxuiRNw3h/DoGbyfNWW%2B2CQD0Mfh2RhPZ/T9QHDXYcQjgETinFIxRV4/zyJOKuKO/UErWQPJffCdiZHJmvP%2BDMd5M7aJfBnd8GYfHflkf4pxEjXHrg/pNeu/se7yhQkwNCGFxKDxAfhH27inQx0ntRWiCCPI32MgxTSBTxJlNDmfayjdDCoiQh0cUAIFBIiNnWPRTd5SrXWghduzisFkVSQqPu50cLKSuupUBY8GnhzqdHPRSDMFJ14toB0zAcb1WCXEdZmztlmz2TAtB8CvGnxqU0parT6BsEEJ07pRTA53mmSZK65lXHgIXjA5eRzggnP3o%2BA5MCAU7PoPvHOqC9G4KqaVVS48bktJwfcjpXSqDtxmapexNVopKCRETAlM5BrrTEH9IS0Fia/AINAXxLYQq6iYZSC2LwtmAt2UJYO8z/KIsWf7PpIA2kPM1hinpZFint2cmVKJlo8V1UJcTVUJKdJku8PVKlCgaV0tlQyuBTKTHEBeKy9lEL6rcr6uPC%2B8L1yRKTC2X8N4PxMDMME58NYwm2PIfYx1ATnE2sriQcaEFkndyOmk1AqEzBZLiNM%2BJVzBoT0DqUy5J4mKVMtfy3qqbLL8uRctVF7THlivzc3VuQyXUd1GYdTavd%2B4XWxUcaNmaakjxzeuH5yDoX7X%2BWYNlxzOUbxBa6sFfbTVAo3t22BFyA0xiRb05pBahVouLc8vRkrK0jJSeGiZ/ccmNrMF8nlKyyJrL%2BXEF1/aOWQqHVvFBO8x0Dpves85Zd228rcYm0thaRVPMxZWxtuLIr4tVES5V8pSW2nJRq00Wroi0vsYyl%2BxrF1XrNVy5Zpl50LUXXcotoq10Stefuj5OL6VInlQSsDVoIOqqg%2BqylsHtWIf1chk1T7zXlxbYm%2BImG7WXl9U4pgOw3U6M9R%2BfjzCYnLhccejciSAyhr0eM9JEAdhZJ2Pu%2BN1Sv3ruTdy/C6aDMJr9uGTD2H%2Bm4Z/eiwjlnbn9JboMza3odhVu3bW%2BUp0pnStUup7jftoSYc7es/5Ow0MTv2SO%2B9powvjsHS%2B2F6DzN5oXfZ5d%2BG/0uZ80cHYR78mJrZu%2Bk98oz3RdC%2BF%2BLpxh2HNixxqFh9Eszv88Rb96Xf0lr01PYT7zZ2IIgee5OtXr2nK0aC6LwmKvPrOY1t9vWxIWZAIK4VNn/2ucA%2BRyjoGlU0ZAJB6DjHqXwZ1fatUrGrEstQ3Fm9xmTzzZSzhtLy3V2YpeV11z2WgO1So9tlVe41UUqWJq5j5GkPnZIChpak3OM5vTXfMHDK2zP3hzdo4CmpoJGSfY5R%2BY1F8LwGwXJwFgswM0VO19CDifRdfQlsicKIylgIYyIhkoSFkJnEqFUaoNRajwHQ%2BHDD6HyJZGwp02PVTAEeFQRwRhBHMtMdNMREZ4WSeiY4qcWP/55nF5L6XmUquVjE3DuXCP2wECR8bkR/ZBwK%2BRzYiT3ryOCZk71lXfi1fOvvPrkJHrBdmOt0GW34TPzSN1Q4p1GY4lyYWTUzxvXo%2B6YeyitrK2IDe90WRAxbYjG%2B6txYsRduIkO9D07gCsm8kmeIsNOPgawIhrmmGjzyFI0ZPQphLTUfy%2Bfr9q9kpKPO9d%2BIhHav190%2BNn54/c3hqeuj3qTU61fV02K6K8Hn1cjbxRcfO62sOeBD%2B8scbt8XqZyr%2Bk6X4fA/rLZvPwtpbK6COYrT%2BKsfFvDFm%2BMcIvsu%2B8%2BB6P1%2BE7JeI8y9%2B9Z9E021z949u9Usk8nt78n8QAVNm8B4Y0SNwCQC/ZeNl9ylR99Fx9Ed39gE%2B8K9rIzNl9F9kc%2BMi9/819nURMvct9ZdDVLcv9REf97dj9HdqDADr9%2BVAtMC0CWsoCl0YDMtH8jdGDX9J8P8/dv8D8C8V8ODT8uCZ8dM/ZCtUC517s7NoC78RD6CxDhEJD8DexpCWDZCg9XcHVOD/UL8nR1NOwrUq9nFsDM8n4jCSBp9tNc0akJI%2BCR84CXDJCCDMMIDK8q8F8Kkl8jxRog1q50cZp69BCy0nMto3kt0G9EJPNJk1Mh4xINBxI1JlCbCk0p4U1etKcp1/k5IrsRtqswUqi6tJ0Gtacktl8b8rNk9ns4CN0G1SMzIm0cs%2Bpyil4L16jhtgU70p1wUItptmimsmRoja9Jov4McEjE8l0Bk25N09p3MMjd1qJ3tTJJAjgKwm05l%2B8FtJVSjBjVl%2BsysL1H0xjb1l5DkHj0NGiYVZjZtmtGlEjrNOie9vQUDTJD0jh4gjgzNvkbjfk7jk5XjpjRsotJi4TKsqcZsuNXE0dlj4iG5fjFtHNNjXM0jlMd0vNsjLpVJoQjgeY/NPCLj9MgsoSu1hjBsodxjniwUhs3iaccEWjes2jHsdCOsiM3setTJcs1IzJrjT1biKiL1OT4Tajxt5SUSyc0SYcKl9D5wJ83DX4c1MTa4VjoINdOEUQdcMpCcTwhioFuShVeSLIrTq1bTQobS6d5ib5GdmcpRSEzgpEOcqFudaEDVhETRBcWERcOEtcgz5x6M/pjQSFgASB05fcbcD8ldJFKCBMrDjTIz6EYzVRDQFJEzRNQlNSjRDDkyA8zDf8Q8qDFD/Vvj9xG1kzN5SySAeQRcWF8tIC1i8N2tulH9BdD82C/9My6zAI3EBQCB1gGBlIyCD9CCNDrlcThCxUBz%2BdP9zEhwhzC92Di8rDxytxJzpzZyNS9FiZjQv8GCQwcIFyijWsVz%2Bz6CKzsI5CLCw8/UDytIjziAZyTI5yp9bz%2BTtCMtVynz1yTCsTtz5C9yxzIwJy%2BFjy/zTyMRec4zpgeQd9NysS6RvRqZUAryCCPC5MgKhDBTHyxNnzpooK3yADrDDyEKfyTzt88KCLRwiKuzK8mzwL9dWziB2yHR0BOyiDCllyyKH8ny8LqKMypN3cuD4KpzGKkLt9kdALNDFt2iHzxKKK8K4ipLdzazZK6KvyGLfyRJYczzOlc9WL3D2LhKBUNKxLU8JLspILXzpLVdw8jKkRvzTKnDrLQxVKlyey/jYCBydLXLzD3K3dPLPzvKTKmKekUKdRRB0KzhMK98v50xcKiB/KbzAqE8tDSKQLyLQkmBwqDSXzIr9LRzDLYqfKEqyqcqxEiKGym0uKLdkFywIhUAtQ0lBBogapNgtVn9DUeQXhxqhKVCBDgqOjQq9CzzcDTdAjew9KRyZKYq4L6KFLfLzKM9FrlqbL2rGDkRM5WREgdTjRbQBlOqjhmRXESLeyU9RCFqX8s8390qLFVqayaqNqRItrEKzLTy9rXrXC75LLLzmrstBckRTrpDpoDqWxkArrHMbrjxc8v57q1Lb9iqtKSyXrxC3qEaREqK3Lqr1qPzNrjLtqEreLWxXDmqjqP8Yb9czqiakbrrTqMbiKsaHKcanLDd8aDDCaLqNyMqEgvqT9arKa4rqalL/D9qSIkq0LfwPq4icLGr8LIaob1zmbN40av94aLr2aUbYb0aKrWr7zHLnrgaCalqRbdLSa1qPKKa/qqaAa/LabtSNbcrRSxJobOaRgjbka1pUbsLMa59GafxM5urerkJ%2BriBBrlokIX4xqJr0wLbRK%2BawqiBn5JLHbvrybYlpb6q5bPa8CGbej/aWbDb6FjaQ7Obw6Cr1KBSs7nLUBc6XKKqJaFCpbXaZb3bdqRqhbQaLKLzzEfbtaOrdbyx9bzEa7%2Bc67ia9a4ba5G7uzCrHrOjs727TRyreNxb87Jbfr5KB6gah6tTy6VLI7exp7br56LdF7Q7V7uagqN6QrdDtKc7d7O797Krqyj6XaT7FLAbnCFbHptRlaMLwLc81asrvatbr7/Rb7Z70LA7a7g6l6Z6V7f6M6ZrNL%2BbSq8KO7uoIr/6e7j7/rgGPbBaL76aWLIbEGHVkH77GDH6A6cH5jXENajgLZM50ikJVMskNB8q/Yr9TJmzyxeL%2BLHRJqvCm7sa%2BzMULZJ7jrmG0GF6MHQ617iIwDxHuLN4Y7pg%2Bq6ZE7hqcDjdU6XhZGQifi8GxKeQsADg/SIBlGib5EXHF0EaVGmb2G2bNGG6X75HebFGHHqhaBnHXH7aYHMrsAPGlpOwwax7pgfbGGrRkHsG/GOaWbsHcG37Zq/1QmnHKE4nUQVh7aYmSnMAvHUmTrq71GH7/G6neNtHbG8nNLCnwninInVbsKsqLYEn6gIGUqVboHmDenYn4GVLvGfx0mzbzr0Gsnl65muaOLWnm7gKQmzhHHOmVRKmynKLa4%2BnPHv7Nar7K6dbfGg7FmZ6w7QFB7zHbbqm9SYjP4KrMdyMxcoyjQ8y4yxQEziBFQLSwFGT1lScmieS5jQEHTp0MEZiIWvi3SGdOHWR2RORuQzhDHbZUwBrIohrAYX58X4cqQaQWFdgCQuRSdMX8R47THAZggsBVBlpbgGAvA9QCw4EERiXaR075i2RyX0W3AqXsWE7cWk7sRDB1QkokRZRohmBaBCUvAqBeEjVqRuX6ReXUXCRs42AKIAXSWmQIIcQio3T4Vc8EQIp/Y0gAAvZVI4b4JV6IJEa1zRcsDQCILgLwK19ZLCLwCnPRLwBgPAX6VUG2Pis4ZAYKPirfKce15Vvh9dSZJEANoN2M0CLpKiYgJ4YgD0HkCNhoNPHC2Nx151lHH4TqHEZgVEdAJEHVxMp1hSEVTBu1nqoxuOkx0VsxolVCityV6tmVn8sQBVh1sNtwPNsgZt2O4V2l/pCIAgAUUDf6JEGgX8TF4xnFg0JO%2BUFltllgamBEJd1gOgRUTscM2ydMTsVdtt9dvFrd1l%2BV%2BlhEYUHNHtqtmt8UOtpQYARtgMF9qV2tgF%2Btr9gECAS9qdjtpCZAAUEhLpBgWgY9u1xV5Vt4PhISC9xD4tvAG10ttIIwcpgxltmcMDjdsxlOs4EXTsX96t/9xUQD79%2BUT9xt39sN%2BkHNX3F%2BCASjt93VmjhjgEZD5EXjwQGtiV1EYgDDHNe4Mt3DiR/2UjtwDs9OiT24Nj%2BHdE0BT5oBL0SIiyTj6j2jvjiN6oE6J1wwQgdODeAAMSOBFwzmgh1kiieH9n1HeB1GiEijE%2BXiNYYGfjg0aA45E7/ffYA8E8BEM%2BehM8DfXjDFjQQV06C544bb47NC47rawBHCSn3is5s7HDs4QAc6c9QBc%2BleIHc9TziC858%2B1Ti%2B4/08EBeGS707S/Owy%2BmRi7wRKk%2Bdxw0Vyti4C6o/i9q9C91nC4UFM/XhLCy4Ets6yjy5hgK6K7c5IDK7MAq6YyO2q4/cS7q7C%2BM9G8i/M%2Bi8Hl68rcC5q5C/q53YG6a%2BZRa4m%2Bs6m5y5m/y5jmc9QsW48/K9xG87W7842%2BC628BAa6u8FGa/QAwza6iI681xUQlyYClwyh6%2Bfb65S/%2B6A%2B2%2BG927G/M7u%2By7mns7m5e8K7e5K6W886%2B8q/W%2BR70/O524i7M/B6O6R5O/67O4B4u/lca5B5u7B8s/u8dGm/x8c8J4W5J4%2B5W/J5%2B9pT%2B4S7R8B8u5q%2Bu57Ba8O4tSh5NNzOg1%2BcLIBcR/hWl8G6BAx7p/G9OEm/58e8F/m%2BJ9K7J%2BCAp9%2B6p4G5p6N72/p9a8Z718d9Z9l/Z5R5o8V4LJ55x4e7x9m6F70Ve9c9F%2BW9W8Owd%2BZ794N6B4V656V555V7U/XB8qOFj3pxZA4BWFoE4ArF4D8A4C0FIFQE4BzksGsBgjWA2GgTZB4FIDlHL4L5WBOhAArAwiL44EkF4BYBAHiHDBeGhDBmhFyyVgrHDA0A0BZmkDL4r6r44F4BuAwjb60BWDgFgCQARGqDwrIAoG9DbAUGUEMA6CEGhieDL5b%2BBCSDoBQoEHP5CFoCv5hiX94Hv7oCGAOGaStGIABsTopAb/vQGIChBWAWwL/lFAf5gCAA8tTHf439NAvAffsgEZCn9OAqAxlsgDqD4Ay%2BvAfgIIBEBiB2AUgGQIIEUAqB1A7fUgLoDaAGAjAKATyPoDwARAbgsAZgGwBACYtSAGbTgFwHH4d9K%2BNsWWJwHHD1gqspgWvpYASATg4BrqccAAHUzQE4FUOgEMDag1B9nQUOgF4Aixs2WADgah0AEdQ2AAAFR6q0BjBKwBQA302B6B6wwQF/pf2v639eAc7TAFsBb5ZsmASQAQQX30DF9S%2BKAyvpwGwA4DD%2BRwVQNCHiDjh4gRxP/rhwgBzsgBY4CADXysCWBOwuAQgEGl2A7QPBKAnCKQF1hMAsAMQVDn3wH6kAh%2BmmF4K5HiBYQQYZgSQGYBZgswNACQ0gJ/zCGr9bAIADfsUM74SA5%2BQQjgDsBCG0CV%2BRQ9viUIzZpBnAkgIAA%3D)
```C++
#include <https://raw.githubusercontent.com/jhcarl0814/ext_generator/main/include/ext/generator.hpp>

#include <iostream>

template<ext::type_agnostic_allocator_c allocator_t>
void example_iterator(allocator_t &allocator)
{
    ext::generator_t<int> generator_example_iterator = [](std::allocator_arg_t, allocator_t const &allocator) -> ext::generator_t<int> {
        co_yield 0;
        ext::generator_t<int> a1 = []() -> ext::generator_t<int> {
            for (int e : std::views::iota(10, 16))
                co_yield std::move(e);
        }();
        co_yield *a1.begin(); // 10
        co_yield *a1.begin(); // 10
        co_yield *++a1.begin(); // 11
        co_yield *++a1.begin(); // 12
        ++a1.begin();
        // generator.handle.promise().p_yielded is pointing to 13
        ++a1.begin();
        // generator.handle.promise().p_yielded is pointing to 14
        co_yield 1;
        for (int &&e : std::ranges::ref_view(a1) | std::views::take(1)) // 14 // https://stackoverflow.com/questions/78273622/how-do-i-make-a-viewable-range-based-on-input-iterators/78274169#78274169 https://stackoverflow.com/questions/73537755/whats-the-point-of-viewable-range-concept/73543913#73543913
            co_yield std::move(e);
        co_yield 2;
        std::cout << "a1.empty() = " << a1.empty() << std::endl;
        for (int &&e : std::ranges::ref_view(a1) | std::views::take(2)) // 15
            co_yield std::move(e);
        std::cout << "a1.empty() = " << a1.empty() << std::endl;
        co_yield 3;
    }(std::allocator_arg, allocator);
    for (int &&e : generator_example_iterator)
        std::cout << e << std::endl;
    std::cout << std::endl << std::endl;
}

template<ext::type_agnostic_allocator_c allocator_t>
void example_lifetime(allocator_t &allocator)
{
    ext::generator_t<int> generator_example_lifetime = [](std::allocator_arg_t, allocator_t const &allocator) -> ext::generator_t<int> {
        co_yield 0;
        ext::generator_t<int> a1 = []() -> ext::generator_t<int> {
            for (int e : std::views::iota(10, 12))
                co_yield std::move(e);
        }();
        co_yield 1;
        co_yield std::ranges::elements_of(std::ranges::ref_view(a1) | std::views::take(1)); // 10
        co_yield 2;
        std::cout << "a1.joinable() = " << a1.joinable() << std::endl;
        co_yield std::ranges::elements_of(std::move(a1)); // 11
        std::cout << "a1.joinable() = " << a1.joinable() << std::endl;
        // co_yield std::ranges::elements_of(a1); // generator_promise_base_t::yield_value(): assert(generator_and_allocator.range.joinable());
        // co_yield std::ranges::elements_of(std::move(a1)); // generator_promise_base_t::yield_value(): assert(generator_and_allocator.range.joinable());

        co_yield 3;
        ext::generator_t<int> a2 = []() -> ext::generator_t<int> {
            for (int e : std::views::iota(20, 22))
                co_yield std::move(e);
        }();
        co_yield 4;
        co_yield std::ranges::elements_of(std::ranges::ref_view(a2) | std::views::take(1)); // 20
        co_yield 5;
        std::cout << "a2.joinable() = " << a2.joinable() << std::endl;
        co_yield std::ranges::elements_of(std::move(a2) | std::views::take(2)); // 21
        std::cout << "a2.joinable() = " << a2.joinable() << std::endl;
        // co_yield std::ranges::elements_of(a2); // generator_promise_base_t::yield_value(): assert(generator_and_allocator.range.joinable());
        // co_yield std::ranges::elements_of(std::move(a2)); // generator_promise_base_t::yield_value(): assert(generator_and_allocator.range.joinable());

        co_yield 6;
        ext::generator_t<int> a3 = []() -> ext::generator_t<int> {
            for (int e : std::views::iota(30, 32))
                co_yield std::move(e);
        }();
        co_yield 7;
        co_yield std::ranges::elements_of(std::ranges::ref_view(a3) | std::views::take(1)); // 30
        co_yield 8;
        std::cout << "a3.joinable() = " << a3.joinable() << std::endl;
        co_yield std::ranges::elements_of(a3); // 31
        co_yield 9;
        std::cout << "a3.joinable() = " << a3.joinable() << std::endl;
        co_yield std::ranges::elements_of(std::move(a3));
        std::cout << "a3.joinable() = " << a3.joinable() << std::endl;
        // co_yield std::ranges::elements_of(a3); // generator_promise_base_t::yield_value(): assert(generator_and_allocator.range.joinable());
        // co_yield std::ranges::elements_of(std::move(a3)); // generator_promise_base_t::yield_value(): assert(generator_and_allocator.range.joinable());
    }(std::allocator_arg, allocator);
    for (int &&e : generator_example_lifetime)
        std::cout << e << std::endl;
    std::cout << std::endl << std::endl;
}

template<ext::type_agnostic_allocator_c allocator_t>
void example_grafting(allocator_t &allocator)
{
    ext::generator_t<int> generator_example_grafting = [](std::allocator_arg_t, allocator_t const &allocator) -> ext::generator_t<int> {
        ext::generator_t<int> a1 = [](allocator_t const &allocator) -> ext::generator_t<int> {
            co_yield 0;
            co_yield std::ranges::elements_of([](std::allocator_arg_t, allocator_t const &allocator) -> ext::generator_t<int> {
                co_yield 1;
                for (int e : std::views::iota(10, 12))
                    co_yield std::move(e);
                co_yield 2;
            }(std::allocator_arg, allocator));
            co_yield 3;
        }(allocator);
        ext::generator_t<int> a2 = [](allocator_t const &allocator) -> ext::generator_t<int> {
            co_yield 4;
            co_yield std::ranges::elements_of([](std::allocator_arg_t, allocator_t const &allocator) -> ext::generator_t<int> {
                co_yield 5;
                co_yield std::ranges::elements_of(std::views::iota(20, 22));
                co_yield 6;
            }(std::allocator_arg, allocator));
            co_yield 7;
        }(allocator);
        ext::generator_t<int> a3 = [](allocator_t const &allocator) -> ext::generator_t<int> {
            co_yield 8;
            co_yield std::ranges::elements_of([](std::allocator_arg_t, allocator_t const &allocator) -> ext::generator_t<int> {
                co_yield 9;
                co_yield std::ranges::elements_of([](std::allocator_arg_t, allocator_t const &allocator) -> ext::generator_t<int> { co_yield 30, co_yield 31; }(std::allocator_arg, allocator));
                co_yield 10;
            }(std::allocator_arg, allocator));
            co_yield 11;
        }(allocator);

        for (int &&e : std::ranges::ref_view(a1) | std::views::take(3)) // 0 1 10
            co_yield std::move(e);
        std::cout << "a1.joinable() = " << a1.joinable() << std::endl;
        co_yield std::ranges::elements_of(std::move(a1)); // 11 2 3
        std::cout << "a1.joinable() = " << a1.joinable() << std::endl;

        for (int &&e : std::ranges::ref_view(a2) | std::views::take(3)) // 4 5 20
            co_yield std::move(e);
        std::cout << "a2.joinable() = " << a2.joinable() << std::endl;
        co_yield std::ranges::elements_of(std::move(a2)); // 21 6 7
        std::cout << "a2.joinable() = " << a2.joinable() << std::endl;

        for (int &&e : std::ranges::ref_view(a3) | std::views::take(3)) // 8 9 30
            co_yield std::move(e);
        std::cout << "a3.joinable() = " << a3.joinable() << std::endl;
        co_yield std::ranges::elements_of(std::move(a3)); // 31 10 11
        std::cout << "a3.joinable() = " << a3.joinable() << std::endl;
    }(std::allocator_arg, allocator);
    for (int &&e : generator_example_grafting)
        std::cout << e << std::endl;
    std::cout << std::endl << std::endl;
}

template<ext::type_agnostic_allocator_c allocator_t>
void example_allocator_value_category(allocator_t &allocator)
{
    ext::generator_t<int> generator_example_allocator_value_category = [](std::allocator_arg_t, allocator_t &allocator) -> ext::generator_t<int> {
        co_yield 0; // allocator_t = std::allocator<void>
        co_yield std::ranges::elements_of([](allocator_t) -> ext::generator_t<int> { co_return; }(allocator));
        co_yield std::ranges::elements_of([](allocator_t const &) -> ext::generator_t<int> { co_return; }(std::as_const(allocator)));
        co_yield std::ranges::elements_of([](allocator_t &) -> ext::generator_t<int> { co_return; }(allocator));
        co_yield std::ranges::elements_of([](allocator_t const &&) -> ext::generator_t<int> { co_return; }(static_cast<allocator_t const &&>(auto(allocator))));
        co_yield std::ranges::elements_of([](allocator_t &&) -> ext::generator_t<int> { co_return; }(auto(allocator)));
        co_yield 1; // allocator_t = std::allocator<void>
        co_yield std::ranges::elements_of([](auto) -> ext::generator_t<int> { co_return; }(allocator));
        co_yield std::ranges::elements_of([](auto &&) -> ext::generator_t<int> { co_return; }(std::as_const(allocator)));
        co_yield std::ranges::elements_of([](auto &&) -> ext::generator_t<int> { co_return; }(allocator));
        co_yield std::ranges::elements_of([](auto &&) -> ext::generator_t<int> { co_return; }(static_cast<allocator_t const &&>(auto(allocator))));
        co_yield std::ranges::elements_of([](auto &&) -> ext::generator_t<int> { co_return; }(auto(allocator)));

        co_yield 2; // allocator_t = boost::interprocess::allocator<...>
        co_yield std::ranges::elements_of([](std::allocator_arg_t, allocator_t) -> ext::generator_t<int> { co_return; }(std::allocator_arg, allocator)); // allocator_t_ = A&, allocator_cvref_t = A
        co_yield std::ranges::elements_of([](std::allocator_arg_t, allocator_t const &) -> ext::generator_t<int> { co_return; }(std::allocator_arg, std::as_const(allocator))); // allocator_t_ = A const&, allocator_cvref_t = A const&
        co_yield std::ranges::elements_of([](std::allocator_arg_t, allocator_t &) -> ext::generator_t<int> { co_return; }(std::allocator_arg, allocator)); // allocator_t_ = A&, allocator_cvref_t = A&
        co_yield std::ranges::elements_of([](std::allocator_arg_t, allocator_t const &&) -> ext::generator_t<int> { co_return; }(std::allocator_arg, static_cast<allocator_t const &&>(auto(allocator)))); // allocator_t_ = A const&, allocator_cvref_t = A const&&
        co_yield std::ranges::elements_of([](std::allocator_arg_t, allocator_t &&) -> ext::generator_t<int> { co_return; }(std::allocator_arg, auto(allocator))); // allocator_t_ = A&, allocator_cvref_t = A&&
        co_yield 3; // allocator_t = boost::interprocess::allocator<...>
        co_yield std::ranges::elements_of([](auto, auto) -> ext::generator_t<int> { co_return; }(std::allocator_arg, allocator)); // allocator_t_ = A&, allocator_cvref_t = A
        co_yield std::ranges::elements_of([](auto, auto &&) -> ext::generator_t<int> { co_return; }(std::allocator_arg, std::as_const(allocator))); // allocator_t_ = A const&, allocator_cvref_t = A const&
        co_yield std::ranges::elements_of([](auto, auto &&) -> ext::generator_t<int> { co_return; }(std::allocator_arg, allocator)); // allocator_t_ = A&, allocator_cvref_t = A&
        co_yield std::ranges::elements_of([](auto, auto &&) -> ext::generator_t<int> { co_return; }(std::allocator_arg, static_cast<allocator_t const &&>(auto(allocator)))); // allocator_t_ = A const&, allocator_cvref_t = A const&&
        co_yield std::ranges::elements_of([](auto, auto &&) -> ext::generator_t<int> { co_return; }(std::allocator_arg, auto(allocator))); // allocator_t_ = A&, allocator_cvref_t = A&&

        auto range = std::views::iota(0, 0);
        co_yield 4; // allocator_t = std::allocator<void>
        co_yield std::ranges::elements_of(range); // allocator_t_ = A&, allocator_cvref_t = A
        co_yield 5; // allocator_t = boost::interprocess::allocator<...>
        co_yield std::ranges::elements_of<decltype(range), allocator_t>(range, allocator); // allocator_t_ = A&, allocator_cvref_t = A
        co_yield std::ranges::elements_of<decltype(range), allocator_t const &>(range, std::as_const(allocator)); // allocator_t_ = A const&, allocator_cvref_t = A const&
        co_yield std::ranges::elements_of<decltype(range), allocator_t &>(range, allocator); // allocator_t_ = A&, allocator_cvref_t = A&
        co_yield std::ranges::elements_of<decltype(range), allocator_t const &&>(range, static_cast<allocator_t const &&>(auto(allocator))); // allocator_t_ = A const&, allocator_cvref_t = A const&&
        co_yield std::ranges::elements_of<decltype(range), allocator_t &&>(range, auto(allocator)); // allocator_t_ = A&, allocator_cvref_t = A&&
    }(std::allocator_arg, allocator);
    for (int &&e : generator_example_allocator_value_category)
        std::cout << e << std::endl;
    std::cout << std::endl << std::endl;
}

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/indexes/null_index.hpp>
#include <boost/interprocess/managed_external_buffer.hpp>
#include <memory>

int main()
{
    constexpr std::size_t buffer_size = 0b1uz << 16u;
    std::unique_ptr<char[]> buffer = std::make_unique_for_overwrite<char[]>(buffer_size);
    using managed_memory_segment_t = boost::interprocess::basic_managed_external_buffer<char, boost::interprocess::rbtree_best_fit<boost::interprocess::null_mutex_family, void *>, boost::interprocess::null_index>;
    managed_memory_segment_t managed_memory_segment(boost::interprocess::create_only, buffer.get(), buffer_size);
    using allocator_t = boost::interprocess::allocator<void, managed_memory_segment_t::segment_manager>;
    allocator_t allocator(managed_memory_segment.get_segment_manager());
    //using allocator_t = std::allocator<void>;
    //allocator_t allocator;

    example_iterator(allocator);
    managed_memory_segment.check_sanity() ? void() : throw std::logic_error("main, assert(managed_memory_segment.check_sanity())");
    managed_memory_segment.all_memory_deallocated() ? void() : throw std::logic_error("main, assert(managed_memory_segment.all_memory_deallocated())");

    example_lifetime(allocator);
    managed_memory_segment.check_sanity() ? void() : throw std::logic_error("main, assert(managed_memory_segment.check_sanity())");
    managed_memory_segment.all_memory_deallocated() ? void() : throw std::logic_error("main, assert(managed_memory_segment.all_memory_deallocated())");

    example_grafting(allocator);
    managed_memory_segment.check_sanity() ? void() : throw std::logic_error("main, assert(managed_memory_segment.check_sanity())");
    managed_memory_segment.all_memory_deallocated() ? void() : throw std::logic_error("main, assert(managed_memory_segment.all_memory_deallocated())");

    example_allocator_value_category(allocator);
    managed_memory_segment.check_sanity() ? void() : throw std::logic_error("main, assert(managed_memory_segment.check_sanity())");
    managed_memory_segment.all_memory_deallocated() ? void() : throw std::logic_error("main, assert(managed_memory_segment.all_memory_deallocated())");

    return 0;
}
```
Output:
```
0
10
10
11
12
1
14
2
a1.empty() = 0
15
a1.empty() = 1
3


0
1
10
2
a1.joinable() = 1
11
a1.joinable() = 0
3
4
20
5
a2.joinable() = 1
21
a2.joinable() = 0
6
7
30
8
a3.joinable() = 1
31
9
a3.joinable() = 1
a3.joinable() = 0


0
1
10
a1.joinable() = 1
11
2
3
a1.joinable() = 0
4
5
20
a2.joinable() = 1
21
6
7
a2.joinable() = 0
8
9
30
a3.joinable() = 1
31
10
11
a3.joinable() = 0


0
1
2
3
4
5


```
