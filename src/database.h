#ifndef LABELBUDDY_DATABASE_H
#define LABELBUDDY_DATABASE_H

#include <memory>

#include <QByteArray>
#include <QObject>
#include <QFile>
#include <QJsonArray>
#include <QProgressDialog>
#include <QSet>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryFile>
#include <QTextStream>
#include <QVariant>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace labelbuddy {

struct DocRecord {
  QString content;
  QByteArray extra_data;
  QString declared_md5;
  QJsonArray annotations;
  bool valid_content = true;
  QString title{};
  QString short_title{};
  QString long_title{};
};

class DocsReader {

public:
  DocsReader(const QString& file_path);
  bool is_open() const;
  virtual bool read_next();
  const DocRecord* get_current_record() const;
  virtual int progress_max() const;
  virtual int current_progress() const;

protected:
  QFile* get_file();
  void set_current_record(std::unique_ptr<DocRecord>);

private:
  QFile file;
  std::unique_ptr<DocRecord> current_record{nullptr};
};

class TxtDocsReader : public DocsReader {

public:
  TxtDocsReader(const QString& file_path);
  bool read_next() override;

private:
  QTextStream stream;
};

class XmlDocsReader : public DocsReader {

public:
  XmlDocsReader(const QString& file_path);
  bool read_next() override;

private:
  QXmlStreamReader xml;
  bool found_error{};
  void read_document();
  void read_document_text();
  void read_md5();
  void read_meta();
  void read_annotation_set();
  void read_annotation();
  void read_title();
  void read_short_title();
  void read_long_title();
  std::unique_ptr<DocRecord> new_record{};
};

std::unique_ptr<DocRecord> json_to_doc_record(const QJsonDocument&);
std::unique_ptr<DocRecord> json_to_doc_record(const QJsonValue&);
std::unique_ptr<DocRecord> json_to_doc_record(const QJsonObject&);
std::unique_ptr<DocRecord> json_to_doc_record(const QJsonArray&);

class JsonDocsReader : public DocsReader {

public:
  JsonDocsReader(const QString& file_path);
  bool read_next() override;
  int progress_max() const override;
  int current_progress() const override;

private:
  QJsonArray all_docs;
  QJsonArray::const_iterator current_doc;
  int total_n_docs;
  int n_seen{};
};

class JsonLinesDocsReader : public DocsReader {

public:
  JsonLinesDocsReader(const QString& file_path);
  bool read_next() override;

private:
  QTextStream stream;
};

class AnnotationsWriter {
public:
  struct Annotation {
    int start_char;
    int end_char;
    QString label_name;
  };

  AnnotationsWriter(const QString& file_path);
  bool is_open() const;
  virtual void add_document(const QString& md5, const QVariant& content,
                            const QJsonObject& extra_data,
                            const QList<Annotation> annotations,
                            const QString& user_name, const QString& title,
                            const QString& short_title,
                            const QString& long_title);
  virtual void write_prefix();
  virtual void write_suffix();

protected:
  QFile* get_file();

private:
  QFile file;
};

class AnnotationsJsonLinesWriter : public AnnotationsWriter {
public:
  AnnotationsJsonLinesWriter(const QString& file_path);
  void add_document(const QString& md5, const QVariant& content,
                    const QJsonObject& extra_data,
                    const QList<Annotation> annotations,
                    const QString& user_name, const QString& title,
                    const QString& short_title,
                    const QString& long_title) override;

  void write_suffix() override;

protected:
  int get_n_docs() const;
  QTextStream& get_stream();

private:
  QTextStream stream;
  int n_docs{};
};

class AnnotationsJsonWriter : public AnnotationsJsonLinesWriter {
public:
  AnnotationsJsonWriter(const QString& file_path);
  void add_document(const QString& md5, const QVariant& content,
                    const QJsonObject& extra_data,
                    const QList<Annotation> annotations,
                    const QString& user_name, const QString& title,
                    const QString& short_title,
                    const QString& long_title) override;
  void write_prefix() override;
  void write_suffix() override;
};

class AnnotationsXmlWriter : public AnnotationsWriter {
public:
  AnnotationsXmlWriter(const QString& file_path);
  void add_document(const QString& md5, const QVariant& content,
                    const QJsonObject& extra_data,
                    const QList<Annotation> annotations,
                    const QString& user_name, const QString& title,
                    const QString& short_title,
                    const QString& long_title) override;
  void write_prefix() override;
  void write_suffix() override;

private:
  QXmlStreamWriter xml;

  void add_annotations(const QList<Annotation> annotations);
};

struct LabelRecord {
  QString name;
  QString color;
};

LabelRecord json_to_label_record(const QJsonValue& json);
QJsonArray read_json_array(const QString& file_path);

class DatabaseCatalog : public QObject {

  Q_OBJECT

public:
  void open_database(const QString& database_path = QString(),
                     bool remember = true);
  QString open_temp_database();
  QList<int> import_documents(const QString& file_path,
                              QProgressDialog* progress = nullptr);
  int import_labels(const QString& file_path);
  QList<int> export_annotations(const QString& file_path,
                                bool labelled_docs_only = true,
                                bool include_documents = true,
                                const QString& user_name = "",
                                QProgressDialog* progress = nullptr);
  int export_labels(const QString& file_path);
  QString get_current_database() const;
  QString get_current_directory() const;
  QString get_last_opened_directory() const;
  QString parent_directory(const QString& file_path) const;
  QVariant get_app_state_extra(const QString& key,
                               const QVariant& default_value) const;
  void set_app_state_extra(const QString& key, const QVariant& value);
  void vacuum_db();

private:
  QSet<QString> databases{};
  QString current_database;
  std::unique_ptr<QTemporaryFile> temp_database = nullptr;

  void create_tables();
  QString get_default_database_path();
  int insert_doc_record(const DocRecord& record, QSqlQuery& query);
  void insert_label(QSqlQuery& query, const QString& label_name,
                    const QString& color = QString());
  int write_doc_and_annotations(AnnotationsWriter& writer, int doc_id,
                                bool include_document,
                                const QString& user_name);

  const QStringList label_colors{"#aec7e8", "#ffbb78", "#98df8a", "#ff9896",
                                 "#c5b0d5", "#c49c94", "#f7b6d2", "#c7c7c7",
                                 "#dbdb8d", "#9edae5"};
  int color_index{};
};

void batch_import_export(const QString& db_path,
                         const QList<QString>& labels_files,
                         const QList<QString>& docs_files,
                         const QString& export_labels_file,
                         const QString& export_docs_file,
                         bool labelled_docs_only, bool include_documents,
                         const QString& user_name, bool vacuum);

} // namespace labelbuddy

#endif
