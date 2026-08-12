#ifndef QTSTREAMREDIRECT_H
#define QTSTREAMREDIRECT_H

#include <QApplication>
#include <QTextEdit>
#include <QDebug>
#include <QMetaObject>
#include <QPointer>
#include <QThread>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <sstream>

// Custom stream buffer to redirect std::cout / std::cerr
class QtStreamRedirect : public std::streambuf {
public:

    static QTextEdit* logWidget()
    {
        if (!m_logWidget) {
            QApplication *application =
                qobject_cast<QApplication*>(QCoreApplication::instance());
            if (!application || QThread::currentThread() != application->thread())
                return nullptr;
            m_logWidget = new QTextEdit();
            m_logWidget->setReadOnly(true);
        }
        return m_logWidget.data();
    }

    
    // Qt message handler for qDebug, qWarning, etc.
    static void qtMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg) {
        QString prefix;
        QColor color = Qt::black;

        switch (type) {
            case QtDebugMsg:    prefix = "[DEBUG] "; color = Qt::darkGreen; break;
            case QtWarningMsg:  prefix = "[WARNING] "; color = Qt::darkYellow; break;
            case QtCriticalMsg: prefix = "[CRITICAL] "; color = Qt::red; break;
            case QtFatalMsg:    prefix = "[FATAL] "; color = Qt::red; break;
        }

        appendToLogWidget(prefix + msg, color);

        if (type == QtFatalMsg) {
            abort();
        }
    }

    
    QtStreamRedirect(QTextEdit* textEdit, QColor color = Qt::black)
        : m_textEdit(textEdit), m_color(color) {}

    ~QtStreamRedirect() override { sync(); }

    void flushBuffer() {
        std::string pending;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            pending.swap(m_buffer);
        }
        if (!pending.empty())
            appendToTextEdit(QString::fromStdString(pending));
    }

    void appendToTextEdit(const QString& text) {
        appendToWidget(m_textEdit, text, m_color);
    }

    void appendToTextEdit(const std::string &text) {
        appendToTextEdit(QString::fromStdString(text));
    }

protected:
    virtual int_type overflow(int_type v) override {
        if (traits_type::eq_int_type(v, traits_type::eof()))
            return traits_type::not_eof(v);

        std::string line;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (v == '\n')
                line.swap(m_buffer);
            else
                m_buffer += static_cast<char>(v);
        }
        if (!line.empty())
            appendToTextEdit(QString::fromStdString(line));
        return v;
    }

    virtual std::streamsize xsputn(const char* p, std::streamsize n) override {
        if (!p || n <= 0)
            return 0;

        QStringList lines;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_buffer.append(p, static_cast<std::size_t>(n));
            std::size_t pos = 0;
            while ((pos = m_buffer.find('\n')) != std::string::npos) {
                lines << QString::fromStdString(m_buffer.substr(0, pos));
                m_buffer.erase(0, pos + 1);
            }
        }
        for (const QString& line : lines)
            appendToTextEdit(line);
        return n;
    }

    int sync() override {
        flushBuffer();
        return 0;
    }

private:
    static void appendToWidget(QPointer<QTextEdit> guardedWidget,
                               const QString& text,
                               const QColor& color)
    {
        if (!guardedWidget)
            return;

        const auto append = [guardedWidget, text, color]() {
            if (!guardedWidget)
                return;
            guardedWidget->setTextColor(color);
            guardedWidget->append(text);
        };
        if (QThread::currentThread() == guardedWidget->thread())
            append();
        else
            QMetaObject::invokeMethod(guardedWidget.data(), append,
                                      Qt::QueuedConnection);
    }

    static void appendToLogWidget(const QString& text, const QColor& color)
    {
        appendToWidget(m_logWidget, text, color);
    }

    static QPointer<QTextEdit> m_logWidget;
    QPointer<QTextEdit> m_textEdit;
    QColor m_color;
    std::string m_buffer;
    std::mutex m_mutex;
};

QPointer<QTextEdit> QtStreamRedirect::m_logWidget;


#endif // QTSTREAMREDIRECT_H
