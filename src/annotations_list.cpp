#include <cassert>

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QItemSelectionModel>
#include <QLabel>
#include <QListView>
#include <QRect>
#include <QSortFilterProxyModel>
#include <QStyle>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>

#include "annotations_list.h"
#include "user_roles.h"

namespace labelbuddy {

PainterRestore::PainterRestore(QPainter* painter) : painter_{painter} {
  if (painter_ != nullptr) {
    painter->save();
  }
}

PainterRestore::~PainterRestore() {
  if (painter_ != nullptr) {
    painter_->restore();
  }
}

AnnotationDelegate::AnnotationDelegate(QObject* parent)
    : QStyledItemDelegate{parent} {
  QFontMetrics metrics{QFont{}};
  em_ = metrics.height();
}

static const QString annotationItemTemplate = R"(
<div style='font-size:medium;'>
<h3 style='margin:0;margin-bottom:%7;font-size:medium;
  color:black;background-color:%2'>%1
&nbsp;&nbsp;
<span style='font-size:medium;font-style:italic;font-weight:normal;'>%3</span>
</h3>
<p style='margin:0;'>
%4<span style='color:black;background-color:%2;'>%5</span>%6
</p>
</div>
)";

static const QString selectedAnnotationItemTemplate = R"(
<div style='font-size:medium;color:black:background-color:%2;'>
<h3 style='margin:0;margin-bottom:%7;font-size:large;
  color:black;background-color:%2'>%1
&nbsp;&nbsp;
<span style='font-size:medium;font-style:italic;font-weight:normal;'>%3</span>
</h3>
<p style='margin:0;'>
%4<span style='background-color:%8;color:%9;
  font-size:large;font-weight:bold;'>%5</span>%6
</p>
</div>
)";

QString AnnotationDelegate::prepareItemHtml(const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const {
  auto labelName = index.data(Roles::LabelNameRole).value<QString>();
  auto labelColor = index.data(Qt::BackgroundRole).value<QColor>().name();
  auto selectedText = index.data(Roles::SelectedTextRole).value<QString>();
  auto prefix = index.data(Roles::AnnotationPrefixRole).value<QString>();
  auto suffix = index.data(Roles::AnnotationSuffixRole).value<QString>();
  auto extraData = index.data(Roles::AnnotationExtraDataRole).value<QString>();
  auto margin = QString::number(.3 * em_);
  auto baseColor = option.palette.base().color().name();
  auto textColor = option.palette.text().color().name();
  auto itemTemplate = annotationItemTemplate;
  auto isSelected = option.state & QStyle::State_Selected;
  if (isSelected) {
    margin = QString::number(.15 * em_);
    itemTemplate = selectedAnnotationItemTemplate;
  }
  auto html = itemTemplate.arg(
      /*1*/ labelName.toHtmlEscaped(), /*2*/ labelColor,
      /*3*/ extraData.toHtmlEscaped(), /*4*/ prefix.toHtmlEscaped(),
      /*5*/ selectedText.toHtmlEscaped(), /*6*/ suffix.toHtmlEscaped(),
      /*7*/ margin);
  if (isSelected) {
    html = html.arg(/*8*/ baseColor, /*9*/ textColor);
  }
  return html;
}

void AnnotationDelegate::paint(QPainter* painter,
                               const QStyleOptionViewItem& option,
                               const QModelIndex& index) const {
  if (option.state & QStyle::State_Selected) {
    auto labelColor = index.data(Qt::BackgroundRole).value<QColor>().name();
    PainterRestore restore{painter};
    painter->fillRect(option.rect, labelColor);
    painter->setPen(QColor{0, 0, 0, 70});
    painter->drawLine(option.rect.topLeft(), option.rect.topRight());
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
  }
  QTextDocument document{};
  document.setHtml(prepareItemHtml(option, index));
  {
    PainterRestore restore{painter};
    painter->translate(option.rect.left(), option.rect.top());
    QRect clip(0, 0, option.rect.width(), option.rect.height());
    document.drawContents(painter, clip);
  }
}

QSize AnnotationDelegate::sizeHint(const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const {
  auto defaultSize = QStyledItemDelegate::sizeHint(option, index);
  return QSize{defaultSize.width(), int(3.5 * em_)};
}

AnnotationsList::AnnotationsList(QWidget* parent) : QFrame(parent) {
  auto layout = new QVBoxLayout();
  setLayout(layout);
  auto title = new QLabel{"Annotations in this document:"};
  layout->addWidget(title);
  title->setWordWrap(true);
  annotationsView_ = new QListView{};
  layout->addWidget(annotationsView_);
  annotationsView_->setFocusPolicy(Qt::NoFocus);
  auto delegate = new AnnotationDelegate{this};
  annotationsView_->setItemDelegate(delegate);
  annotationsView_->viewport()->installEventFilter(this);
}

void AnnotationsList::setModel(AnnotationsModel* model) {
  assert(model != nullptr);
  annotationsModel_ = model;
  annotationsListModel_.reset(new AnnotationsListModel{});
  annotationsListModel_->setSourceModel(model);
  proxyModel_.reset(new QSortFilterProxyModel{});
  proxyModel_->setSourceModel(annotationsListModel_.get());
  proxyModel_->setSortRole(Roles::AnnotationStartCharRole);
  proxyModel_->sort(0);
  annotationsView_->setModel(proxyModel_.get());
  QObject::connect(annotationsView_->selectionModel(),
                   &QItemSelectionModel::selectionChanged, this,
                   &AnnotationsList::onSelectionChange);
  // prevent automatically selecting another annotation when the current one is
  // deleted
  QObject::connect(
      annotationsModel_, &AnnotationsModel::aboutToDeleteAnnotation,
      annotationsView_->selectionModel(), &QItemSelectionModel::clear);
}

void AnnotationsList::selectAnnotation(int annotationId) {
  if (annotationId == -1) {
    annotationsView_->selectionModel()->clearCurrentIndex();
    annotationsView_->selectionModel()->clearSelection();
    return;
  }
  auto modelIndex = annotationsListModel_->indexForAnnotationId(annotationId);
  auto sortedIndex = proxyModel_->mapFromSource(modelIndex);
  annotationsView_->selectionModel()->setCurrentIndex(
      sortedIndex, QItemSelectionModel::Clear | QItemSelectionModel::Select |
                       QItemSelectionModel::Current);
}

void AnnotationsList::resetAnnotations() {
  annotationsListModel_->resetAnnotations();
}

bool AnnotationsList::eventFilter(QObject* object, QEvent* event) {
  if (object == annotationsView_->viewport() &&
      event->type() == QEvent::MouseButtonPress) {
    emit clicked();
  }
  return false;
}

void AnnotationsList::onSelectionChange(const QItemSelection& selected) {
  if (!selected.indexes().size()) {
    return;
  }
  auto annotationId =
      selected.indexes()[0].data(Roles::AnnotationIdRole).value<int>();
  emit selectedAnnotationIdChanged(annotationId);
}

} // namespace labelbuddy
