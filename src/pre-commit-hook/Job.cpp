#include <sys/types.h>
#include <sys/wait.h>
#include <cassert>
#include <cstdio>
#include <unistd.h>

#include "Job.hpp"

Job::Job()
{
	reset();
}

Job::~Job()
{
	assert(m_pipeFd == -1);
}

bool Job::start(std::string &&filename)
{
	assert(m_state == State::Init);
	m_filename = std::move(filename);

	int pipeFd[2];
	if (pipe(pipeFd) == -1) {
		perror("pipe");
		return false;
	}

	m_pid = fork();
	switch (m_pid) {
		case -1:
			perror("Fatal error on fork");
			close(pipeFd[0]);
			close(pipeFd[1]);
			return false;
		case 0:
			dup2(pipeFd[1], STDERR_FILENO);
			close(pipeFd[0]);
			close(pipeFd[1]);

			//TODO hardcoded path
			execl("./Lucy", "./Lucy", m_filename.c_str(), (char *)NULL);
			perror("exec");
			exit(1);
			break;
		default: {
			close(pipeFd[1]);
			m_pipeFd = pipeFd[0];
			m_state = State::Working;
		}
	}

	return true;
}

void Job::process()
{
	assert(m_state == State::Working);

	enum { BufferSize = 64 << 10 };
	if (m_outputOffset + BufferSize > m_output.size())
		m_output.resize(m_output.size() + BufferSize);

	auto rv = read(m_pipeFd, &m_output[m_outputOffset], m_output.size() - m_outputOffset);
	if (rv == -1) {
		perror("read from pipe");
		exit(1);
	}

	if (rv == 0) {
		m_state = State::Finished;
		close(m_pipeFd);
		m_pipeFd = -1;
		m_output.resize(m_outputOffset);
		waitpid(m_pid, &m_exitStatus, 0);
	}

	m_outputOffset += rv;
}

void Job::reset()
{
	m_outputOffset = 0;
	if (m_pipeFd != -1)
		close(m_pipeFd);
	m_pipeFd = -1;
	m_exitStatus = -1;
	m_pid = -1;
	m_state = State::Init;
}
