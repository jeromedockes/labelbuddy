#include <cassert>

#include <QFile>

#include "csv.h"

namespace labelbuddy {

CsvReader::CsvReader(QTextStream* in_stream) : in_stream_{in_stream} {
  assert(in_stream_->device() != nullptr);
  assert(!(in_stream_->device()->openMode() & QIODevice::Text));
  in_stream->setCodec("UTF-8");
}

void CsvReader::shift_buffer() {
  if (data_start_ == data_end_) {
    data_start_ = 0;
    data_end_ = 0;
    return;
  }
  assert(data_end_ == data_start_ + 1);
  buffer_[0] = buffer_[data_start_];
  data_start_ = 0;
  data_end_ = 1;
}

bool CsvReader::read_char() {
  if (in_stream_->atEnd()) {
    return false;
  }
  if (data_end_ == buffer_size_) {
    shift_buffer();
  }
  *in_stream_ >> buffer_[data_end_];
  ++data_end_;
  return true;
}

bool CsvReader::read_token() {
  assert(data_size() <= 1);
  if (!data_size()) {
    if (!read_char()) {
      current_char_ = '\0';
      current_token_ = CsvToken::Empty;
      return false;
    }
  }
  assert(data_size() == 1);
  current_char_ = buffer_[data_start_];
  current_token_ = CsvToken::SingleChar;
  ++data_start_;
  if (!found_prefix()) {
    return true;
  }
  if (!read_char()) {
    return true;
  }
  assert(data_size() == 1);
  if (current_char_ == dquote && buffer_[data_start_] == dquote) {
    current_token_ = CsvToken::Double_dquote;
    ++data_start_;
    return true;
  }
  if (current_char_ == cr && buffer_[data_start_] == lf) {
    current_token_ = CsvToken::Crlf;
    current_char_ = lf;
    ++data_start_;
    return true;
  }
  return true;
}

bool CsvReader::at_end() const { return at_data_end() || found_error_; }

bool CsvReader::at_record_end() const {
  assert(at_record_end_ || (!found_error_));
  return at_record_end_ || found_error_;
}

bool CsvReader::found_error() const { return found_error_; }

QString CsvReader::read_field() {
  current_field_.resize(0);
  quoted_ = false;
  if (!read_token()) {
    check_field_ending();
    return current_field_;
  }
  if (current_char_ == lf || current_char_ == comma) {
    check_field_ending();
    return current_field_;

  } else if (current_char_ == dquote) {
    read_quoted_field();
    return current_field_;
  } else {
    current_field_.append(current_char_);
    read_unquoted_field();
    return current_field_;
  }
}

void CsvReader::check_field_ending() {
  switch (current_token_) {
  case CsvToken::Empty:
    at_record_end_ = true;
    found_error_ = false;
    return;
  case CsvToken::Crlf:
    at_record_end_ = true;
    found_error_ = false;
    return;
  case CsvToken::SingleChar:
    if (current_char_ == comma) {
      at_record_end_ = false;
      found_error_ = false;
      return;
    } else if (current_char_ == lf) {
      at_record_end_ = true;
      found_error_ = false;
      return;
    } else {
      set_found_error();
      return;
    }
  default:
    set_found_error();
    return;
  }
}

void CsvReader::set_found_error() {
  at_record_end_ = true;
  found_error_ = true;
}

void CsvReader::read_quoted_field() {
  quoted_ = true;
  while (read_token()) {
    assert(current_token_ == CsvToken::Double_dquote ||
           current_token_ == CsvToken::SingleChar);
    if (current_char_ == dquote && current_token_ == CsvToken::SingleChar) {
      quoted_ = false;
      read_token();
      check_field_ending();
      return;
    } else {
      current_field_.append(current_char_);
    }
  }
  // arrived at the end of stream without finding closing quote
  set_found_error();
}

void CsvReader::read_unquoted_field() {
  quoted_ = false;
  while (read_token()) {
    assert(current_token_ != CsvToken::Empty);
    if (current_char_ == dquote) {
      set_found_error();
      return;
    } else if (current_char_ == comma || current_char_ == lf) {
      check_field_ending();
      return;
    } else {
      current_field_.append(current_char_);
    }
  }
  check_field_ending();
}

QList<QString> CsvReader::read_record() {
  QList<QString> record{};
  if (at_end()) {
    return record;
  }
  do {
    record.append(read_field());
  } while (!at_record_end());
  return record;
}

CsvMapReader::CsvMapReader(QTextStream* in_stream) : reader_{in_stream} {
  header_ = reader_.read_record();
}
bool CsvMapReader::at_end() const { return reader_.at_end(); }
bool CsvMapReader::found_error() const { return reader_.found_error(); }

const QList<QString>& CsvMapReader::header() const { return header_; }

QMap<QString, QString> CsvMapReader::read_record() {
  QMap<QString, QString> record{};
  if (at_end()) {
    return record;
  }
  int i{};
  do {
    if (i < header_.size() && !record.contains(header_[i])) {
      record[header_[i]] = reader_.read_field();
    } else {
      // discard fields beyond last header or that correspond to a duplicate
      // column name
      reader_.read_field();
    }
    ++i;
  } while (!reader_.at_record_end());
  return record;
}

CsvWriter::CsvWriter(QTextStream* out_stream, bool start_with_empty_col)
    : out_stream_{out_stream}, start_with_empty_col_{start_with_empty_col} {
  assert(out_stream_->device() != nullptr);
  assert(!(out_stream_->device()->openMode() & QIODevice::Text));
  out_stream->setCodec("UTF-8");
  out_stream->setGenerateByteOrderMark(true);
}

void CsvWriter::write_empty_col_header_if_needed() {
  if (start_with_empty_col_ && (!wrote_empty_col_header_)) {
    *out_stream_ << "ignore_this_column";
    wrote_empty_col_header_ = true;
  }
}

template <typename IteratorType>
void CsvWriter::write_record(IteratorType first, IteratorType last) {
  write_empty_col_header_if_needed();
  QString quoted{};
  bool is_first_field_in_row{!start_with_empty_col_};
  while (first != last) {
    if (!is_first_field_in_row) {
      *out_stream_ << ",";
    }
    is_first_field_in_row = false;
    if (first->contains(QRegExp(R"([",\r\n])"))) {
      quoted = *first;
      quoted.replace(R"(")", R"("")");
      *out_stream_ << R"(")" << quoted << R"(")";
    } else {
      *out_stream_ << *first;
    }
    ++first;
  }
  *out_stream_ << "\r\n";
}

template void CsvWriter::write_record<QList<QString>::const_iterator>(
    QList<QString>::const_iterator, QList<QString>::const_iterator);

} // namespace labelbuddy
