BUILD_DIR=build2
# BUILD_DIR must be empty, or cmake may loop indefinitely
mkdir $BUILD_DIR
cd $BUILD_DIR
# we must include at least one MC backend in LLVM_TARGETS_TO_BUILD, even if only
# the CppBackend gets used
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles" .. -DLLVM_TARGETS_TO_BUILD="CppBackend;Cpu0"
