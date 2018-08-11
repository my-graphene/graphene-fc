#pragma once
/**
 * @file fc/reflect.hpp
 *
 * @brief Defines types and macros used to provide reflection.
 *
 */

#include <fc/utility.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <stdint.h>
#include <string.h>

#include <fc/reflect/typename.hpp>

namespace fc {

/**
 *  @brief defines visit functions for T
 *  Unless this is specialized, visit() will not be defined for T.
 *
 *  @tparam T - the type that will be visited.
 *
 *  The @ref FC_REFLECT(TYPE,MEMBERS) or FC_STATIC_REFLECT_DERIVED(TYPE,BASES,MEMBERS) macro is used to specialize this
 *  class for your type.
 */
template<typename T>
struct reflector{
    typedef T type;
    typedef fc::false_type is_defined;
    typedef fc::false_type is_enum;

    /**
     *  @tparam Visitor a function object of the form:
     *
     *    @code
     *     struct functor {
     *        template<typename Member, class Class, Member (Class::*member)>
     *        void operator()( const char* name )const;
     *     };
     *    @endcode
     *
     *  If T is an enum then the functor has the following form:
     *    @code
     *     struct functor {
     *        template<int Value>
     *        void operator()( const char* name )const;
     *     };
     *    @endcode
     *
     *  @param v a functor that will be called for each member on T
     *
     *  @note - this method is not defined for non-reflected types.
     */
    #ifdef DOXYGEN
    template<typename Visitor>
    static inline void visit( const Visitor& v );
    #endif // DOXYGEN
};

void throw_bad_enum_cast( int64_t i, const char* e );
void throw_bad_enum_cast( const char* k, const char* e );
} // namespace fc


#ifndef DOXYGEN

#define FC_REFLECT_VISIT_BASE(r, visitor, base) \
  fc::reflector<base>::visit( visitor );


#ifndef _MSC_VER
  #define TEMPLATE template
#else
  // Disable warning C4482: nonstandard extention used: enum 'enum_type::enum_value' used in qualified name
  #pragma warning( disable: 4482 )
  #define TEMPLATE
#endif

#define FC_REFLECT_VISIT_MEMBER( r, visitor, elem ) \
{ typedef decltype(((type*)nullptr)->elem) member_type;  \
  visitor.TEMPLATE operator()<member_type,type,&type::elem>( BOOST_PP_STRINGIZE(elem) ); \
}

#define FC_REFLECT_VISIT_MEMBER_I( r, visitor, I, elem ) \
   case I: FC_REFLECT_VISIT_MEMBER( r, visitor, elem ) break;


#define FC_REFLECT_BASE_MEMBER_COUNT( r, OP, elem ) \
  OP fc::reflector<elem>::total_member_count

#define FC_REFLECT_MEMBER_COUNT( r, OP, elem ) \
  OP 1

#define FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
template<typename Visitor>\
static inline void visit( const Visitor& v ) { \
    BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_MEMBER, v, MEMBERS ) \
}\
template<typename Visitor, typename IndexType>\
static inline void visit_local_member( const Visitor& v, IndexType index ) { \
   switch( index ) {\
      BOOST_PP_SEQ_FOR_EACH_I( FC_REFLECT_VISIT_MEMBER_I, v, MEMBERS ) \
      default: break;\
   }\
}

#define FC_REFLECT_DERIVED_IMPL_EXT( TYPE, INHERITS, MEMBERS ) \
template<typename Visitor>\
void fc::reflector<TYPE>::visit( const Visitor& v ) { \
    BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_MEMBER, v, MEMBERS ) \
}

#endif // DOXYGEN


#define FC_REFLECT_VISIT_ENUM( r, enum_type, elem ) \
  v.operator()(BOOST_PP_STRINGIZE(elem), int64_t(enum_type::elem) );
#define FC_REFLECT_ENUM_TO_STRING( r, enum_type, elem ) \
   case enum_type::elem: return BOOST_PP_STRINGIZE(elem);
#define FC_REFLECT_ENUM_TO_FC_STRING( r, enum_type, elem ) \
   case enum_type::elem: return fc::string(BOOST_PP_STRINGIZE(elem));

#define FC_REFLECT_ENUM_FROM_STRING( r, enum_type, elem ) \
  if( strcmp( s, BOOST_PP_STRINGIZE(elem)  ) == 0 ) return enum_type::elem;
#define FC_REFLECT_ENUM_FROM_STRING_CASE( r, enum_type, elem ) \
   case enum_type::elem:

#define FC_REFLECT_ENUM( ENUM, FIELDS ) \
namespace fc { \
template<> struct reflector<ENUM> { \
    typedef fc::true_type is_defined; \
    typedef fc::true_type is_enum; \
    static const char* to_string(ENUM elem) { \
      switch( elem ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_TO_STRING, ENUM, FIELDS ) \
        default: \
           fc::throw_bad_enum_cast( fc::to_string(int64_t(elem)).c_str(), BOOST_PP_STRINGIZE(ENUM) ); \
      }\
      return nullptr; \
    } \
    static const char* to_string(int64_t i) { \
      return to_string(ENUM(i)); \
    } \
    static fc::string to_fc_string(ENUM elem) { \
      switch( elem ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_TO_FC_STRING, ENUM, FIELDS ) \
      } \
      return fc::to_string(int64_t(elem)); \
    } \
    static fc::string to_fc_string(int64_t i) { \
      return to_fc_string(ENUM(i)); \
    } \
    static ENUM from_int(int64_t i) { \
      ENUM e = ENUM(i); \
      switch( e ) \
      { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_FROM_STRING_CASE, ENUM, FIELDS ) \
          break; \
        default: \
          fc::throw_bad_enum_cast( i, BOOST_PP_STRINGIZE(ENUM) ); \
      } \
      return e;\
    } \
    static ENUM from_string( const char* s ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_FROM_STRING, ENUM, FIELDS ) \
        int64_t i = 0; \
        try \
        { \
           i = boost::lexical_cast<int64_t>(s); \
        } \
        catch( const boost::bad_lexical_cast& e ) \
        { \
           fc::throw_bad_enum_cast( s, BOOST_PP_STRINGIZE(ENUM) ); \
        } \
        return from_int(i); \
    } \
    template< typename Visitor > \
    static void visit( Visitor& v ) \
    { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_ENUM, ENUM, FIELDS ) \
    } \
};  \
template<> struct get_typename<ENUM>  { static const char* name()  { return BOOST_PP_STRINGIZE(ENUM);  } }; \
}

/*  Note: FC_REFLECT_ENUM previously defined this function, but I don't think it ever
 *        did what we expected it to do.  I've disabled it for now.
 *
 *  template<typename Visitor> \
 *  static inline void visit( const Visitor& v ) { \
 *      BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_ENUM, ENUM, FIELDS ) \
 *  }\
 */

/**
 *  @def FC_REFLECT_DERIVED(TYPE,INHERITS,MEMBERS)
 *
 *  @brief Specializes fc::reflector for TYPE where
 *         type inherits other reflected classes
 *
 *  @param INHERITS - a sequence of base class names (basea)(baseb)(basec)
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define FC_REFLECT_DERIVED( TYPE, INHERITS, MEMBERS ) \
namespace fc {  \
  template<> struct get_typename<TYPE>  { static const char* name()  { return BOOST_PP_STRINGIZE(TYPE);  } }; \
template<> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef fc::true_type  is_defined; \
    typedef fc::false_type is_enum; \
    enum  member_count_enum {  \
      local_member_count = 0  BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_MEMBER_COUNT, +, MEMBERS ),\
      total_member_count = local_member_count BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_BASE_MEMBER_COUNT, +, INHERITS )\
    }; \
    FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; }
#define FC_REFLECT_DERIVED_TEMPLATE( TEMPLATE_ARGS, TYPE, INHERITS, MEMBERS ) \
namespace fc {  \
  template<BOOST_PP_SEQ_ENUM(TEMPLATE_ARGS)> struct get_typename<TYPE>  { static const char* name()  { return BOOST_PP_STRINGIZE(TYPE);  } }; \
template<BOOST_PP_SEQ_ENUM(TEMPLATE_ARGS)> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef fc::true_type  is_defined; \
    typedef fc::false_type is_enum; \
    enum  member_count_enum {  \
      local_member_count = 0  BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_MEMBER_COUNT, +, MEMBERS ),\
      total_member_count = local_member_count BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_BASE_MEMBER_COUNT, +, INHERITS )\
    }; \
    FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; }

//BOOST_PP_SEQ_SIZE(MEMBERS),

/**
 *  @def FC_REFLECT(TYPE,MEMBERS)
 *  @brief Specializes fc::reflector for TYPE
 *
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 *
 *  @see FC_REFLECT_DERIVED
 */
#define FC_REFLECT( TYPE, MEMBERS ) \
    FC_REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

#define FC_REFLECT_TEMPLATE( TEMPLATE_ARGS, TYPE, MEMBERS ) \
    FC_REFLECT_DERIVED_TEMPLATE( TEMPLATE_ARGS, TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

#define FC_REFLECT_EMPTY( TYPE ) \
    FC_REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, BOOST_PP_SEQ_NIL )

#define FC_REFLECT_TYPENAME( TYPE ) \
namespace fc { \
  template<> struct get_typename<TYPE>  { static const char* name()  { return BOOST_PP_STRINGIZE(TYPE);  } }; \
}

#define FC_REFLECT_FWD( TYPE ) \
namespace fc { \
  template<> struct get_typename<TYPE>  { static const char* name()  { return BOOST_PP_STRINGIZE(TYPE);  } }; \
template<> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef fc::true_type is_defined; \
    enum  member_count_enum {  \
      local_member_count = BOOST_PP_SEQ_SIZE(MEMBERS), \
      total_member_count = local_member_count BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_BASE_MEMBER_COUNT, +, INHERITS )\
    }; \
    template<typename Visitor> static void visit( const Visitor& v ); \
}; }


#define FC_REFLECT_DERIVED_IMPL( TYPE, MEMBERS ) \
    FC_REFLECT_IMPL_DERIVED_EXT( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

#define FC_REFLECT_IMPL( TYPE, MEMBERS ) \
    FC_REFLECT_DERIVED_IMPL_EXT( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )



