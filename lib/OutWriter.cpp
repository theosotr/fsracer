#include "OutWriter.h"


namespace writer {


void OutWriter::SetupOutStream() {
  switch (write_option) {
    case WRITE_FILE:
      of.open(filename);
    default:
      break;
  }
}


void OutWriter::ClearOutStream() {
  switch (write_option) {
    case WRITE_FILE:
      of.close();
    default:
      break;
  }
}


std::ostream &OutWriter::OutStream() {
  switch (write_option) {
    case WRITE_FILE:
      return of;
    default:
      return std::cout;
  }
}


std::string OutWriter::ToString() {
  switch (write_option) {
    case WRITE_STDOUT:
      return "STDOUT";
    default:
      return "FILE '" + filename + "'";
  }
}


}
