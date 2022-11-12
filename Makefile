# Makefile is outdated, use OreBuild
CXX := g++ # em++
FILES := examples/renderer.cpp # Change to sample.cpp

GLM := D:/Dev/C++/LIB/glm

Mova := D:/Dev/C++/LIB/Mova/src
Mova_OBJ := $(or $(word 1,$(wildcard build/Mova/*.o)), mova)

Include := -I $(Mova) -I $(GLM)
Flags := -Wall # -D IMGUI_IMPL_OPENGL_ES2
# Linker := -s ASYNCIFY -s WASM=1 -s MAX_WEBGL_VERSION=2 -s ALLOW_MEMORY_GROWTH=1 --bind --use-preload-plugins
Settings := -o test.exe # -o html/index.html --shell-file html/basic_template.html --preload-file test.png

html/index.html: $(ImGUI_OBJ) $(Mova_OBJ) $(FILES)
	@echo Building Application...
	@$(CXX) $(wildcard $(FILES) build/*/*.o) $(Flags) $(Linker) $(Include) $(Settings)

$(Mova_OBJ): $(Mova)/*.cpp $(Mova)/*/*.cpp $(Mova)/*.h
	@echo Building Mova...
	@if not exist build\Mova @mkdir build\Mova
	@for %%f in ($(wildcard $(Mova)/*.cpp $(Mova)/*/*.cpp)) do @$(CXX) -c %%f $(Flags) $(Include) -o build/Mova/%%~nxf.o
