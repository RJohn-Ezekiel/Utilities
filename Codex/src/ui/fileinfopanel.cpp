#include "ui/fileinfopanel.h"
#include "ui/style.h"

#include <QHBoxLayout>

namespace codex {

FileInfoPanel::FileInfoPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(12);

    m_fileLabel = new QLabel(this);
    m_posLabel = new QLabel(this);
    m_wordCountLabel = new QLabel(this);

    auto color = QString::fromLatin1(style::TEXT_SECONDARY);
    auto styleSheet = QStringLiteral("color: %1;").arg(color);
    m_fileLabel->setStyleSheet(styleSheet);
    m_posLabel->setStyleSheet(styleSheet);
    m_wordCountLabel->setStyleSheet(styleSheet);

    layout->addWidget(m_fileLabel);
    layout->addStretch();
    layout->addWidget(m_posLabel);
    layout->addWidget(m_wordCountLabel);
}

void FileInfoPanel::update(int line, int col, int words, int chars, const std::string &filename)
{
    m_fileLabel->setText(QString::fromStdString(filename));
    m_posLabel->setText(QStringLiteral("Ln: %1, Col: %2").arg(line).arg(col));
    m_wordCountLabel->setText(QStringLiteral("Words: %1  |  Chars: %2").arg(words).arg(chars));
}

void FileInfoPanel::clear()
{
    m_fileLabel->clear();
    m_posLabel->clear();
    m_wordCountLabel->clear();
}

} // namespace codex
