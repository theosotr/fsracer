#include <iomanip>
#include <iostream>
#include <ostream>

#include "Debug.h"
#include "Utils.h"


namespace debug {

msg::msg():
  os(std::cout),
  color(colors::NONE) {  }

msg::msg(std::string str):
  os(std::cout),
  color(colors::NONE) {
    os << "[" << str << "]: ";
}


msg::msg(std::ostream &os_):
  os(os_),
  color(colors::NONE) {  }


msg::msg(std::ostream &os_, enum colors::Colors color_):
  os(os_),
  color(color_) {
    PrintColor();
}


msg::msg(enum colors::Colors color_):
  os(std::cout),
  color(color_) {
    PrintColor();
}


msg::~msg() {
  ResetColor();
  os << std::endl;
}


msg &msg::operator<<(char c) {
  os << c;
  return *this;
}


msg &msg::operator<<(signed char c) {
  os << c;
  return *this;
}


msg &msg::operator<<(unsigned char c) {
  os << c;
  return *this;
}


msg &msg::operator<<(const char *buf) {
  os << buf;
  return *this;
}


msg &msg::operator<<(const std::string &str) {
  os << str;
  return *this;
}


msg &msg::operator<<(unsigned long N) {
  os << N;
  return *this;
}


msg &msg::operator<<(long N) {
  os << N;
  return *this;
}


msg &msg::operator<<(unsigned int N) {
  os << N;
  return *this;
}


msg &msg::operator<<(int N) {
  os << N;
  return *this;
}


msg &msg::operator<<(unsigned short N) {
  os << N;
  return *this;
}


msg &msg::operator<<(short N) {
  os << N;
  return *this;
}

msg &msg::operator<<(double D) {
  // Dump double up to 3 decimals.
  os << std::fixed << std::setprecision(3) << D;
  return *this;
}


msg &msg::operator<<(const void *P) {
  os << utils::PtrToString(P);
  return *this;
}


void msg::PrintColor() {
  switch (color) {
    case colors::NONE:
      break;
    default:
      os << "\x1b[0;3" << color << "m";
  }
}


void msg::ResetColor() {
  switch (color) {
    case colors::NONE:
      break;
    default:
      os << "\x1b[0m";
  }
}


void msg::PrintPrefix(const std::string &prefix) {
  os << prefix << ": ";
}


void msg::PrintPrefix(const std::string &prefix, const std::string &msg) {
  os << prefix << '[' << msg << "]: ";
}


} // namespace debug
