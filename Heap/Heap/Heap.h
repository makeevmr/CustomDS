#ifndef KTH_HEAP
#define KTH_HEAP

#include "../Compare/Compare.h"
#include <stdexcept>
#include <type_traits>

template <typename T, typename Compare>
class Heap {
private:
    using value_type = T;
    using size_type = std::size_t;
    using value_compare = Compare;
    using reference = T &;
    using const_reference = const T &;

    value_type *data;
    size_type size;
    size_type capacity;
    value_compare comparator;

    void swap(Heap<T, Compare> &other) noexcept;

    static void free(value_type *data_to_free, size_type destructor_calls) noexcept;

    static void uninitializedCopy(value_type *copy_to, const Heap<T, Compare> &copy_from);

    static void uninitializedCopy(value_type *copy_to, const value_type *copy_from, size_type size);

    [[nodiscard]] size_type getLeft(size_type index) const noexcept;

    [[nodiscard]] size_type getRight(size_type index) const noexcept;

    [[nodiscard]] size_type getParent(size_type index) const noexcept;

    void resize();

    void siftingDown(size_type index) noexcept(std::is_nothrow_swappable_v<T>);

    void siftingUp(size_type index) noexcept(std::is_nothrow_swappable_v<T>);

    void makeHeap() noexcept(std::is_nothrow_swappable_v<T>);

public:
    Heap();

    explicit Heap(size_type capacity);

    Heap(value_type *construct_from_data, size_type size);

    Heap(const Heap<T, Compare> &other);

    Heap<T, Compare> &operator=(const Heap<T, Compare> &other);

    Heap(Heap &&other) noexcept;

    Heap<T, Compare> &operator=(Heap<T, Compare> &&other) noexcept;

    ~Heap();

    // Element access
    [[nodiscard]] const_reference top() const;

    // Capacity
    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] size_type getSize() const noexcept;

    // Modifiers
    void push(const value_type &value);

    void push(const value_type &&value);

    void pop();
};
#endif // KTH_HEAP
