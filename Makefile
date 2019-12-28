file=hack
.PHONY: s
all: s

$(file).scr: $(file).cpp
	g++ -std=c++11 -O3 -Llib -Iinclude $(file).cpp -o $(file).scr -lscrnsave -lfreeglut -lgdi32 -lopengl32
	
s: $(file).scr
	$(file).scr /s

demo1: $(file).scr
	cp -f demo1.txt window.txt
	$(file).scr /s

demo2: $(file).scr
	cp -f demo2.txt window.txt
	$(file).scr /s

demo3: $(file).scr
	cp -f demo3.txt window.txt
	$(file).scr /s

debug:
	g++ -std=c++11 -O0 -g -Llib -Iinclude $(file).cpp -o $(file).scr -lscrnsave -lfreeglut -lgdi32 -lopengl32
	gdb $(file).scr

clean:
	rm *.scr
