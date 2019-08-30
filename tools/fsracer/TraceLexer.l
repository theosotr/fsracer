%{
    #include <string>

    #include "TraceLexer.hpp"
    #include "TraceParser.hpp"

    #include "Debug.h"

    #define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng)

    #define TOKEN(t) fstrace::TraceParser::make_##t(yytext, location)
    #define SINGLE_TOKEN(t) fstrace::TraceParser::make_##t(location)
%}


%option debug
%option nodefault
%option noyywrap
%option c++
%option yyclass="fstrace::TraceLexer"


%{
    // Code run each time a pattern is matched.
    #define YY_USER_ACTION  location.columns (yyleng);
%}

%%

%{
    fstrace::location& location = *loc;
    location.step ();
%}


[ \t\r ]+                location.step();
\n+                      location.lines (yyleng); location.step();


"!PID"                   return TOKEN(PID);
"!Working Directory"     return TOKEN(CWD);

":"                      return SINGLE_TOKEN(COLON);
"!"                      return SINGLE_TOKEN(EXCLAMATION);

"Operation"              return TOKEN(OP);
"do"                     return TOKEN(DO);
"done"                   return TOKEN(DONE);
"consumed"               return TOKEN(CONSUMED);
"produced"               return TOKEN(PRODUCED);
"expunged"               return TOKEN(EXPUNGED);
"hpath"                  return TOKEN(HPATH);
"hpathsym"               return TOKEN(HPATHSYM);
"newFd"                  return TOKEN(NEWFD);
"delFd"                  return TOKEN(DELFD);
"rename"                 return TOKEN(RENAME);
"symlink"                return TOKEN(SYMLINK);
"AT_FDCWD"               return TOKEN(ATFDCWD);
"Begin"                  return TOKEN(BEGIN_BLOCK);
"End"                    return TOKEN(END_BLOCK);
"MAIN"                   return TOKEN(MAIN);
"newEvent"               return TOKEN(NEW_EVENT);
"link"                   return TOKEN(LINK);
"trigger"                return TOKEN(TRIGGER);
"submitOp"               return TOKEN(SUBMIT_OP);
"SYNC"                   return TOKEN(SYNC);
"ASYNC"                  return TOKEN(ASYNC);
"S"                      return TOKEN(S);
"M"                      return TOKEN(M);
"W"                      return TOKEN(W);
"EXTERNAL"               return TOKEN(EXTERNAL);

[a]?sync_[0-9]+          return TOKEN(OPID);
[0-9]+                   return TOKEN(NUMBER);
[a-zA-Z0-9_\/\-.]+       return TOKEN(IDENTIFIER);

.                        {
                           debug::err("TraceParser")
                             << "Unknown character [" << yytext << "]";
                           return TOKEN(ERROR);
                         }
            
<<EOF>>                  return SINGLE_TOKEN(END);
%%
