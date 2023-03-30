#ifndef LABELBUDDY_UTILS_H
#define LABELBUDDY_UTILS_H

#include <QCommandLineParser>
#include <QFont>
#include <QModelIndexList>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QUrl>
#include <QWidget>

/// \file
/// Misc. utilities

namespace labelbuddy {

/// The labelbuddy version 'x.y.z'
QString getVersion();

QString getWelcomeMessage();

/// URL to the html documentation

/// Searched in /usr/share/doc, the parent directory of the running program, and
/// if not found, return URL to the online doc.
QUrl getDocUrl(const QString& pageName = "documentation");

void prepareParser(QCommandLineParser& parser);

/// Find the first index that is in the column 0

/// If none of the indices are in column 0 return `indices.constEnd()`.
/// LabelList and DocList display column 0 but also have a hidden column
/// containing the row id. This is used to find the first selected item that is
/// in the displayed column -- hidden items could be selected by clicking
/// "select all".
///
/// In practice this is a redundant check because the corresponding models'
/// `flags` make the other columns non-selectable.
QModelIndexList::const_iterator findFirstInCol0(const QModelIndexList& indices);

/// ATM single lower-case letters. If `acceptEmpty` the empty string also
/// allowed.

/// Empty string can be acceptable to clear the label's shortcut in Dataset tab.
QRegularExpression shortcutKeyPattern(bool acceptEmpty = false);

/// Display string for the database name (path or helpful message if temp db)

/// `databaseName` is the Qt database name; if not temporary database it is the
/// path to the sqlite file. If `fullPath` is false only filename is returned;
/// if `tempWarning` and database is temporary or in-memory a message saying it
/// will disappear is added.
QString databaseNameDisplay(const QString& databaseName, bool fullPath = true,
                            bool tempWarning = true);

enum class Side { Left, Right };

void scaleMargin(QWidget& widget, Side side, float scale = .5);

const QString& suggestLabelColor(int colorIndex);
const QString& suggestLabelColor();

int castProgressToRange(double current, double maximum, double rangeMax = 1000);

/// compare sequences as sets ie irrespective of order or duplicates
template <typename C1, typename C2>
bool setCompare(const C1& container1, const C2& container2) {
  QSet<typename C1::value_type> set1{};
  for (const auto& val1 : container1) {
    set1 << val1;
  }
  QSet<typename C2::value_type> set2{};
  for (const auto& val2 : container2) {
    set2 << val2;
  }
  return set1 == set2;
}

/// The directory containing a file
QString parentDirectory(const QString& filePath);

} // namespace labelbuddy
#endif
