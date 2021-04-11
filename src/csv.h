#ifndef LABELBUDDY_CSV_H
#define LABELBUDDY_CSV_H

#include <QChar>
#include <QList>
#include <QMap>
#include <QString>
#include <QTextStream>

namespace labelbuddy {

static constexpr QChar cr = '\r';
static constexpr QChar lf = '\n';
static constexpr QChar dquote = '"';
static constexpr QChar comma = ',';

/// read excel-style (rfc4180, https://tools.ietf.org/html/rfc4180) or
/// unix-style (same with \n instead of \r\n) csv.
/// eg
/// abc,"def","gh
/// ""hello"" ijk "
class CsvReader {

public:
  /// `in_stream` should _not_ be opened in mode QIODevice::Text
  CsvReader(QTextStream* in_stream);

  QList<QString> read_record();
  QString read_field();

  bool at_end() const;
  bool at_record_end() const;

  /// true if an error was found; in this case `at_end` and `at_record_end` also
  /// return `true`.
  bool found_error() const;

private:
  bool at_record_end_{};
  bool found_error_{};
  QString current_field_{};
  bool quoted_{};

  void read_quoted_field();
  void read_unquoted_field();
  void check_field_ending();
  void set_found_error();

  // reading tokens from the stream

  enum class CsvToken { Empty, SingleChar, Crlf, Double_dquote };

  CsvToken current_token_{};
  QChar current_char_{};
  QTextStream* in_stream_;
  static constexpr int buffer_size_ = 32;
  QChar buffer_[buffer_size_]{};
  int data_start_{};
  int data_end_{};

  inline bool found_prefix() const {
    return (quoted_ && current_char_ == dquote) ||
           (!quoted_ && current_char_ == cr);
  }
  bool read_token();
  bool at_data_end() const {
    return (data_start_ == data_end_) && in_stream_->atEnd();
  }
  bool read_char();
  void shift_buffer();
  inline int data_size() const { return data_end_ - data_start_; }
};

/// Read records from a csv and return them as a QMap

/// the first row of the csv is interpreted as a header and the keys of each
/// record are the column names. if columns have duplicate names the first one
/// is used. If a record has more fields than the header the extra fields are
/// skipped.
class CsvMapReader {

public:
  /// `in_stream` should _not_ be opened in mode QIODevice::Text
  CsvMapReader(QTextStream* in_stream);
  QMap<QString, QString> read_record();
  const QList<QString>& header() const;
  bool at_end() const;
  bool found_error() const;

private:
  CsvReader reader_;
  QList<QString> header_{};
};

/// write rfc4180 csv, excel-style (fields quoted only when necessary)

/// eg
/// abc,def,"gh
/// ""hello"" ijk"

/// fields that contain any of '"', '\r', '\n', ',' are quoted.

/// document will be utf-8 encoded with a BOM so it is correctly read as utf-8
/// eg by libreoffice or excel on windows.

/// by default a first column is added; the content of the first row is
/// 'ignore_this_column' and the other row are empty. Fields in this column are
/// not quoted.
/// This way, if the csv is read by a tool that can read utf-8 but does not
/// expect and skip the BOM (e.g. Python on unix), the first empty column's name
/// will be '0xfeffignore_this_column' instead of 'ignore_this_column' but the
/// other columns (those that contain actual data) won't be affected.
/// This behaviour can be disabled by passing `start_with_empty_col = false`.
class CsvWriter {
public:
  /// `out_stream` should _not_ be opened in mode QIODevice::Text
  CsvWriter(QTextStream* out_stream, bool start_with_empty_col = true);
  template <typename IteratorType>
  void write_record(IteratorType first, IteratorType last);

private:
  QTextStream* out_stream_;
  bool start_with_empty_col_;
  bool wrote_empty_col_header_{false};

  void write_empty_col_header_if_needed();
};
} // namespace labelbuddy

#endif
