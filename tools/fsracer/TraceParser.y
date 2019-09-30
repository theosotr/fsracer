%skeleton "lalr1.cc" /* -*- C++ -*- */
%defines
%define api.namespace {fstrace}
%define parser_class_name {TraceParser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
    #include <iostream>
    #include <string>
    #include <sstream>
    #include <vector>

    #include "Operation.h"
    #include "Trace.h"
    #include "Utils.h"

    namespace fstrace {
        class TraceGeneratorDriver;
        class TraceLexer;
    }
}

%lex-param { fstrace::TraceLexer &lexer  }
%parse-param { fstrace::TraceLexer &lexer  }
%parse-param { fstrace::TraceGeneratorDriver &driver  }
%locations
%define parse.trace
%define parse.error verbose

%code top {
    #include "TraceGeneratorDriver.hpp"
    #include "TraceLexer.hpp"

    static fstrace::TraceParser::symbol_type yylex(fstrace::TraceLexer &lexer) {
      return lexer.get_next_token();           
    }
}


%define api.token.prefix {TOK_}

%token
  END 0        "end of file"
  COLON        ":"
  EXCLAMATION  "!"
;

%token <std::string>
  PID           "!PID"
  CWD           "!Working Directory"
  OP            "Operation"
  DO            "Do"
  DONE          "Done"
  HPATH         "hpath"
  HPATHSYM      "hpathsym"
  NEWFD         "newFd"
  DELFD         "delFd"
  RENAME        "rename"
  SYMLINK       "symlink"
  ATFDCWD       "AT_FDCWD"
  BEGIN_BLOCK   "Begin"
  MAIN          "Main"
  END_BLOCK     "End"
  NEW_EVENT     "newEvent"
  LINK          "link"
  TRIGGER       "trigger"
  SUBMIT_OP     "submitOp"
  SYNC          "SYNC"
  ASYNC         "ASYNC"
  S M W EXTERNAL
  PRODUCED      "produced"
  CONSUMED      "consumed"
  EXPUNGED      "expunged"
;
%token <std::string>
  NUMBER     "number"
  OPID       "operation id"
  IDENTIFIER "identifier"
  ERROR      "character"

%type <int> dirfd
%type <operation::Hpath::EffectType> effect_type
%type <trace::Event> event_type
%type <std::vector<std::string>> meta_vars

%start program

%%

program : stats PID COLON NUMBER CWD COLON IDENTIFIER op_defs block_defs {
          driver.trace_f->SetThreadId(std::stoi($4));
          driver.trace_f->SetCwd($7);
        }
        ;


op_defs : op_def {  }
        | op_defs op_def {  }
        ;


op_def : OP OPID DO operations DONE {
         trace::ExecOp *exec_op = new trace::ExecOp($2);
         for (auto const &op_entry : driver.opers) {
           exec_op->AddOperation(op_entry);
         }
         driver.trace_f->AddExecOp(exec_op);
         driver.opers.clear();
       }
       | OP OPID DO DONE {
         driver.trace_f->AddExecOp(new trace::ExecOp($2));
         driver.opers.clear();
       }
       ;


operations : oper {  }
           | operations oper { }
           ;


oper : HPATH dirfd IDENTIFIER effect_type meta_vars {
       operation::Hpath *hpath = new operation::Hpath($2, $3, $4);
       AddOperationDebugInfo(hpath, $5);
       driver.opers.push_back(hpath);
     }
     | HPATHSYM dirfd IDENTIFIER effect_type meta_vars {
       operation::HpathSym *hpathsym = new operation::HpathSym($2, $3, $4);
       AddOperationDebugInfo(hpathsym, $5);
       driver.opers.push_back(hpathsym);
     }
     | NEWFD dirfd IDENTIFIER NUMBER meta_vars {
       operation::NewFd *new_fd = new operation::NewFd(
           $2, $3, std::stoi($4));
       AddOperationDebugInfo(new_fd, $5);
       driver.opers.push_back(new_fd);
     }
     | NEWFD dirfd IDENTIFIER meta_vars {
       operation::NewFd *new_fd = new operation::NewFd($2, $3, -1);
       new_fd->MarkFailed();
       AddOperationDebugInfo(new_fd, $4);
       driver.opers.push_back(new_fd);
     }
     | DELFD NUMBER meta_vars {
       operation::DelFd *del_fd = new operation::DelFd(std::stoi($2));
       AddOperationDebugInfo(del_fd, $3);
       driver.opers.push_back(del_fd);
     }
     | LINK dirfd IDENTIFIER dirfd IDENTIFIER meta_vars {
       operation::Link *link = new operation::Link($2, $3, $4, $5);
       AddOperationDebugInfo(link, $6);
       driver.opers.push_back(link);
     }
     | RENAME dirfd IDENTIFIER dirfd IDENTIFIER meta_vars {
       operation::Rename *rename = new operation::Rename($2, $3, $4, $5);
       AddOperationDebugInfo(rename, $6);
       driver.opers.push_back(rename);
     }
     | SYMLINK dirfd IDENTIFIER IDENTIFIER meta_vars {
       operation::Symlink *symlink = new operation::Symlink($2, $3, $4);
       AddOperationDebugInfo(symlink, $5);
       driver.opers.push_back(symlink);
     }
     ;


dirfd : ATFDCWD { $$ = AT_FDCWD; }
      | NUMBER { $$ = std::stoi($1); }
      ;


effect_type : CONSUMED { $$ = operation::Hpath::CONSUMED; }
            | PRODUCED { $$ = operation::Hpath::PRODUCED; }
            | EXPUNGED { $$ = operation::Hpath::EXPUNGED; }
            ;


block_defs : block_def { }
           | block_defs block_def { }
           ;


block_def : BEGIN_BLOCK MAIN exprs END_BLOCK {
            trace::Block *block = new trace::Block(MAIN_BLOCK);
            for (auto const &expr_entry : driver.exprs) {
              block->AddExpr(expr_entry);
            }
            driver.trace_f->AddBlock(block);
            driver.exprs.clear();
          }
          | BEGIN_BLOCK NUMBER exprs END_BLOCK {
            trace::Block *block = new trace::Block(std::stoi($2));
            for (auto const &expr_entry : driver.exprs) {
              block->AddExpr(expr_entry);
            }
            driver.trace_f->AddBlock(block);
            driver.exprs.clear();
          }
          | BEGIN_BLOCK NUMBER END_BLOCK {
            driver.trace_f->AddBlock(new trace::Block(std::stoi($2)));
            driver.exprs.clear();
          }
          ;


exprs : expr {  }
      | exprs expr {  }
      ;


expr : NEW_EVENT NUMBER event_type meta_vars {
       trace::NewEventExpr *new_event = new trace::NewEventExpr(
           std::stoi($2), $3);
       for (auto const &debug_info : $4) {
         new_event->AddDebugInfo(debug_info);
       }
       driver.exprs.push_back(new_event);
     }
     | NEW_EVENT NUMBER event_type {
       driver.exprs.push_back(new trace::NewEventExpr(std::stoi($2), $3));
     }
     | LINK NUMBER NUMBER {
       driver.exprs.push_back(new trace::LinkExpr(
           std::stoi($2), std::stoi($3)));
     }
     | TRIGGER NUMBER {
       driver.exprs.push_back(new trace::Trigger(std::stoi($2)));
     }
     | SUBMIT_OP OPID SYNC meta_vars {
       trace::SubmitOp *submit_op = new trace::SubmitOp($2);
       for (auto const &debug_info : $4) {
         submit_op->AddDebugInfo(debug_info);
       }
       driver.exprs.push_back(submit_op);
     }
     | SUBMIT_OP OPID NUMBER ASYNC meta_vars {
       trace::SubmitOp *submit_op = new trace::SubmitOp($2, std::stoi($3));
       for (auto const &debug_info : $5) {
         submit_op->AddDebugInfo(debug_info);
       }
       driver.exprs.push_back(submit_op);
     }
     ;


event_type : S NUMBER { $$ = trace::Event(trace::Event::S, std::stoi($2)); }
           | M NUMBER { $$ = trace::Event(trace::Event::M, std::stoi($2)); }
           | W NUMBER { $$ = trace::Event(trace::Event::W, std::stoi($2)); }
           | EXTERNAL { $$ = trace::Event(trace::Event::EXT, 0); }
           ;

stats : EXCLAMATION IDENTIFIER COLON NUMBER
      | stats EXCLAMATION IDENTIFIER COLON NUMBER
      ;

meta_vars : EXCLAMATION IDENTIFIER {
            $$ = std::vector<std::string>(); $$.push_back($2);
          }
          | EXCLAMATION RENAME {
            $$ = std::vector<std::string>(); $$.push_back("rename");
          }
          | meta_vars EXCLAMATION IDENTIFIER  { $1.push_back($3); $$ = $1; }
          ;

%%

void fstrace::TraceParser::error (const location_type& l,
                                  const std::string& m) {
  std::stringstream ss;
  ss << l;
  driver.AddError(utils::err::TRACE_ERROR, m, ss.str());
}
