%{
    #include <string>

    #include "driver.hpp"
    #include "scanner.hpp"
    #include "parser.hpp"

    #include "Debug.h"

    #define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng)

    #define TOKEN(t) fstrace::parser::make_##t(yytext, location)
    #define SINGLE_TOKEN(t) fstrace::parser::make_##t(location)
%}


%option debug
%option nodefault
%option noyywrap
%option c++
%option yyclass="fstrace::Scanner"


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

":"                      return SINGLE_TOKEN(COLON);

"!PID"                   return TOKEN(PID);
"!Working Directory"     return TOKEN(CWD);
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
"submitOp"               return TOKEN(SUBMIT_OP);
"SYNC"                   return TOKEN(SYNC);
"ASYNC"                  return TOKEN(ASYNC);
"S"                      return TOKEN(S);
"M"                      return TOKEN(M);
"W"                      return TOKEN(W);
"EXTERNAL"               return TOKEN(EXTERNAL);

[a]?sync_[0-9]+          return TOKEN(OPID);
[0-9]+                   return TOKEN(ID);
[a-zA-Z0-9_\/\-.]+       return TOKEN(PATH);
[!][a-zA-Z_]+            return TOKEN(META);

.                        { 
                           debug::err("Parser")
                             << "unknown character [" << yytext << "]\n";
                           return SINGLE_TOKEN(END);
                         }
            
<<EOF>>                  return SINGLE_TOKEN(END);
%%
