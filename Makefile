CXX := em++

ImGUI := D:/Dev/C++/LIB/imgui
ImGUI_OBJ := $(or $(word 1,$(wildcard build/ImGUI/*.o)),imgui)

Mova := D:/Dev/C++/LIB/Mova/src
Mova_OBJ := $(or $(word 1,$(wildcard build/Mova/*.o)), mova)

Include := -I $(ImGUI) -I $(Mova)
Flags := -D IMGUI_IMPL_OPENGL_ES2
Linker := -s ASYNCIFY -s WASM=1 -s MAX_WEBGL_VERSION=2 -s ALLOW_MEMORY_GROWTH=1 --bind --use-preload-plugins
Settings := -o html/index.html --shell-file html/basic_template.html

html/index.html: $(ImGUI_OBJ) $(Mova_OBJ) test/main.cpp
	@echo Building Application...
	@$(CXX) $(wildcard test/main.cpp build/*/*.o) $(Flags) $(Linker) $(Include) $(Settings)

$(ImGUI_OBJ): $(ImGUI)/*.cpp $(ImGUI)/*.h $(ImGUI)/backends/imgui_impl_opengl3.cpp $(ImGUI)/backends/imgui_impl_opengl3.h
	@echo Building ImGUI...
	@for %%f in ($(wildcard $(ImGUI)/*.cpp $(ImGUI)/backends/imgui_impl_opengl3.cpp)) do @$(CXX) -c %%f $(Flags) $(Include) -o build/ImGUI/%%~nxf.o

$(Mova_OBJ): $(Mova)/*.cpp $(Mova)/*.h
	@echo Building Mova...
	@for %%f in ($(wildcard $(Mova)/*.cpp)) do @$(CXX) -c %%f $(Flags) $(Include) -o build/Mova/%%~nxf.o
