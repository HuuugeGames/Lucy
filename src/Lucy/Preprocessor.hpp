#pragma once

#include <iosfwd>
#include <string>

#include "position.hh"

class Preprocessor : public std::streambuf {
public:
	Preprocessor() = default;
	Preprocessor(const Preprocessor &) = delete;
	Preprocessor operator = (const Preprocessor &) = delete;

	const std::string & data() const { return m_data; }
	bool preprocess();
	void setInputFile(const std::string &filename, std::istream *input);
	void setInputStream(std::istream *input);

private:
	int underflow();

	std::istream *m_input = &std::cin;
	std::string m_filename;
	std::string m_data;
	yy::position m_position;
};
