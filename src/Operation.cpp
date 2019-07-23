#include "Analyzer.h"
#include "Operation.h"

namespace operation {


void NewFd::Accept(analyzer::Analyzer *analyzer) {
  if (analyzer) {
    analyzer->AnalyzeNewFd(this);
  }
};


void DelFd::Accept(analyzer::Analyzer *analyzer) {
  if (analyzer) {
    analyzer->AnalyzeDelFd(this);
  }
};


void Hpath::Accept(analyzer::Analyzer *analyzer) {
  if (analyzer) {
    analyzer->AnalyzeHpath(this);
  }
};


void HpathSym::Accept(analyzer::Analyzer *analyzer) {
  if (analyzer) {
    analyzer->AnalyzeHpath(this);
  }
};


void Link::Accept(analyzer::Analyzer *analyzer) {
  if (analyzer) {
    analyzer->AnalyzeLink(this);
  }
};


void Rename::Accept(analyzer::Analyzer *analyzer) {
  if (analyzer) {
    analyzer->AnalyzeRename(this);
  }
};


void Symlink::Accept(analyzer::Analyzer *analyzer) {
  if (analyzer) {
    analyzer->AnalyzeSymlink(this);
  }
};

}
