CXX := em++
FILES := examples/sample.cpp # Change to renderer.cpp

GLM := D:/Dev/C++/LIB/glm

ImGUI := D:/Dev/C++/LIB/imgui
ImGUI_OBJ := $(or $(word 1,$(wildcard build/ImGUI/*.o)),imgui)

Mova := D:/Dev/C++/LIB/Mova/src
Mova_OBJ := $(or $(word 1,$(wildcard build/Mova/*.o)), mova)

Include := -I $(ImGUI) -I $(Mova) -I $(GLM)
Flags := -D IMGUI_IMPL_OPENGL_ES2
Linker := -s ASYNCIFY -s WASM=1 -s MAX_WEBGL_VERSION=2 -s ALLOW_MEMORY_GROWTH=1 --bind --use-preload-plugins
Settings := -o html/index.html --shell-file html/basic_template.html --preload-file test.png

html/index.html: $(ImGUI_OBJ) $(Mova_OBJ) $(FILES)
	@echo Building Application...
	@$(CXX) $(wildcard $(FILES) build/*/*.o) $(Flags) $(Linker) $(Include) $(Settings)

$(ImGUI_OBJ): $(ImGUI)/*.cpp $(ImGUI)/*.h $(ImGUI)/backends/imgui_impl_opengl3.cpp $(ImGUI)/backends/imgui_impl_opengl3.h
	@echo Building ImGUI...
	@mkdir build\ImGUI
	@for %%f in ($(wildcard $(ImGUI)/*.cpp $(ImGUI)/backends/imgui_impl_opengl3.cpp)) do @$(CXX) -c %%f $(Flags) $(Include) -o build/ImGUI/%%~nxf.o

$(Mova_OBJ): $(Mova)/*.cpp $(Mova)/*/*.cpp $(Mova)/*.h
	@echo Building Mova...
	@mkdir build\Mova
	@for %%f in ($(wildcard $(Mova)/*.cpp $(Mova)/*/*.cpp)) do @$(CXX) -c %%f $(Flags) $(Include) -o build/Mova/%%~nxf.o
