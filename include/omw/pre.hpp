/**
 * @file   omw/pre.hpp
 * @brief  Predeclarations for omw
 * @author Vincent TAVERNIER <vince.tavernier@gmail.com>
 * @date   2018
 */

#ifndef _OMW_PRE_HPP_
#define _OMW_PRE_HPP_

namespace omw
{
template <typename T> class basic_array;
template <typename T> class basic_matrix;
template <typename wrapper_impl> class wrapper_base;

#if OMW_MATHEMATICA
class mathematica;
#endif

#if OMW_OCTAVE
class octave;
#endif
}

#endif /* _OMW_PRE_HPP_ */
