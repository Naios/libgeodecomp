/**
 * Copyright 2012-2013 Andreas Schäfer
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>
#include <libflatarray/flat_array.hpp>

class Cell
{
public:
    /**
     * This friend declaration is required to give utility classes,
     * e.g. operator<<(CELL_TYPE, LibFlatArray::soa_accessor), access
     * to our private members.
     */
    LIBFLATARRAY_ACCESS

    inline
    Cell(const double temp = 0) :
        temp(temp)
    {}

    // fixme: write something sensible here
    template<typename NEIGHBORHOOD>
    void update(const NEIGHBORHOOD& hood)
    {
        temp = 123;
    }

private:
    double temp;
};

LIBFLATARRAY_REGISTER_SOA(Cell, ((double)(temp)))

int main(int argc, char **argv)
{
    std::cout << "go\n";

    std::cout << "boomer " << LibFlatArray::detail::flat_array::offset<Cell, 1>::OFFSET << "\n";

    return 0;
}
