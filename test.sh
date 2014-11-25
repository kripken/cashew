~/Dev/fastcomp/mergebuild/Release+Asserts/bin/clang++ -std=c++11 istring.cpp parser.cpp simple_ast.cpp test.cpp -g
./a.out &> o
diff -U5 o text.txt

