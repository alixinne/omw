/**
 * @file   omw/pre.hpp
 * @brief  Predeclarations for omw
 * @author Vincent TAVERNIER <vincent.tavernier@inria.fr>
 * @date   2018
 */

#ifndef _OMW_PRE_HPP_
#define _OMW_PRE_HPP_

namespace omw
{
template <typename T> class basic_array;
template <typename T> class basic_matrix;
class wrapper_base;

#if OMW_MATHEMATICA
class mathematica;
#endif

#if OMW_OCTAVE
class octave;
#endif
}

#endif /* _OMW_PRE_HPP_ */
