add_library(ImGui STATIC
	imgui.cpp
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_impl_opengl3.cpp
	imgui_impl_sdl.cpp
	imgui_widgets.cpp
)

target_compile_definitions(ImGui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
target_include_directories(ImGui PUBLIC ${OpenGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
