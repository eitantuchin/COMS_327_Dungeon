#include <iostream>
#include <string>

// Base class A
class A {
public:
    // Constructor with multiple arguments
    A(int a, int b, const std::string &desc)
        : x(a), y(b), description(desc)
    {
        std::cout << "A's constructor called with: x=" << x
                  << ", y=" << y
                  << ", description=\"" << description << "\"\n";
    }

    // A simple method to demonstrate inheritance
    void method1() {
        std::cout << "A::method1() called.\n"
                  << "  - x = " << x << "\n"
                  << "  - y = " << y << "\n"
                  << "  - description = " << description << "\n";
    }

private:
    int x;
    int y;
    std::string description;
};

// Derived class B inherits publicly from A
class B : public A {
public:
    // B's constructor forwards arguments to A's constructor
    B(int x, int y, const std::string &desc)
        : A(x, y, desc)  // Initialize the base A part with x, y, desc
    {
        std::cout << "B's constructor called.\n";
    }

    // No method1() here, so B inherits method1() from A
};

int main() {
    // Create an instance of B, passing multiple arguments
    B b(10, 20, "Hello Inheritance");

    // Even though B doesn't define method1(), it inherits it from A
    b.method1();

    return 0;
}

