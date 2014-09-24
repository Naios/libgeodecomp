/**
 * Copyright 2014 Andreas Schäfer
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef FLAT_ARRAY_SOA_ACCESSOR_HPP
#define FLAT_ARRAY_SOA_ACCESSOR_HPP

namespace LibFlatArray {

/**
 * This class provides an object-oriented view to a "Struct of
 * Arrays"-style grid. It requires the user to register the type CELL
 * using the macro LIBFLATARRAY_REGISTER_SOA.
 *
 * All registed members will be avalable by functions of the same
 * name, so if "Cell" had two members "float a" and "char b", then
 * these would be accessible via soa_accessor<Cell, ...>::a() and
 * soa_accessor<Cell, ...>::b().
 *
 * soa_accessor<> also provides an operator[] which can be used to
 * access neighboring cells.
 */
template<typename CELL, long DIM_X, long DIM_Y, long DIM_Z, long INDEX>
class soa_accessor;

template<typename CELL, long DIM_X, long DIM_Y, long DIM_Z, long INDEX>
class const_soa_accessor;

template<typename CELL, long DIM_X, long DIM_Y, long DIM_Z, long INDEX>
class soa_accessor_light;

template<typename CELL, long DIM_X, long DIM_Y, long DIM_Z, long INDEX>
class const_soa_accessor_light;

}

#endif

