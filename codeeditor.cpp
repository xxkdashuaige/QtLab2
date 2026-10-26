
#include "codeeditor.h"

#include <QPainter>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QColor>
#include <QTextEdit>
#include <QTextFormat>
#include <QSize>

// 左侧行号区域
class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor)
        : QWidget(editor), codeEditor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    // 文本块数量变化时，调整行号区域宽度
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, [this]() {
                setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
            });

    // 文本滚动时同步更新行号
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);

    // 光标移动时高亮当前行
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int maxValue = qMax(1, blockCount());

    while (maxValue >= 10) {
        maxValue /= 10;
        ++digits;
    }

    return 12 + fontMetrics().horizontalAdvance(
                    QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberArea(
    const QRect &rect, int dy)
{
    if (dy != 0) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(
            0, rect.y(),
            lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();

    lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(),
              lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(
    QPaintEvent *event)
{
    QPainter painter(lineNumberArea);

    // 行号区域背景
    painter.fillRect(event->rect(), QColor("#F0F0F0"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();

    int top = qRound(
        blockBoundingGeometry(block)
            .translated(contentOffset()).top());

    int bottom = top
                 + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible()
            && bottom >= event->rect().top()) {

            QString number =
                QString::number(blockNumber + 1);

            painter.setPen(QColor("#606060"));

            painter.drawText(
                0, top,
                lineNumberArea->width() - 5,
                fontMetrics().height(),
                Qt::AlignRight | Qt::AlignVCenter,
                number);
        }

        block = block.next();
        top = bottom;

        bottom = top
                 + qRound(blockBoundingRect(block).height());

        ++blockNumber;
    }
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        selection.format.setBackground(
            QColor("#EAF2FF"));

        selection.format.setProperty(
            QTextFormat::FullWidthSelection, true);

        selection.cursor = textCursor();
        selection.cursor.clearSelection();

        selections.append(selection);
    }

    setExtraSelections(selections);
}
