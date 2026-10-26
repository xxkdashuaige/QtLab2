
#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

class QWidget;
class QPaintEvent;
class QResizeEvent;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    // 计算左侧行号区域的宽度
    int lineNumberAreaWidth() const;

protected:
    // 调整窗口大小时同步调整行号区域
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // 根据当前可见文本块绘制行号
    void updateLineNumberArea(
        const QRect &rect, int dy);

    // 高亮当前行
    void highlightCurrentLine();

private:
    QWidget *lineNumberArea;

    // 绘制行号区域
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    friend class LineNumberArea;
};

#endif // CODEEDITOR_H
