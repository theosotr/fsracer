#ifndef TOKENS_H
#define TOKENS_H

#include <string>

#if ! defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif


#undef YY_DECL
#define YY_DECL fstrace::TraceParser::symbol_type fstrace::TraceLexer::get_next_token()

#include "TraceParser.hpp" // this is needed for symbol_type

namespace fstrace {

class TraceLexer : public yyFlexLexer {
public:
  TraceLexer(std::istream *in) : yyFlexLexer(in) {
    loc = new fstrace::TraceParser::location_type();
  }
  ~TraceLexer() {
    if (loc) {
      delete loc;
    }
  }

  virtual fstrace::TraceParser::symbol_type get_next_token();

private:
  fstrace::TraceParser::location_type *loc = nullptr;

};


} 


#endif
