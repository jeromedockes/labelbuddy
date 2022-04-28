#ifndef LABELBUDDY_DATABASE_IMPL_H
#define LABELBUDDY_DATABASE_IMPL_H

#include <QJsonArray>

#include "database.h"

namespace labelbuddy {

struct Annotation {
  int start_char;
  int end_char;
  QString label_name;
  QString extra_data;
};

struct DocRecord {
  QString content{};
  QByteArray metadata{};
  QString declared_md5{};
  QList<Annotation> annotations{};
  bool valid_content = true;
  QString display_title{};
  QString list_title{};
};

class DocsReader {

public:
  DocsReader(const QString& file_path);
  virtual ~DocsReader() = default;
  bool is_open() const;
  bool has_error() const;
  ErrorCode error_code() const;
  QString error_message() const;
  virtual bool read_next() = 0;
  const DocRecord* get_current_record() const;
  virtual int progress_max() const;
  virtual int current_progress() const;

protected:
  QFile* get_file();
  void set_current_record(std::unique_ptr<DocRecord>);
  static constexpr int progress_range_max_{1000};
  void set_error(ErrorCode code, const QString& message);

private:
  QFile file_;
  std::unique_ptr<DocRecord> current_record_{nullptr};
  double file_size_{};
  ErrorCode error_code_ = ErrorCode::NoError;
  QString error_message_{};
};

class TxtDocsReader : public DocsReader {

public:
  TxtDocsReader(const QString& file_path);
  bool read_next() override;

private:
  QTextStream stream_;
};

std::unique_ptr<DocRecord> json_to_doc_record(const QJsonDocument&);
std::unique_ptr<DocRecord> json_to_doc_record(const QJsonValue&);
std::unique_ptr<DocRecord> json_to_doc_record(const QJsonObject&);

class JsonDocsReader : public DocsReader {

public:
  JsonDocsReader(const QString& file_path);
  bool read_next() override;
  int progress_max() const override;
  int current_progress() const override;

private:
  QJsonArray all_docs_;
  QJsonArray::const_iterator current_doc_;
  int total_n_docs_;
  int n_seen_{};
};

class JsonLinesDocsReader : public DocsReader {

public:
  JsonLinesDocsReader(const QString& file_path);
  bool read_next() override;

private:
  QTextStream stream_;
};

class DocsWriter {
public:
  DocsWriter(const QString& file_path, bool include_text,
             bool include_annotations);
  virtual ~DocsWriter() = default;

  /// after constructing the file should be open and is_open should return true
  bool is_open() const;
  bool is_including_text() const;
  bool is_including_annotations() const;

  virtual void add_document(const QString& md5, const QString& content,
                            const QJsonObject& metadata,
                            const QList<Annotation>& annotations,
                            const QString& display_title,
                            const QString& list_title) = 0;
  /// at the start of the output file
  virtual void write_prefix();

  /// at the end of the output file
  virtual void write_suffix();

protected:
  QFile* get_file();

private:
  QFile file_;
  bool include_text_;
  bool include_annotations_;
};

class JsonLinesDocsWriter : public DocsWriter {
public:
  JsonLinesDocsWriter(const QString& file_path, bool include_text,
                      bool include_annotations);
  void add_document(const QString& md5, const QString& content,
                    const QJsonObject& metadata,
                    const QList<Annotation>& annotations,
                    const QString& display_title,
                    const QString& list_title) override;

  void write_suffix() override;

protected:
  int get_n_docs() const;
  QTextStream& get_stream();

private:
  QTextStream stream_;
  int n_docs_{};
};

class JsonDocsWriter : public JsonLinesDocsWriter {
public:
  JsonDocsWriter(const QString& file_path, bool include_text,
                 bool include_annotations);

  void add_document(const QString& md5, const QString& content,
                    const QJsonObject& metadata,
                    const QList<Annotation>& annotations,
                    const QString& display_title,
                    const QString& list_title) override;
  void write_prefix() override;
  void write_suffix() override;
};

struct LabelRecord {
  QString name;
  QString color{};
  QString shortcut_key{};
};

LabelRecord json_to_label_record(const QJsonValue& json);

struct ReadLabelsResult {
  QJsonArray labels;
  ErrorCode error_code;
  QString error_message;
};

/// read labels into a json array

/// returns the array and a (error code, error message) pair
ReadLabelsResult read_labels(const QString& file_path);

ReadLabelsResult read_json_labels(QFile& file);

ReadLabelsResult read_json_lines_labels(QFile& file);

ReadLabelsResult read_txt_labels(QFile& file);

/// remove a connection from the qt databases

/// unless `cancel` is called, removes the connection from qt database list when
/// deleted or when `execute` is called. used to clean up the added connection
/// if database cannot be used (eg if it is not a labelbuddy database).
/// if cancelled `execute` and the destructor do nothing.
class RemoveConnection {
  QString connection_name_;
  bool cancelled_;

public:
  RemoveConnection(const QString& connection_name);
  ~RemoveConnection();
  void execute() const;
  void cancel();
};

QPair<QStringList, QString>
accepted_and_default_formats(DatabaseCatalog::Action action,
                             DatabaseCatalog::ItemKind kind);

/// Last used database if it is found in QSettings and exists else ""
QString get_default_database_path();

/// return a reader appropriate for the filename extension
std::unique_ptr<DocsReader> get_docs_reader(const QString& file_path);

/// return a writer appropriate for the filename extension
std::unique_ptr<DocsWriter> get_docs_writer(const QString& file_path,
                                            bool include_text,
                                            bool include_annotations);

ExportLabelsResult write_labels_to_json(const QJsonArray& labels,
                                        const QString& file_path);
ExportLabelsResult write_labels_to_json_lines(const QJsonArray& labels,
                                              const QString& file_path);
} // namespace labelbuddy

#endif
