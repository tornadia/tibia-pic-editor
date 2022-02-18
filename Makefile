all: picparser

picparser: picparser.cpp
	g++ picparser.cpp -o picparser -lpng

format:
	clang-format picparser.cpp -i

clean:
	@rm -f picparser

.PHONY:
	clean
	format
