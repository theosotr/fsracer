#ifndef OUT_WRITER_H
#define OUT_WRITER_H

#include <fstream>
#include <iostream>

using namespace std;


namespace writer {

/**
 * This class is responsible for writing to either
 * standard output or a file.
 */
class OutWriter {
  public:
    /**
     * This enumeration indicates the writing option.
     * It's either writing to standard output or to a file.
     */
    enum WriteOption {
      WRITE_STDOUT,
      WRITE_FILE
    };

    /** Constructor that initializes the output stream. */
    OutWriter(enum WriteOption write_option_, string filename_):
      write_option(write_option_),
      filename(filename_) {
        SetupOutStream();
    }

    /** Destructor that clears the output stream. */
    ~OutWriter() {
      ClearOutStream();
    }

    /** Gets the output stream used internally. */
    ostream &OutStream();

  private:
    /// Writing option. */
    enum WriteOption write_option;

    /// Name of the file to which we are writing (used with WRITE_FILE option).
    string filename;

    /// Output file stream.
    ofstream of;

    /** Set the output stream up. */
    void SetupOutStream();

    /**
     * Clears the output stream (e.g., closes the file descriptor of the file).
     */
    void ClearOutStream();
};


}

#endif
