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


ostream &OutWriter::OutStream() {
  switch (write_option) {
    case WRITE_FILE:
      return of;
    default:
      return cout;
  }
}


string OutWriter::ToString() {
  switch (write_option) {
    case WRITE_STDOUT:
      return "STDOUT";
    case WRITE_FILE:
      return "FILE '" + filename + "'";
  }
}


}
