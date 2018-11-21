#pragma once

#include <git2.h>
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
	const std::string & filename() const { return m_filename; }
	const std::string & output() const { return m_output; }
	pid_t pid() const { return m_pid; }
	State state() const { return m_state; }

	int readFd() const { return m_readFd; }
	int writeFd() const { return m_writeFd; }

	bool shouldWrite() const { return m_blobOffset != m_blobBuffer.size; }
	void read();
	void write();

	void reset();
	bool start(const Config &cfg, std::string &&filename, git_repository *repo, git_index *repoIndex);

private:
	void init();

	std::string m_filename;

	std::string m_output;
	size_t m_outputOffset;

	git_blob *m_blob;
	git_buf m_blobBuffer;
	size_t m_blobOffset;

	int m_readFd, m_writeFd;
	int m_exitStatus;
	pid_t m_pid;
	State m_state;
};
