#include <sys/types.h>
#include <sys/wait.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "Config.hpp"
#include "Job.hpp"

Job::Job()
{
	init();
	m_blobBuffer.ptr = nullptr;
	m_blobBuffer.size = 0;
	m_blobBuffer.asize = 0;
}

Job::~Job()
{
	assert(m_readFd == -1);
	assert(m_writeFd == -1);
	git_buf_free(&m_blobBuffer);
}

bool Job::start(const Config &cfg, std::string &&filename, git_repository *repo, git_index *repoIndex)
{
	assert(m_state == State::Init);

	int pipeFd[2][2];
	if (pipe(pipeFd[0]) == -1 || pipe(pipeFd[1]) == -1) {
		perror("pipe");
		return false;
	}

	m_pid = fork();
	switch (m_pid) {
		case -1:
			perror("Fatal error on fork");
			close(pipeFd[0][0]);
			close(pipeFd[0][1]);
			close(pipeFd[1][0]);
			close(pipeFd[1][1]);
			return false;
		case 0:
			dup2(pipeFd[0][1], STDERR_FILENO);
			dup2(pipeFd[1][0], STDIN_FILENO);

			close(pipeFd[0][0]);
			close(pipeFd[0][1]);
			close(pipeFd[1][0]);
			close(pipeFd[1][1]);

			execl(cfg.lucyPath.c_str(), cfg.lucyPath.c_str(), (char *)NULL);
			fprintf(stderr, "Cannot exec %s: %s\n", cfg.lucyPath.c_str(), strerror(errno));
			exit(1);
			break;
		default: {
			m_readFd = pipeFd[0][0];
			m_writeFd = pipeFd[1][1];
			close(pipeFd[0][1]);
			close(pipeFd[1][0]);
			m_state = State::Working;

			m_filename = std::move(filename);
			auto entry = git_index_get_bypath(repoIndex, m_filename.c_str(), 0);

			assert(m_blob == nullptr);
			git_blob_lookup(&m_blob, repo, &entry->id);
			git_blob_filtered_content(&m_blobBuffer, m_blob, m_filename.c_str(), 1);
		}
	}

	return true;
}

void Job::read()
{
	assert(m_state == State::Working);

	enum { BufferSize = 64 << 10 };
	if (m_outputOffset + BufferSize > m_output.size())
		m_output.resize(m_output.size() + BufferSize);

	auto rv = ::read(m_readFd, &m_output[m_outputOffset], m_output.size() - m_outputOffset);
	if (rv == -1) {
		perror("read from pipe");
		exit(1);
	}

	if (rv == 0) {
		m_state = State::Finished;
		close(m_readFd);
		m_readFd = -1;
		m_output.resize(m_outputOffset);
		waitpid(m_pid, &m_exitStatus, 0);
	}

	m_outputOffset += rv;
}

void Job::write()
{
	assert(m_state == State::Working);

	auto rv = ::write(m_writeFd, m_blobBuffer.ptr + m_blobOffset, m_blobBuffer.size - m_blobOffset);
	if (rv == -1) {
		perror("write to pipe");
		exit(1);
	}

	m_blobOffset += rv;
	if (m_blobOffset == m_blobBuffer.size) {
		close(m_writeFd);
		m_writeFd = -1;
	}
}

void Job::reset()
{
	if (m_readFd != -1)
		close(m_readFd);

	if (m_writeFd != -1)
		close(m_writeFd);

	if (m_blob)
		git_blob_free(m_blob);

	init();
}

void Job::init()
{
	m_readFd = -1;
	m_writeFd = -1;

	m_outputOffset = 0;
	m_exitStatus = -1;
	m_pid = -1;
	m_state = State::Init;

	m_blob = nullptr;
	m_blobOffset = 0;
}
