#ifndef LABELBUDDY_DATABASE_IMPL_H
#define LABELBUDDY_DATABASE_IMPL_H

#include <QJsonArray>

#include "database.h"

namespace labelbuddy {

struct Annotation {
  static constexpr int nullIndex = -1;

  int startChar;
  int endChar;
  QString labelName;
  QString extraData;
  int startByte;
  int endByte;
};

struct DocRecord {
  QString content{};
  QByteArray metadata{};
  QString declaredMd5{};
  QList<Annotation> annotations{};
  bool validContent = true;
  QString displayTitle{};
  QString listTitle{};
};

class DocsReader {

public:
  explicit DocsReader(const QString& filePath);
  virtual ~DocsReader() = default;
  bool isOpen() const;
  bool hasError() const;
  ErrorCode errorCode() const;
  QString errorMessage() const;
  virtual bool readNext() = 0;
  const DocRecord* getCurrentRecord() const;
  virtual int progressMax() const;
  virtual int currentProgress() const;

protected:
  QFile* getFile();
  void setCurrentRecord(std::unique_ptr<DocRecord>);
  static constexpr int progressRangeMax_{1000};
  void setError(ErrorCode code, const QString& message);

private:
  QFile file_;
  std::unique_ptr<DocRecord> currentRecord_{nullptr};
  double fileSize_{};
  ErrorCode errorCode_ = ErrorCode::NoError;
  QString errorMessage_{};
};

class TxtDocsReader : public DocsReader {

public:
  explicit TxtDocsReader(const QString& filePath);
  bool readNext() override;

private:
  QTextStream stream_;
};

std::unique_ptr<DocRecord> jsonToDocRecord(const QJsonDocument&);
std::unique_ptr<DocRecord> jsonToDocRecord(const QJsonValue&);
std::unique_ptr<DocRecord> jsonToDocRecord(const QJsonObject&);

class JsonDocsReader : public DocsReader {

public:
  explicit JsonDocsReader(const QString& filePath);
  bool readNext() override;
  int progressMax() const override;
  int currentProgress() const override;

private:
  QJsonArray allDocs_;
  QJsonArray::const_iterator currentDoc_;
  int totalNDocs_;
  int nSeen_{};
};

class JsonLinesDocsReader : public DocsReader {

public:
  explicit JsonLinesDocsReader(const QString& filePath);
  bool readNext() override;

private:
  QTextStream stream_;
};

class DocsWriter {
public:
  explicit DocsWriter(const QString& filePath, bool includeText,
                      bool includeAnnotations);
  virtual ~DocsWriter() = default;

  /// after constructing the file should be open and isOpen should return true
  bool isOpen() const;
  bool isIncludingText() const;
  bool isIncludingAnnotations() const;

  virtual void addDocument(const QString& md5, const QString& content,
                           const QJsonObject& metadata,
                           const QList<Annotation>& annotations,
                           const QString& displayTitle,
                           const QString& listTitle) = 0;
  /// at the start of the output file
  virtual void writePrefix();

  /// at the end of the output file
  virtual void writeSuffix();

protected:
  QFile* getFile();

private:
  QFile file_;
  bool includeText_;
  bool includeAnnotations_;
};

class JsonLinesDocsWriter : public DocsWriter {
public:
  explicit JsonLinesDocsWriter(const QString& filePath, bool includeText,
                               bool includeAnnotations);
  void addDocument(const QString& md5, const QString& content,
                   const QJsonObject& metadata,
                   const QList<Annotation>& annotations,
                   const QString& displayTitle,
                   const QString& listTitle) override;

  void writeSuffix() override;

protected:
  int getNDocs() const;
  QTextStream& getStream();

private:
  QTextStream stream_;
  int nDocs_{};
};

class JsonDocsWriter : public JsonLinesDocsWriter {
public:
  explicit JsonDocsWriter(const QString& filePath, bool includeText,
                          bool includeAnnotations);

  void addDocument(const QString& md5, const QString& content,
                   const QJsonObject& metadata,
                   const QList<Annotation>& annotations,
                   const QString& displayTitle,
                   const QString& listTitle) override;
  void writePrefix() override;
  void writeSuffix() override;
};

struct LabelRecord {
  QString name;
  QString color{};
  QString shortcutKey{};
};

LabelRecord jsonToLabelRecord(const QJsonValue& json);

struct ReadLabelsResult {
  QJsonArray labels;
  ErrorCode errorCode;
  QString errorMessage;
};

/// read labels into a json array

/// returns the array and a (error code, error message) pair
ReadLabelsResult readLabels(const QString& filePath);

ReadLabelsResult readJsonLabels(QFile& file);

ReadLabelsResult readJsonLinesLabels(QFile& file);

ReadLabelsResult readTxtLabels(QFile& file);

/// remove a connection from the qt databases

/// unless `cancel` is called, removes the connection from qt database list when
/// deleted or when `execute` is called. used to clean up the added connection
/// if database cannot be used (eg if it is not a labelbuddy database).
/// if cancelled `execute` and the destructor do nothing.
class RemoveConnection {
  QString connectionName_;
  bool cancelled_;

public:
  explicit RemoveConnection(const QString& connectionName);
  ~RemoveConnection();
  void execute() const;
  void cancel();
};

QPair<QStringList, QString>
acceptedAndDefaultFormats(DatabaseCatalog::Action action,
                          DatabaseCatalog::ItemKind kind);

/// Last used database if it is found in QSettings and exists else ""
QString getDefaultDatabasePath();

/// return a reader appropriate for the filename extension
std::unique_ptr<DocsReader> getDocsReader(const QString& filePath);

/// return a writer appropriate for the filename extension
std::unique_ptr<DocsWriter> getDocsWriter(const QString& filePath,
                                          bool includeText,
                                          bool includeAnnotations);

ExportLabelsResult writeLabelsToJson(const QJsonArray& labels,
                                     const QString& filePath);
ExportLabelsResult writeLabelsToJsonLines(const QJsonArray& labels,
                                          const QString& filePath);
} // namespace labelbuddy

#endif
