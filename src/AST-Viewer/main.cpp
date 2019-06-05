#include <GL/glew.h>
#include <SDL.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include "Lucy/Driver.hpp"
#include "graphviz.hpp"

std::ostream & operator << (std::ostream &os, const ImVec2 &vec);

std::vector <std::string_view> split(const std::string &s)
{
	std::vector <std::string_view> result;

	std::string::size_type cur = 0, prev = 0;
	do {
		cur = s.find('\n', cur);

		if (cur != std::string::npos) {
			result.emplace_back(&s[prev], cur - prev);
			++cur;
			prev = cur;
		} else {
			result.emplace_back(&s[prev]);
		}
	} while (cur != std::string::npos);

	return result;
}

struct ASTGenState {
	ASTGenState() = default;
	ASTGenState(const ASTGenState &) = delete;
	ASTGenState(ASTGenState &&) = default;
	ASTGenState & operator = (const ASTGenState &) = delete;
	ASTGenState & operator = (ASTGenState &&) = default;
	~ASTGenState() = default;

	std::string luaCode;
	std::vector <std::string_view> lines;
	std::string errorLog;
	DotGraph graph;
	std::unique_ptr <AST::Chunk> astRoot;
	std::string windowId;
	bool show = true;
};

int sourceEditCallback(ImGuiInputTextCallbackData *data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
		std::string *buff = static_cast<std::string *>(data->UserData);
		buff->resize(data->BufTextLen);
		data->Buf = buff->data();
	}

	return 0;
}

ASTGenState generateAST(const std::string &luaCode)
{
	std::istringstream ss{luaCode};
	Driver d;
	d.setInputStream(&ss);

	std::ostringstream errorStream;
	d.setErrorStream(&errorStream);

	ASTGenState result;
	result.luaCode = luaCode;
	if (result.luaCode.back() == '\n')
		result.luaCode.pop_back();

	if (d.parse() != 0) {
		result.errorLog = errorStream.str();
	} else {
		result.astRoot = std::move(d.chunks()[0]);
		result.graph.prepare(*result.astRoot);

		std::ostringstream ss;
		ss << "View##" << result.astRoot.get();
		result.windowId = ss.str();
		result.lines = split(result.luaCode);
	}

	return result;
}

void renderCode(const std::vector <std::string_view> &code, yy::location highlight)
{
	auto *drawList = ImGui::GetWindowDrawList();
	const auto textColor = ImGui::GetColorU32(ImGuiCol_Text);

	if (highlight.begin == highlight.end) {
		for (const auto &s : code) {
			drawList->AddText(ImGui::GetCursorScreenPos(), textColor, s.begin(), s.end());
			ImGui::NewLine();
		}
		return;
	}

	//prefer 0-based indexing
	--highlight.begin.column;
	--highlight.begin.line;
	--highlight.end.column;
	--highlight.end.line;

	struct TextChunk {
		TextChunk(const std::string_view &text, ImVec2 pos, bool highlight)
			: text{text}, pos{pos}, size{ImGui::CalcTextSize(text.begin(), text.end())}, highlight{highlight}
		{
		}

		TextChunk(const char *begin, const char *end, ImVec2 pos, bool highlight)
			: text{begin, static_cast<std::string_view::size_type>(end - begin)},
			  pos{pos}, size{ImGui::CalcTextSize(text.begin(), text.end())}, highlight{highlight}
		{
		}

		std::string_view text;
		ImVec2 pos, size;
		bool highlight;
	};

	std::vector <TextChunk> chunks;

	unsigned line = 0;
	while (line != highlight.begin.line) {
		chunks.emplace_back(code[line], ImGui::GetCursorScreenPos(), false);
		ImGui::NewLine();
		++line;
	}

	auto cursor = ImGui::GetCursorScreenPos();
	if (highlight.begin.column != 0) {
		const char *textBegin = code[line].begin();
		const char *textEnd = textBegin + highlight.begin.column;

		chunks.emplace_back(textBegin, textEnd, cursor, false);
		cursor.x += ImGui::CalcTextSize(textBegin, textEnd).x;
	}

	{
		const char *textBegin = code[line].begin() + highlight.begin.column;
		const char *textEnd;
		if (highlight.begin.line == highlight.end.line)
			textEnd = code[line].begin() + highlight.end.column;
		else
			textEnd = code[line].end();

		chunks.emplace_back(textBegin, textEnd, cursor, true);
		cursor.x += ImGui::CalcTextSize(textBegin, textEnd).x;
	}

	if (highlight.begin.line == highlight.end.line && highlight.end.column != code[line].size()) {
		const char *textBegin = code[line].begin() + highlight.end.column;
		const char *textEnd = code[line].end();

		chunks.emplace_back(textBegin, textEnd, cursor, false);
	}
	ImGui::NewLine();
	++line;

	while (line < highlight.end.line) {
		const char *textBegin = code[line].begin();
		const char *textEnd = code[line].end();

		chunks.emplace_back(textBegin, textEnd, ImGui::GetCursorScreenPos(), true);
		ImGui::NewLine();
		++line;
	}

	if (line == highlight.end.line) {
		const char *textBegin = code[line].begin();
		const char *textEnd = code[line].begin() + highlight.end.column;

		chunks.emplace_back(textBegin, textEnd, ImGui::GetCursorScreenPos(), true);
		cursor = ImGui::GetCursorScreenPos();

		if (highlight.end.column != code[line].size()) {
			cursor.x += ImGui::CalcTextSize(textBegin, textEnd).x;

			textBegin = textEnd;
			textEnd = code[line].end();
			chunks.emplace_back(textBegin, textEnd, cursor, false);
		}

		ImGui::NewLine();
		++line;
	}

	while (line < code.size()) {
		chunks.emplace_back(code[line], ImGui::GetCursorScreenPos(), false);
		ImGui::NewLine();
		++line;
	}

	const auto bgColor = ImGui::GetColorU32(ImGuiCol_TextSelectedBg);
	for (const auto &chunk : chunks) {
		if (chunk.highlight)
			drawList->AddRectFilled(chunk.pos, ImVec2{chunk.pos.x + chunk.size.x, chunk.pos.y + chunk.size.y}, bgColor);
	}

	for (const auto &chunk : chunks)
		drawList->AddText(chunk.pos, textColor, chunk.text.begin(), chunk.text.end());
}

std::ostream & operator << (std::ostream &os, const ImVec2 &vec)
{
	os << '[' << vec.x << ", " << vec.y << ']';
	return os;
}

int main()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		fprintf(stderr, "Error: %s\n", SDL_GetError());
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_Window *window = SDL_CreateWindow("AST Viewer",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_GLContext glCtx = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(window, glCtx);
	ImGui_ImplOpenGL3_Init("#version 130");
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	bool done = false;
	std::string buffer = "a = b + c\nx = 2 + 3 * 4";

	std::vector <ASTGenState> astViews;
	std::string astError;

	while (!done) {
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		{
			ImGui::Begin("AST Viewer");

			ImGui::Text("Code:");
			ImGui::InputTextMultiline("##source", buffer.data(), buffer.size() + 1,
				ImVec2{-1.0f, ImGui::GetTextLineHeight() * 16},
				ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize,
				sourceEditCallback, &buffer);

			if (!astError.empty())
				ImGui::Text(astError.data());

			if (ImGui::Button("Generate AST")) {
				auto ast = generateAST(buffer);
				astError.clear();
				if (!ast.errorLog.empty()) {
					astError = std::move(ast.errorLog);
				} else if (!ast.astRoot->isEmpty()) {
					astViews.emplace_back(std::move(ast));
					astViews.back().lines = split(astViews.back().luaCode);
				}
			}

			unsigned idx = 0;
			while (idx < astViews.size()) {
				auto &ast = astViews[idx];
				if (ast.show) {
					static yy::location codeHighlight;

					ImGui::Begin(ast.windowId.c_str(), &ast.show);
					renderCode(ast.lines, codeHighlight);
					const auto pos = ImGui::GetCursorPos();

					auto *drawList = ImGui::GetWindowDrawList();
					drawList->ChannelsSplit(2);

					const auto &nodes = ast.graph.nodes();
					std::vector <ImVec2> nodeCenter;

					drawList->ChannelsSetCurrent(1);
					codeHighlight.initialize();
					for (const auto &n : nodes) {
						ImGui::SetCursorPos(ImVec2{n.r.x, pos.y + n.r.y});
						ImGui::Button(n.label.c_str());
						if (ImGui::IsItemHovered())
							codeHighlight = n.codeLocation;

						auto topLeft = ImGui::GetItemRectMin();
						auto bottomRight = ImGui::GetItemRectMax();
						nodeCenter.push_back(ImVec2{(topLeft.x + bottomRight.x) * 0.5f, (topLeft.y + bottomRight.y) * 0.5f});
					}

					drawList->ChannelsSetCurrent(0);
					for (const auto &e : ast.graph.edges())
						drawList->AddLine(nodeCenter[e.u], nodeCenter[e.v], 0xFFFFFFFF, 1);

					drawList->ChannelsMerge();
					ImGui::End();

					++idx;
				} else {
					astViews[idx] = std::move(astViews.back());
					astViews.pop_back();
				}
			}

			ImGui::End();
		}

		ImGui::Render();
		SDL_GL_MakeCurrent(window, glCtx);
		glViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(glCtx);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
