#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QPushButton>
#include <QSpinBox>

#include <string>

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    [[nodiscard]] std::string fontFamily() const;
    [[nodiscard]] int fontSize() const;
    [[nodiscard]] double lineSpacing() const;
    [[nodiscard]] bool rememberLastPassage() const;
    [[nodiscard]] bool clearHistoryRequested() const noexcept { return clearHistory_; }

    void setFontFamily(const std::string& family);
    void setFontSize(int size);
    void setLineSpacing(double spacing);
    void setRememberLastPassage(bool remember);

signals:
    void settingsChanged();

private:
    void setupUI();

    QFontComboBox* fontFamily_;
    QSpinBox* fontSize_;
    QDoubleSpinBox* lineSpacing_;
    QCheckBox* rememberLast_;
    QPushButton* clearHistoryBtn_;
    bool clearHistory_ = false;
};
