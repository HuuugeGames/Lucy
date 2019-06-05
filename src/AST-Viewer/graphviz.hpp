#pragma once

#include <string>
#include <vector>

#include "location.hh"

struct Rect {
	float x, y, width, height;
};

struct Node {
	Rect r;
	std::string label;
	yy::location codeLocation;
};

struct Edge {
	unsigned u, v;
};

namespace AST {
	class Chunk;
};

class DotGraph {
public:
	DotGraph() = default;
	DotGraph(const DotGraph &other) = delete;
	DotGraph(DotGraph &&other) = default;
	DotGraph & operator = (const DotGraph &other) = delete;
	DotGraph & operator = (DotGraph &&other) = default;
	~DotGraph() = default;

	const std::vector <Edge> & edges() const { return m_edges; }
	const std::vector <Node> & nodes() const { return m_nodes; }
	void prepare(const AST::Chunk &root);

private:
	std::vector <Node> m_nodes;
	std::vector <Edge> m_edges;
};
