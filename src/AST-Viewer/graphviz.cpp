#include <array>
#include <cgraph.h>
#include <cstdio>
#include <gvc.h>
#include <sstream>

#include "graphviz.hpp"

#include "Lucy/AST.hpp"

namespace {

class GraphContext {
public:
	GraphContext(const AST::Chunk &root);
	GraphContext(const GraphContext &) = delete;
	GraphContext(GraphContext &&) = delete;
	GraphContext & operator = (const GraphContext &) = delete;
	GraphContext & operator = (GraphContext &&) = delete;
	~GraphContext();

	Agraph_t * graph() const { return m_graph; }
	void reload(std::vector <Node> &nodes, std::vector <Edge> &edges, const char *data);

private:
	struct GraphNode {
		GraphNode(const char *label, const yy::location &loc, Agnode_t *node)
			: label{label}, loc{loc}, node{node} {}

		std::string label;
		yy::location loc;
		Agnode_t *node = nullptr;
	};

	unsigned createNode(const char *label, const yy::location &loc)
	{
		const unsigned nodeIdx = m_graphNodes.size();
		char buff[25];
		sprintf(buff, "%u", nodeIdx);

		Agnode_t *node = agnode(m_graph, buff, 1);
		m_graphNodes.emplace_back(label, loc, node);
		agset(node, const_cast<char *>("label"), m_graphNodes.back().label.data());

		agset(node, const_cast<char *>("comment"), buff);
		return nodeIdx;
	}

	void pushNode(const char *label, const yy::location &loc)
	{
		const unsigned parentIdx = m_nodeStack.back(), curIdx = createNode(label, loc);

		m_nodeStack.push_back(curIdx);
		m_edges.push_back(Edge{parentIdx, curIdx});
		agedge(m_graph, m_graphNodes[parentIdx].node, m_graphNodes[curIdx].node, nullptr, 1);
	}

	void popNode()
	{
		m_nodeStack.pop_back();
	}

	void walk(const AST::Assignment &assignment);
	void walk(const AST::BinOp &binOp);
	void walk(const AST::ExprList &exprList);
	void walk(const AST::LValue &lval);
	void walk(const AST::NestedExpr &expr);
	void walk(const AST::Node &node);
	void walk(const AST::UnOp &unOp);
	void walk(const AST::VarList &varList);

	Agraph_t *m_graph = nullptr;
	std::vector <GraphNode> m_graphNodes;
	std::vector <Edge> m_edges;
	std::vector <unsigned> m_nodeStack;
};

GraphContext::GraphContext(const AST::Chunk &root)
{
	m_graph = agopen(const_cast<char *>("AST"), Agstrictdirected, nullptr);

	agattr(m_graph, AGNODE, const_cast<char *>("comment"), const_cast<char *>(""));

	m_nodeStack.push_back(createNode("Chunk", root.location()));
	for (const auto &n : root.children())
		walk(*n);
}

GraphContext::~GraphContext()
{
	agclose(m_graph);
}

void GraphContext::reload(std::vector <Node> &nodes, std::vector <Edge> &edges, const char *data)
{
	std::unique_ptr <Agraph_t, decltype(&agclose)> graph{agmemread(data), &agclose};
	assert(graph);
	nodes.resize(agnnodes(graph.get()));
	Agnode_t *node = agfstnode(graph.get());

	std::string tmp;
	Rect boundingBox;

	std::istringstream is{agget(graph.get(), const_cast<char *>("bb"))};
	is >> boundingBox.x;
	is.ignore();
	is >> boundingBox.y;
	is.ignore();
	is >> boundingBox.width;
	is.ignore();
	is >> boundingBox.height;

	boundingBox.width -= boundingBox.x;
	boundingBox.height -= boundingBox.y;

	while (node) {
		unsigned nodeIdx = atol(agget(node, const_cast<char *>("comment")));
		Node &rn = nodes[nodeIdx];
		rn.codeLocation = m_graphNodes[nodeIdx].loc;
		rn.label = agget(node, const_cast<char *>("label"));
		rn.r.width = atof(agget(node, const_cast<char *>("width")));
		rn.r.height = atof(agget(node, const_cast<char *>("height")));

		is.clear();
		is.str(agget(node, const_cast<char *>("pos")));
		is >> rn.r.x;
		is.ignore();
		is >> rn.r.y;
		rn.r.y = boundingBox.height - rn.r.y;

		node = agnxtnode(graph.get(), node);
	}

	edges = std::move(m_edges);
}

void GraphContext::walk(const AST::Assignment &assignment)
{
	pushNode("=", assignment.location());
	walk(assignment.varList());
	walk(assignment.exprList());
	popNode();
}

void GraphContext::walk(const AST::BinOp &binOp)
{
	pushNode(binOp.toString(), binOp.location());
	walk(binOp.left());
	walk(binOp.right());
	popNode();
}

void GraphContext::walk(const AST::ExprList &exprList)
{
	pushNode("ExprList", exprList.location());
	for (const auto &e : exprList.exprs())
		walk(*e);
	popNode();
}

void GraphContext::walk(const AST::LValue &lval)
{
	pushNode(lval.resolveName().c_str(), lval.location());
	popNode();
}

void GraphContext::walk(const AST::NestedExpr &expr)
{
	pushNode("NestedExpr", expr.location());
	walk(expr.expr());
	popNode();
}

void GraphContext::walk(const AST::Node &node)
{
	#define SUBCASE(type) case AST::Node::Type::type: walk(static_cast<const AST::type &>(node)); break

	switch (node.type().value()) {
		SUBCASE(Assignment);
		SUBCASE(BinOp);
		SUBCASE(LValue);
		SUBCASE(NestedExpr);
		SUBCASE(UnOp);
		default:
			pushNode(node.type(), node.location());
			popNode();
			break;
	}

	#undef SUBCASE
}

void GraphContext::walk(const AST::UnOp &unOp)
{
	pushNode(unOp.toString(), unOp.location());
	walk(unOp.operand());
	popNode();
}

void GraphContext::walk(const AST::VarList &varList)
{
	pushNode("VarList", varList.location());
	for (const auto &v : varList.vars())
		walk(*v);
	popNode();
}

} //namespace

void DotGraph::prepare(const AST::Chunk &root)
{
	static GVC_t *graphvizCtx = gvContext();

	GraphContext ctx(root);

	/*
	 * Graphviz does not generate position information in the "layout"
	 * stage, but in the "render" stage.
	 */
	gvLayout(graphvizCtx, ctx.graph(), "dot");

	char *data;
	unsigned length;
	gvRenderData(graphvizCtx, ctx.graph(), "dot", &data, &length);
	ctx.reload(m_nodes, m_edges, data);
	gvFreeRenderData(data);
	gvFreeLayout(graphvizCtx, ctx.graph());
}
