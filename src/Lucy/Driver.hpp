#pragma once

#include <fstream>
#include <memory>
#include <vector>

#include "AST_fwd.hpp"
#include "Preprocessor.hpp"
#include "Scanner.hpp"

class Driver {
	friend class yy::Parser;
public:
	Driver();

	void addChunk(std::unique_ptr <AST::Chunk> &&chunk);
	std::vector <std::unique_ptr <AST::Chunk> > & chunks();
	const std::vector <std::unique_ptr <AST::Chunk> > & chunks() const;

	int parse();

	yy::location location() const { return yy::location{m_position, m_position}; }
	yy::location location(const char *s);
	yy::position position() const { return m_position; }

	void logError(const std::string &msg);
	void nextLine();
	void step();

	void setErrorStream(std::ostream *os) { m_errorStream = os; }
	void setInputFile(const char *filename);
	void setInputFile(const std::string &filename) { setInputFile(filename.c_str()); }
	void setInputStream(std::istream *input);

private:

	Preprocessor m_preprocessor;
	yy::Parser m_parser;
	Scanner m_scanner;
	std::istream m_inputStream;
	std::ifstream m_inputFile;
	std::ostream *m_errorStream;

	std::vector <std::unique_ptr <AST::Chunk> > m_chunks;
	std::string m_filename;
	yy::position m_position;
};
