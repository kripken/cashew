/media/d69dd9b2-4791-4b8e-aeb3-f6353b45d710/home/alon/Dev/fastcomp/mergebuild/Release+Asserts/bin/clang++ -std=c++11 istring.cpp parser.cpp simple_ast.cpp test.cpp -g
./a.out &> o
diff -U5 o text.txt

