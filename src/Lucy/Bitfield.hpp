#pragma once

#include <array>

template <unsigned Size, typename Base = uint8_t>
class Bitfield {
public:
	unsigned get(unsigned idx) const { return (m_bits[index(idx)] >> bit(idx)) & 1; }
	void set(unsigned idx) { m_bits[index(idx)] |= (1 << bit(idx)); }
	void unset(unsigned idx) { m_bits[index(idx)] &= ~(1 << bit(idx)); }

private:
	constexpr unsigned bit(unsigned idx) const { return idx % sizeof(Base); }
	constexpr unsigned index(unsigned idx) const { return idx / sizeof(Base); }

	std::array <Base, (Size + sizeof(Base) - 1) / sizeof(Base)> m_bits;
};
