#pragma once

#include <QMainWindow>

class IBHM;
class Canvas;
class QComboBox;
class QLineEdit;
class QSpinBox;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

private:
    /**
     * Run the mesh optimizer.
     */
    void runSlot() const;

    /**
     * Select a file from a directory and open the mesh.
     */
    void openMesh();

private:
    IBHM *_ibhm = nullptr;
};
