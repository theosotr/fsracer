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
class outs {
public:
  /**
   * Default Constructor.
   *
   * Enables printing to standard output without colors.
   */
  outs();

  /**
   * Enables printing to standard output without colors.
   *
   * The parameter `str` corresponds to the initial string to
   * be printed.
   */
  outs(std::string str);

  /**
   * Enables printing to the given output stream without colors.
   */
  outs(std::ostream &os_);

  /**
   * Enables printing to the given output stream with the given color.
   */
  outs(std::ostream &os_, enum colors::Colors color_);
  
  /**
   * Enables printing to the standard output with the given color.
   */
  outs(enum colors::Colors color_);
  
  /** Destructor of the wrapper class. */
  ~outs();

  // --- Methods for overloading the '<<' operator ---
  outs &operator<<(char c);
  outs &operator<<(signed char c);
  outs &operator<<(unsigned char c);
  outs &operator<<(const char *buf);
  outs &operator<<(const std::string &str);
  outs &operator<<(unsigned long N);
  outs &operator<<(long N);
  outs &operator<<(unsigned int N);
  outs &operator<<(int N);
  outs &operator<<(unsigned short N);
  outs &operator<<(short N);
  outs &operator<<(double D);
  outs &operator<<(const void *P);

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
class info : public outs {
public:
  info():
    outs(colors::GREEN) {
      PrintPrefix(PREFIX_MSG);
  }

  info(std::string str):
    outs(colors::GREEN) {
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
class warn : public outs {
public:
  warn():
    outs(colors::YELLOW) {
      PrintPrefix(PREFIX_MSG);
  }

  warn(std::string str):
    outs(colors::YELLOW) {
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
class err : public outs {
public:
  err():
    outs(std::cerr, colors::RED) {
      PrintPrefix(PREFIX_MSG);
  }

  err(std::string str):
    outs(std::cerr, colors::RED) {
      PrintPrefix(PREFIX_MSG, str);
  }

private:
  static constexpr const char *PREFIX_MSG = "Error";
};


} // namespace debug

#endif
