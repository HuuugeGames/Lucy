#pragma once

#include <string>

struct Config;

class Job {
public:
	enum class State {
		Init,
		Working,
		Finished,
	};

	Job();
	~Job();

	int exitStatus() { return m_exitStatus; }
	int fd() const { return m_pipeFd; }
	const std::string & filename() const { return m_filename; }
	const std::string & output() const { return m_output; }
	pid_t pid() const { return m_pid; }
	State state() const { return m_state; }

	void process();
	void reset();
	bool start(const Config &cfg, std::string &&filename);

private:
	std::string m_filename;
	std::string m_output;
	size_t m_outputOffset;
	int m_pipeFd;
	int m_exitStatus;
	pid_t m_pid;
	State m_state;
};
