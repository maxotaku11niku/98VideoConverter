// 98VideoConverter: Converts videos into .98v format, suitable for use with 98VIDEOP.COM
// Maxim Hoxha 2022-2023
// This is the main application window
// This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)

#include "MainWindow.h"

MainWindow::MainWindow() : Gtk::ApplicationWindow()
{
    set_title("98VideoConveter");
    set_default_size(1280, 600);
}

MainWindow::~MainWindow()
{
    
}