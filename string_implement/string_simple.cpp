#include <cassert>   // assert
#include <cstddef>   // std::size_t
#include <utility>   // std::move/swap
#include <string.h>  // memcmp/memcpy/strlen

class String {
public:
    String() = default;
    String(const char* s) : size_(strlen(s)), capacity_(size_)
    {
        data_ = new char[size_ + 1];
        memcpy(data_, s, size_ + 1);
    }
    String(const String& s)
        : data_(new char[s.size_ + 1]), size_(s.size_), capacity_(size_)
    {
        memcpy(data_, s.data_, size_ + 1);
    }
    String(String&& s) noexcept
        : data_(s.data_), size_(s.size_), capacity_(s.capacity_)
    {
        s.reset();
    }

#if 1
    String& operator=(const String& rhs)
    {
        if (this != &rhs) {
            if (capacity_ < rhs.size_) {
                auto new_data = new char[rhs.size_ + 1];
                delete[] data_;
                data_ = new_data;
                capacity_ = rhs.size_;
            }
            memcpy(data_, rhs.data_, rhs.size_ + 1);
            size_ = rhs.size_;
        }
        return *this;
    }
    String& operator=(String&& rhs) noexcept
    {
        if (this != &rhs) {
            data_ = rhs.data_;
            size_ = rhs.size_;
            capacity_ = rhs.capacity_;
            rhs.reset();
        }
        return *this;
    }
#else
    String& operator=(String rhs) noexcept
    {
        rhs.swap(*this);
        return *this;
    }
#endif

    ~String() { delete[] data_; }

    void swap(String& rhs) noexcept
    {
        using std::swap;
        swap(data_, rhs.data_);
        swap(size_, rhs.size_);
        swap(capacity_, rhs.capacity_);
    }

    const char* c_str() const
    {
        return data_;
    }
    const char* data() const
    {
        return data_;
    }
    std::size_t size() const
    {
        return size_;
    }

    friend bool operator==(const String& lhs, const String& rhs)
    {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        return memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
    }
    friend bool operator!=(const String& lhs, const String& rhs)
    {
        return !operator==(lhs, rhs);
    }

private:
    void reset()
    {
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }

    char* data_{};
    std::size_t size_{};
    std::size_t capacity_{};
};

int main()
{
    String s1;
    assert(s1.size() == 0);
    String s2{"Hello"};
    assert(s2.size() == 5);
    String s3 = s2;
    assert(s3.size() == 5);
    assert(s3 == s2);
    s1 = std::move(s3);
    assert(s3.size() == 0);
    assert(s1.size() == 5);
    assert(s3 != s2);
    assert(s1 == s2);
}
