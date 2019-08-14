#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <ostream>


namespace debug {

namespace colors {

/**
 * An enumeration that defines the colors
 * corresponding to ANSI escape sequence.
 *
 * The element called `NONE` corresponds to the default
 * case; so we do not override colors.
 */
enum Colors {
  BLACK = 0,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA,
  CYAN,
  WHITE,
  NONE,
};

}; // namespace colors


/**
 * This is a wrapper class that prints message to an output stream.
 */
class msg {
public:
  /**
   * Default Constructor.
   *
   * Enables printing to standard output without colors.
   */
  msg();

  /**
   * Enables printing to standard output without colors.
   *
   * The parameter `str` corresponds to the initial string to
   * be printed.
   */
  msg(std::string str);

  /**
   * Enables printing to the given output stream without colors.
   */
  msg(std::ostream &os_);

  /**
   * Enables printing to the given output stream with the given color.
   */
  msg(std::ostream &os_, enum colors::Colors color_);
  
  /**
   * Enables printing to the standard output with the given color.
   */
  msg(enum colors::Colors color_);
  
  /** Destructor of the wrapper class. */
  ~msg();

  // --- Methods for overloading the '<<' operator ---
  msg &operator<<(char c);
  msg &operator<<(signed char c);
  msg &operator<<(unsigned char c);
  msg &operator<<(const char *buf);
  msg &operator<<(const std::string &str);
  msg &operator<<(unsigned long N);
  msg &operator<<(long N);
  msg &operator<<(unsigned int N);
  msg &operator<<(int N);
  msg &operator<<(unsigned short N);
  msg &operator<<(short N);
  msg &operator<<(double D);
  msg &operator<<(const void *P);

  /** Prints the ANSI character sequence that enables colors. */
  void PrintColor();

  /** Resets colors to the default case. */
  void ResetColor();

protected:
  /** The output stream used for printing. */
  std::ostream &os;
  /** Color used for printing. */
  enum colors::Colors color;

  /** Print the specified prefix before any stream. */
  void PrintPrefix(const std::string &prefix);
  
  /**
   * Print the specified prefix along with
   * the given message before any stream. */
  void PrintPrefix(const std::string &prefix, const std::string &msg);

};


/**
 * This class is used for priting informative messages.
 *
 * Any message is printed to the standard output using the
 * color `GREEN`.
 */
class info : public msg {
public:
  info():
    msg(colors::GREEN) {
      PrintPrefix(PREFIX_MSG);
  }

  info(std::string str):
    msg(colors::GREEN) {
      PrintPrefix(PREFIX_MSG, str);
  }

private:
  static constexpr const char *PREFIX_MSG = "Info";
};


/**
 * This class is used for printing warning messages.
 *
 * Any message is printed to the standard output using the
 * color `YELLOW`.
 */
class warn : public msg {
public:
  warn():
    msg(colors::YELLOW) {
      PrintPrefix(PREFIX_MSG);
  }

  warn(std::string str):
    msg(colors::YELLOW) {
      PrintPrefix(PREFIX_MSG, str);
  }

private:
  static constexpr const char *PREFIX_MSG = "Warning";
};


/**
 * This class is used for printing erroneous messages.
 *
 * Any message is printed to the standard error using the
 * color `RED`.
 */
class err : public msg {
public:
  err():
    msg(std::cerr, colors::RED) {
      PrintPrefix(PREFIX_MSG);
  }

  err(std::string str):
    msg(std::cerr, colors::RED) {
      PrintPrefix(PREFIX_MSG, str);
  }

private:
  static constexpr const char *PREFIX_MSG = "Error";
};


} // namespace debug

#endif
