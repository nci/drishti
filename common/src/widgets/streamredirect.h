#ifndef QTSTREAMREDIRECT_H
#define QTSTREAMREDIRECT_H

#include <QTextEdit>
#include <QDebug>
#include <iostream>
#include <streambuf>
#include <sstream>

// Custom stream buffer to redirect std::cout / std::cerr
class QtStreamRedirect : public std::streambuf {
public:
    QtStreamRedirect(QTextEdit* textEdit, QColor color = Qt::black)
        : m_textEdit(textEdit), m_color(color) {}

    void flushBuffer() {
        if (!m_buffer.empty()) {
            appendToTextEdit(QString::fromStdString(m_buffer));
            m_buffer.clear();
        }
    }

    void appendToTextEdit(const QString& text) {
        if (m_textEdit) {
            m_textEdit->setTextColor(m_color);
            m_textEdit->append(text);
        }
    }

    void appendToTextEdit(const std::string &text) {
        if (m_textEdit) {
            m_textEdit->setTextColor(m_color);
            m_textEdit->append(QString::fromStdString(text));
        }
    }

protected:
    virtual int_type overflow(int_type v) override {
        if (v == '\n') {
            flushBuffer();
        } else {
            m_buffer += static_cast<char>(v);
        }
        return v;
    }

    virtual std::streamsize xsputn(const char* p, std::streamsize n) override {
        m_buffer.append(p, n);
        size_t pos = 0;
        while ((pos = m_buffer.find('\n')) != std::string::npos) {
            QString line = QString::fromStdString(m_buffer.substr(0, pos));
            appendToTextEdit(line);
            m_buffer.erase(0, pos + 1);
        }
        return n;
    }

private:
    QTextEdit* m_textEdit;
    QColor m_color;
    std::string m_buffer;
};


QTextEdit *g_logWidget = nullptr; // Global pointer to the QTextEdit for logging

// Qt message handler for qDebug, qWarning, etc.
void qtMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg) {
    QString prefix;
    QColor color = Qt::black;

    switch (type) {
        case QtDebugMsg:    prefix = "[DEBUG] "; color = Qt::darkGreen; break;
        case QtWarningMsg:  prefix = "[WARNING] "; color = Qt::darkYellow; break;
        case QtCriticalMsg: prefix = "[CRITICAL] "; color = Qt::red; break;
        case QtFatalMsg:    prefix = "[FATAL] "; color = Qt::red; break;
    }

    // Append to QTextEdit (global pointer)
    extern QTextEdit* g_logWidget;
    if (g_logWidget) {
        g_logWidget->setTextColor(color);
        g_logWidget->append(prefix + msg);
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

#endif // QTSTREAMREDIRECT_H
