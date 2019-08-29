%skeleton "lalr1.cc" /* -*- C++ -*- */
%defines
%define api.namespace {fstrace}
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
    #include <iostream>
    #include <string>
    #include <vector>
    #include <stdint.h>

    #include "Operation.h"
    #include "Trace.h"

    namespace fstrace {
        class driver;
        class Scanner;
    }
}

%lex-param { fstrace::Scanner &scanner  }
%parse-param { fstrace::Scanner &scanner  }
%parse-param { fstrace::driver &driver  }
%locations
%define parse.trace
%define parse.error verbose

%code top {
    #include "driver.hpp"
    #include "scanner.hpp"

    static fstrace::parser::symbol_type yylex(fstrace::Scanner &scanner) {
      return scanner.get_next_token();           
    }
}


%define api.token.prefix {TOK_}

%token
  END 0 "end of file"
  COLON ":"
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
  SUBMIT_OP     "submitOp"
  SYNC          "SYNC"
  ASYNC         "ASYNC"
  S M W EXTERNAL
  PRODUCED      "produced"
  CONSUMED      "consumed"
  EXPUNGED      "expunged"
;
%token <std::string>
  META "meta variable"
  ID
  OPID "operation id"
  PATH "path"

%type <int> dirfd
%type <operation::Hpath::EffectType> effect_type
%type <trace::Event> event_type

%start program

%%

program : PID COLON ID CWD COLON PATH op_defs block_defs {
          driver.trace_f->SetThreadId(std::stoi($3));
          driver.trace_f->SetCwd($6);
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


oper : HPATH dirfd PATH effect_type meta_vars {
       operation::Hpath *hpath = new operation::Hpath($2, $3, $4);
       driver.opers.push_back(hpath);
     }
     | HPATHSYM dirfd PATH effect_type meta_vars {
       operation::HpathSym *hpath = new operation::HpathSym($2, $3, $4);
       driver.opers.push_back(hpath);
     }
     | NEWFD dirfd PATH ID meta_vars {
       operation::NewFd *new_fd = new operation::NewFd(
           $2, $3, std::stoi($4));
       driver.opers.push_back(new_fd);
     }
     | DELFD ID meta_vars {
       operation::DelFd *del_fd = new operation::DelFd(std::stoi($2));
       driver.opers.push_back(del_fd);
     }
     | LINK dirfd PATH dirfd PATH meta_vars {
       operation::Link *link = new operation::Link($2, $3, $4, $5);
       driver.opers.push_back(link);
     }
     | RENAME dirfd PATH dirfd PATH meta_vars {
       operation::Rename *rename = new operation::Rename($2, $3, $4, $5);
       driver.opers.push_back(rename);
     }
     | SYMLINK dirfd PATH PATH meta_vars {
       operation::Symlink *symlink = new operation::Symlink($2, $3, $4);
       driver.opers.push_back(symlink);
     }
     ;


dirfd : ATFDCWD { $$ = AT_FDCWD; }
      | ID { $$ = std::stoi($1); }
      ;


effect_type : CONSUMED { $$ = operation::Hpath::CONSUMED; }
            | PRODUCED { $$ = operation::Hpath::PRODUCED; }
            | EXPUNGED { $$ = operation::Hpath::EXPUNGED; }
            ;


block_defs : block_def { }
           | block_defs block_def { }
           ;


block_def : BEGIN_BLOCK MAIN exprs END_BLOCK {
            trace::Block *block = new trace::Block(0);
            for (auto const &expr_entry : driver.exprs) {
              block->AddExpr(expr_entry);
            }
            driver.trace_f->AddBlock(block);
            driver.exprs.clear();
          }
          | BEGIN_BLOCK ID exprs END_BLOCK {
            trace::Block *block = new trace::Block(std::stoi($2));
            for (auto const &expr_entry : driver.exprs) {
              block->AddExpr(expr_entry);
            }
            driver.trace_f->AddBlock(block);
            driver.exprs.clear();
          }
          | BEGIN_BLOCK ID END_BLOCK {
            driver.trace_f->AddBlock(new trace::Block(std::stoi($2)));
            driver.exprs.clear();
          }
          ;


exprs : expr {  }
      | exprs expr {  }
      ;


expr : NEW_EVENT ID event_type meta_vars {
       driver.exprs.push_back(new trace::NewEventExpr(std::stoi($2), $3));
     }
     | NEW_EVENT ID event_type {
       driver.exprs.push_back(new trace::NewEventExpr(std::stoi($2), $3));
     }
     | LINK ID ID {
       driver.exprs.push_back(new trace::LinkExpr(std::stoi($2), std::stoi($3)));
     }
     | SUBMIT_OP OPID SYNC meta_vars {
       driver.exprs.push_back(new trace::SubmitOp($2));
     }
     | SUBMIT_OP OPID ID ASYNC meta_vars {
       driver.exprs.push_back(new trace::SubmitOp($2, std::stoi($3)));
     }
     ;


event_type : S ID { $$ = trace::Event(trace::Event::S, std::stoi($2)); }
           | M ID { $$ = trace::Event(trace::Event::M, std::stoi($2)); }
           | W ID { $$ = trace::Event(trace::Event::W, std::stoi($2)); }
           | EXTERNAL { $$ = trace::Event(trace::Event::EXT, 0); }
           ;


meta_vars : META {  }
          | meta_vars META  { }
          ;
%%

void fstrace::parser::error (const location_type& l, const std::string& m) {
  std::cerr << l << ": " << m << '\n';  
}
