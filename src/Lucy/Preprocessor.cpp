#include <iostream>
#include <iterator>

#include "Logger.hpp"
#include "Preprocessor.hpp"

bool Preprocessor::preprocess()
{
	std::istreambuf_iterator<char> p{m_input->rdbuf()};
	const std::istreambuf_iterator<char> EOFIter;

	if (p == EOFIter)
		return false;

	std::string result;
	char stringDelim = 0;

	auto step = [this, &p]
	{
		m_position += 1;
		return *p++;
	};

	auto nextLine = [this, &p]
	{
		m_position.lines(1);
		++p;
	};

	auto longCommentCheck = [this, &p, &EOFIter, &result, &step]
	{
		if (p == EOFIter || *p != '[')
			return -1;

		result.push_back(' ');
		step();

		if (p == EOFIter || (*p != '=' && *p != '['))
			return -1;

		int depth = 0;
		while (p != EOFIter && *p == '=') {
			result.push_back(' ');
			step();
			++depth;
		}

		if (p == EOFIter || *p != '[')
			return -1;

		result.push_back(' ');
		step();
		return depth;
	};

	while (p != EOFIter) {
		if (stringDelim != 0) {
			if (*p == '\\') {
				result.push_back(step());
				if (p != EOFIter)
					result.push_back(step());
				continue;
			}

			result.push_back(*p);
			if (*p == stringDelim)
				stringDelim = 0;
			step();
		} else if (*p == '-' && !result.empty() && result.back() == '-') {
			result.pop_back();
			result.append(2, ' ');
			step();

			const yy::position commentStart = m_position - 2;
			if (const int depth = longCommentCheck(); depth >= 0) {
				//long comment
				bool possibleMatch = false;
				bool endComment = false;
				int depthLeft;

				while (p != EOFIter && !endComment) {
					switch (*p) {
						case '=':
							if (possibleMatch)
								--depthLeft;
							break;
						case ']':
							if (!possibleMatch || depthLeft != 0) {
								possibleMatch = true;
								depthLeft = depth;
							} else if (depthLeft == 0) {
								endComment = true;
							}
							break;
						default:
							possibleMatch = false;
					}

					if (*p == '\n') {
						result.push_back('\n');
						nextLine();
					} else {
						result.push_back(' ');
						step();
					}
				}

				if (!endComment)
					FATAL("Error parsing long comment (EOF reached) started at: " << commentStart);

			} else {
				//short comment
				while (p != EOFIter && step() != '\n')
					result.push_back(' ');
				result.push_back('\n');
				m_position.lines(1);
			}
		} else {
			if (*p == '\'' || *p == '"')
				stringDelim = *p;
			result.push_back(step());
		}
	}

	m_data = std::move(result);

	return true;
}

void Preprocessor::setInputFile(const std::string &filename, std::istream *input)
{
	m_filename = filename;
	m_position.initialize(&m_filename);
	m_input = input;
}

int Preprocessor::underflow()
{
	if (!preprocess())
		return traits_type::eof();

	setg(m_data.data(), m_data.data(), m_data.data() + m_data.size());
	return traits_type::to_int_type(m_data[0]);
}
