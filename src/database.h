#ifndef LABELBUDDY_DATABASE_H
#define LABELBUDDY_DATABASE_H

#include <memory>

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QProgressDialog>
#include <QSet>
#include <QSqlQuery>
#include <QString>
#include <QTextStream>
#include <QVariant>
#include <QWidget>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace labelbuddy {

struct DocRecord {
  QString content;
  QByteArray extra_data;
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
};

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
                            const QString& user_name);
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
                    const QString& user_name

                    ) override;

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
                    const QString& user_name) override;
  void write_prefix() override;
  void write_suffix() override;
};

class AnnotationsXmlWriter : public AnnotationsWriter {
public:
  AnnotationsXmlWriter(const QString& file_path);
  void add_document(const QString& md5, const QVariant& content,
                    const QJsonObject& extra_data,
                    const QList<Annotation> annotations,
                    const QString& user_name) override;
  void write_prefix() override;
  void write_suffix() override;

private:
  QXmlStreamWriter xml;

  void add_annotations(const QList<Annotation> annotations);
};

class DatabaseCatalog : public QWidget {

  Q_OBJECT

public:
  void open_database(const QString& database_path = QString());
  int import_documents(const QString& file_path, QWidget* parent = nullptr);
  int import_labels(const QString& file_path);
  QList<int> export_annotations(const QString& file_path,
                                bool labelled_docs_only = true,
                                bool include_documents = true,
                                const QString& user_name = "");

  QString get_current_database() const;
  QString get_current_directory() const;
  QString parent_directory(const QString& file_path) const;
  QVariant get_app_state_extra(const QString& key,
                               const QVariant& default_value) const;
  void set_app_state_extra(const QString& key, const QVariant& value);

private:
  QSet<QString> databases{};
  QString current_database;

  void create_tables();
  QString get_default_database_path();
  int write_doc_and_annotations(AnnotationsWriter& writer, int doc_id,
                                bool include_document,
                                const QString& user_name);
};

} // namespace labelbuddy

#endif
