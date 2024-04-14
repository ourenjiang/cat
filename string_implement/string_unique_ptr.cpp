#include <cassert>   // assert
#include <cstddef>   // std::size_t
#include <memory>    // std::unique_ptr/make_unique
#include <utility>   // std::move/swap
#include <string.h>  // memcmp/memcpy/strlen

class String {
public:
    String() = default;
    String(const char* s) : size_(strlen(s)), capacity_(size_)
    {
        data_ = std::make_unique<char[]>(size_ + 1);
        memcpy(data_.get(), s, size_ + 1);
    }
    String(const String& s) : size_(s.size()), capacity_(size_)
    {
        data_ = std::make_unique<char[]>(size_ + 1);
        memcpy(data_.get(), s.data(), size_ + 1);
    }
    String(String&& s) = default;
    String& operator=(const String& rhs)
    {
        if (this != &rhs) {
            if (capacity_ < rhs.size_) {
                auto new_data = std::make_unique<char[]>(rhs.size_ + 1);
                data_ = std::move(new_data);
                capacity_ = rhs.size_;
            }
            memcpy(data_.get(), rhs.data(), rhs.size_ + 1);
            size_ = rhs.size_;
        }
        return *this;
    }
    String& operator=(String&& rhs) = default;
    ~String() = default;

    void swap(String& rhs) noexcept
    {
        using std::swap;
        swap(data_, rhs.data_);
        swap(size_, rhs.size_);
        swap(capacity_, rhs.capacity_);
    }

    const char* c_str() const
    {
        return data_.get();
    }
    const char* data() const
    {
        return data_.get();
    }
    std::size_t size() const
    {
        return data_ ? size_ : 0;
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
    std::unique_ptr<char[]> data_{};
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
