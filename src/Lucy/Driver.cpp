#include <cstring>

#include "AST.hpp"
#include "Driver.hpp"
#include "Logger.hpp"

Driver::Driver()
	: m_parser{*this}, m_scanner{*this},
	  m_inputStream{&m_preprocessor}, m_errorStream{&std::cerr},
	  m_filename{"<stdin>"}, m_position{&m_filename, 1, 1}
{
}

void Driver::addChunk(AST::Chunk *chunk)
{
	m_chunks.emplace_back(chunk);
}

std::vector <std::unique_ptr <AST::Chunk> > & Driver::chunks()
{
	return m_chunks;
}

const std::vector <std::unique_ptr <AST::Chunk> > & Driver::chunks() const
{
	return m_chunks;
}

int Driver::parse()
{
	m_scanner.switch_streams(&m_inputStream);
	if (m_parser.parse() != 0)
		return 1;

	size_t idx = 0;
	while (idx < m_chunks.size()) {
		if (m_chunks[idx].get() == nullptr) {
			m_chunks[idx] = std::move(m_chunks.back());
			m_chunks.pop_back();
		} else {
			++idx;
		}
	}

	if (m_chunks.empty())
		return 2;
	return 0;
}

yy::location Driver::location(const char *s)
{
	yy::position end = m_position + strlen(s);
	auto result = yy::location{m_position, end};
	m_position = end;
	return result;
}

void Driver::nextLine()
{
	m_position.lines(1);
}

void Driver::step()
{
	m_position += 1;
}

void Driver::setInputFile(const char *filename)
{
	m_filename = filename;
	m_inputFile.open(m_filename);
	if (m_inputFile.fail())
		FATAL("Unable to open file for reading: " << m_filename.c_str() << '\n');

	m_position.initialize(&m_filename);
	m_preprocessor.setInputFile(m_filename, &m_inputFile);
}

void Driver::setInputStream(std::istream *input)
{
	m_filename.clear();
	m_position.initialize();
	m_preprocessor.setInputStream(input);
}

void Driver::logError(const std::string &msg)
{
	if (!m_errorStream)
		return;
	*m_errorStream << msg;
}
