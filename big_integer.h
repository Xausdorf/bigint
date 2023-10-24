#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <limits>
#include <string>
#include <vector>

struct big_integer {
public:
    big_integer();
    big_integer(const big_integer& other);
    big_integer(int a);
    big_integer(unsigned int a);
    big_integer(long a);
    big_integer(unsigned long a);
    big_integer(long long a);
    big_integer(unsigned long long a);
    explicit big_integer(const std::string& str);
    ~big_integer();

    big_integer& operator=(const big_integer& other);

    big_integer& operator+=(const big_integer& rhs);
    big_integer& operator-=(const big_integer& rhs);
    big_integer& operator*=(const big_integer& rhs);
    big_integer& operator/=(const big_integer& rhs);
    big_integer& operator%=(const big_integer& rhs);

    big_integer& operator&=(const big_integer& rhs);
    big_integer& operator|=(const big_integer& rhs);
    big_integer& operator^=(const big_integer& rhs);

    big_integer& operator<<=(int rhs);
    big_integer& operator>>=(int rhs);

    big_integer operator+() const;
    big_integer operator-() const;
    big_integer operator~() const;

    big_integer& operator++();
    big_integer operator++(int);

    big_integer& operator--();
    big_integer operator--(int);

    friend bool operator==(const big_integer& a, const big_integer& b);
    friend bool operator!=(const big_integer& a, const big_integer& b);
    friend bool operator<(const big_integer& a, const big_integer& b);
    friend bool operator>(const big_integer& a, const big_integer& b);
    friend bool operator<=(const big_integer& a, const big_integer& b);
    friend bool operator>=(const big_integer& a, const big_integer& b);
    friend bool operator==(const big_integer& a, const int& b);

    friend std::string to_string(const big_integer& a);

private:
    static const uint32_t STRING_RADIX = 1'000'000'000;
    static const uint32_t CHAR_RADIX = 10;
    static const uint64_t RADIX = 1ull << std::numeric_limits<uint32_t>::digits;

    std::vector<uint32_t> value;
    bool is_negative = false;

    void skip_leading_zeros();

    std::pair<big_integer, big_integer> division(const big_integer& a, const big_integer& b);

    void commutative_bitwise_operation(const big_integer& rhs,
                                       const std::function<uint32_t(uint32_t a, uint32_t b)> binary_function);

    big_integer& mul_to_short(uint32_t rhs);
    big_integer& add_to_short(uint32_t rhs);
    big_integer& sub_to_short(uint32_t rhs);
    big_integer& div_to_short(uint32_t rhs);
    big_integer& mod_to_short(uint32_t rhs);

    void swap(big_integer& other);

    static big_integer mul_to_short(const big_integer& a, uint32_t rhs);
};

big_integer operator+(const big_integer& a, const big_integer& b);
big_integer operator-(const big_integer& a, const big_integer& b);
big_integer operator*(const big_integer& a, const big_integer& b);
big_integer operator/(const big_integer& a, const big_integer& b);
big_integer operator%(const big_integer& a, const big_integer& b);

big_integer operator&(const big_integer& a, const big_integer& b);
big_integer operator|(const big_integer& a, const big_integer& b);
big_integer operator^(const big_integer& a, const big_integer& b);

big_integer operator<<(const big_integer& a, int b);
big_integer operator>>(const big_integer& a, int b);

bool operator==(const big_integer& a, const big_integer& b);
bool operator!=(const big_integer& a, const big_integer& b);
bool operator<(const big_integer& a, const big_integer& b);
bool operator>(const big_integer& a, const big_integer& b);
bool operator<=(const big_integer& a, const big_integer& b);
bool operator>=(const big_integer& a, const big_integer& b);
bool operator==(const big_integer& a, const int& b);

std::string to_string(const big_integer& a);
std::ostream& operator<<(std::ostream& out, const big_integer& a);