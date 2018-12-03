#pragma once

#include <fstream>
#include <memory>
#include <vector>

#include "AST.hpp"
#include "Preprocessor.hpp"
#include "Scanner.hpp"

class Driver {
	friend class yy::Parser;
public:
	Driver();

	void addChunk(AST::Chunk *chunk);
	std::vector <std::unique_ptr <AST::Chunk> > & chunks();
	const std::vector <std::unique_ptr <AST::Chunk> > & chunks() const;

	int parse();

	yy::location location(const char *s);
	void nextLine();
	void step();

	void setInputFile(const char *filename);
	void setInputFile(const std::string &filename) { setInputFile(filename.c_str()); }

private:
	Preprocessor m_preprocessor;
	yy::Parser m_parser;
	Scanner m_scanner;
	std::istream m_inputStream;
	std::ifstream m_inputFile;

	std::vector <std::unique_ptr <AST::Chunk> > m_chunks;
	std::string m_filename;
	yy::position m_position;
};
