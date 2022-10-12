#include "vector.h"
#include "tests.h"


int main() {

    tests::UnitTests();            // общий UnitTest класса Vector<T>
    tests::Benchmark();            // сравнение со стандартным std::vector
}