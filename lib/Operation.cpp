#include "Analyzer.h"
#include "Operation.h"

namespace operation {


void NewFd::Accept(analyzer::Analyzer *analyzer) const {
  if (analyzer) {
    analyzer->AnalyzeNewFd(this);
  }
};


void DelFd::Accept(analyzer::Analyzer *analyzer) const {
  if (analyzer) {
    analyzer->AnalyzeDelFd(this);
  }
};


void Hpath::Accept(analyzer::Analyzer *analyzer) const {
  if (analyzer) {
    analyzer->AnalyzeHpath(this);
  }
};


string Hpath::EffToString(enum EffectType effect) {
  switch (effect) {
    case CONSUMED:
      return "consumed";
    case PRODUCED:
      return "produced";
    case EXPUNGED:
      return "expunged";
  }
}


bool Hpath::Consumes(enum EffectType effect) {
  switch (effect) {
    case CONSUMED:
      return true;
    default:
      return false;
  }
}


void HpathSym::Accept(analyzer::Analyzer *analyzer) const {
  if (analyzer) {
    analyzer->AnalyzeHpathSym(this);
  }
};


void Link::Accept(analyzer::Analyzer *analyzer) const {
  if (analyzer) {
    analyzer->AnalyzeLink(this);
  }
};


void Rename::Accept(analyzer::Analyzer *analyzer) const {
  if (analyzer) {
    analyzer->AnalyzeRename(this);
  }
};


void Symlink::Accept(analyzer::Analyzer *analyzer) const {
  if (analyzer) {
    analyzer->AnalyzeSymlink(this);
  }
};

}
