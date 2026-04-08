//
// Created by David Yang on 2026-04-08.
//

#ifndef AION_SHORTVEC_HPP
#define AION_SHORTVEC_HPP

#include <utility>
#include <initializer_list>

namespace aion::ast {
    class ASTContext;

    /// ShortVec - A lightweight alternative to std::vector, allocating
    /// within context boundaries and optimized for fewer allocations/elements.
    ///
    /// @tparam EleT - Element type stored in the vector
    /// @tparam Context - Context type for allocation and management
    template <typename EleT, typename Context = ASTContext> // element type
    class ShortVec {
        EleT* data_;
        std::size_t size_;
        std::size_t capacity_;
        Context& context_;

    public:
        using value_type = EleT;
        using context_type = Context;
        using size_type = std::size_t;
        using reference = EleT&;
        using const_reference = const EleT&;
        using pointer = EleT*;
        using const_pointer = const EleT*;
        using iterator = pointer;
        using const_iterator = const_pointer;

        explicit ShortVec(Context& context) : data_(nullptr), size_(0), capacity_(0), context_(context) {}

        ShortVec(Context& c, std::initializer_list<EleT> elements) : ShortVec(c) {
            size_ = elements.size();
            capacity_ = si::next_power_of_two(size_);
            data_ = static_cast<EleT*>(context_.allocate(capacity_ * sizeof(EleT), alignof(EleT)));
            std::copy(elements.begin(), elements.end(), data_);
        }

        ShortVec(const ShortVec& other) : ShortVec(other.context_) {
            size_ = other.size_;
            capacity_ = other.capacity_;
            data_ = new EleT[capacity_];
            std::copy(other.data_, other.data_ + size_, data_);
        }

        ShortVec(ShortVec&& other) noexcept
        : ShortVec(other.context_) {
            size_ = other.size_;
            capacity_ = other.capacity_;
            data_ = other.data_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }

        ~ShortVec() = delete;

        ShortVec& operator=(const ShortVec& other) {
            if (this == &other) {
                return *this;
            }
            this->data_ = other.data_;
            this->size_ = other.size_;
            this->capacity_ = other.capacity_;
            return *this;
        }
        ShortVec& operator=(ShortVec&& other) noexcept {
            if (this == &other) {
                return *this;
            }
            this->data_ = other.data_;
            this->size_ = other.size_;
            this->capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
            return *this;
        }

        void reserve(size_type new_cap);
        void push_back(const EleT& element);

        template <typename... Args>
        requires std::is_constructible_v<EleT, Args...>
        void emplace_back(Args... args);

        reference operator[](size_type index);
        reference at(size_type index);

        pointer begin() noexcept { return data_; }
        pointer end() noexcept { return data_ + size_; }

        [[nodiscard]] size_type size() const { return size_; }
        [[nodiscard]] size_type capacity() const { return capacity_; }

    };

    template<typename EleT, typename Context>
    void ShortVec<EleT, Context>::reserve(size_type new_cap) {
        if (new_cap <= capacity_) {
             return;
        }
        auto p2_new_cap = si::next_power_of_two(new_cap);
        auto p2_new_data = static_cast<EleT*>(context_.allocate(p2_new_cap * sizeof(EleT), alignof(EleT)));
        std::copy(data_, data_ + size_, p2_new_data);
        data_ = p2_new_data;
        capacity_ = p2_new_cap;

        // no deallocation – context is assumed to be an area allocator
    }

    template <typename EleT, typename Context>
    void ShortVec<EleT, Context>::push_back(const EleT& element) {
        if (size_ == capacity_) {
            reserve(capacity_ * 2);
        }
        data_[size_++] = element;
    }

    template<typename EleT, typename Context>
    template<typename... Args> requires std::is_constructible_v<EleT, Args...>
    void ShortVec<EleT, Context>::emplace_back(Args... args) {
        if (size_ == capacity_) {
            reserve(capacity_ * 2);
        }
        new (data_ + size_) EleT(std::forward<Args>(args)...);
        size_++;
    }

    template<typename EleT, typename Context>
    typename ShortVec<EleT, Context>::reference ShortVec<EleT, Context>::at(size_type index) {
        if (index >= size_) {
            return end();
        }
        return data_[index];
    }

    template<typename EleT, typename Context>
    typename ShortVec<EleT, Context>::reference ShortVec<EleT, Context>::operator[](size_type index) {
        return at(index);
    }

    // ShortVec(ASTContext& context) -> ShortVec<int>;
}

#endif //AION_SHORTVEC_HPP
