#include <iomanip>
#include <iostream>
#include <ostream>

#include "Debug.h"
#include "Utils.h"


namespace debug {

outs::outs():
  os(std::cout),
  color(colors::NONE) {  }

outs::outs(std::string str):
  os(std::cout),
  color(colors::NONE) {
    os << "[" << str << "]: ";
}


outs::outs(std::ostream &os_):
  os(os_),
  color(colors::NONE) {  }


outs::outs(std::ostream &os_, enum colors::Colors color_):
  os(os_),
  color(color_) {
    PrintColor();
}


outs::outs(enum colors::Colors color_):
  os(std::cout),
  color(color_) {
    PrintColor();
}


outs::~outs() {
  ResetColor();
  os << std::endl;
}


outs &outs::operator<<(char c) {
  os << c;
  return *this;
}


outs &outs::operator<<(signed char c) {
  os << c;
  return *this;
}


outs &outs::operator<<(unsigned char c) {
  os << c;
  return *this;
}


outs &outs::operator<<(const char *buf) {
  os << buf;
  return *this;
}


outs &outs::operator<<(const std::string &str) {
  os << str;
  return *this;
}


outs &outs::operator<<(unsigned long N) {
  os << N;
  return *this;
}


outs &outs::operator<<(long N) {
  os << N;
  return *this;
}


outs &outs::operator<<(unsigned int N) {
  os << N;
  return *this;
}


outs &outs::operator<<(int N) {
  os << N;
  return *this;
}


outs &outs::operator<<(unsigned short N) {
  os << N;
  return *this;
}


outs &outs::operator<<(short N) {
  os << N;
  return *this;
}

outs &outs::operator<<(double D) {
  // Dump double up to 3 decimals.
  os << std::fixed << std::setprecision(3) << D;
  return *this;
}


outs &outs::operator<<(const void *P) {
  os << utils::PtrToString(P);
  return *this;
}


void outs::PrintColor() {
  switch (color) {
    case colors::NONE:
      break;
    default:
      os << "\x1b[0;3" << color << "m";
  }
}


void outs::ResetColor() {
  switch (color) {
    case colors::NONE:
      break;
    default:
      os << "\x1b[0m";
  }
}


void outs::PrintPrefix(const std::string &prefix) {
  os << prefix << ": ";
}


void outs::PrintPrefix(const std::string &prefix, const std::string &msg) {
  os << prefix << '[' << msg << "]: ";
}


} // namespace debug
