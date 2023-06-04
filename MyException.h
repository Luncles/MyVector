#include <exception>
using namespace std;

class RangeException : public exception
{
public:
    const char* what() const noexcept override
    {
        return "running past the end of an array";
    }
};