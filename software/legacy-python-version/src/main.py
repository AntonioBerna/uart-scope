# ----------------------------------------------------------- #
# TODO: Use QSerialPort instead of pyserial                   #
# TODO: Re-implement all code in C++ using Qt and QSerialPort #
# ----------------------------------------------------------- #

import sys
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QVBoxLayout, QWidget, QHBoxLayout, QPushButton,
    QSlider, QLabel, QComboBox, QGridLayout, QStyleFactory, QMessageBox
)
from PyQt5.QtCore import QTimer, Qt

import serial.tools.list_ports

import numpy as np
import pyqtgraph as pg

CHANNELS = 4
BAUD_RATES = [9600, 115200] # ? Baud rate options
TIMEOUT = 1
MAX_PLOT_POINTS = 1000 # ? Number of points to display

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("UART Scope")
        self.setup_ui()
        self.setup_serial()
        self.setup_plots()
        self.is_acquiring = False
        self.is_paused = False
        self.pause_resume_button.setEnabled(False)
        self.clear_button.setEnabled(False)
    
    def update_scale_value(self, value):
        self.scale_x_value_label.setText(f"Current Scale: {value}")

    def auto_position(self):
        self.graphics_view.enableAutoRange(axis=pg.ViewBox.XYAxes)
    
    def start_serial_read(self):
        if self.serial_port and self.baud_rate:
            self.is_acquiring = True

    def pause_resume(self):
        self.is_paused = not self.is_paused
        if self.is_paused:
            self.pause_resume_button.setText("Resume")
            self.timer.stop()
            if self.serial_port:
                self.serial_port.close()
        else:
            self.pause_resume_button.setText("Pause")
            if self.serial_port and self.baud_rate:
                self.timer.start()
                self.start_serial_read()
                self.is_acquiring = True
            if self.serial_port:
                self.serial_port.open()
            self.serial_data = b''
    
    def clear_plot(self):
        self.plot_data = np.zeros((CHANNELS, MAX_PLOT_POINTS))
        current_plot_length = self.scale_x_slider.value()
        for i in range(CHANNELS):
            if self.channel_buttons[i].isChecked():
                self.plot_data_items[i].setData(self.x_data[-current_plot_length:], self.plot_data[i, -current_plot_length:])

    def toggle_channel(self, index, checked):
        # ? Change button color based on the channel state
        if checked:
            # ? Set button color to match the plot color
            colors = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0)]  # ? Colors for each channel
            self.channel_buttons[index].setStyleSheet(f"background-color: rgb{colors[index]}; color: white;")
            self.plot_data_items[index].setVisible(True)  # ? Show the plot for this channel
        else:
            # ? Change button back to black
            self.channel_buttons[index].setStyleSheet("background-color: rgb(0, 0, 0); color: white;")
            self.plot_data_items[index].setVisible(False)  # ? Hide the plot for this channel

    
    def select_baud_rate(self, index):
        baud_rate_str = self.baud_rates.itemText(index)
        try:
            self.baud_rate = int(baud_rate_str)
        except ValueError as e:
            print(f"Error converting baud rate: {e}")

        if self.serial_port and self.serial_port.is_open:
            self.serial_port.baudrate = self.baud_rate
            if not self.is_paused:
                self.timer.start()
    
    def select_serial_port(self, index):
        port_name = self.serial_ports.itemText(index)
        
        # ? Check if baud rate is selected
        if self.baud_rate is None:
            QMessageBox.warning(self, "Missing Baud Rate", "Please select a baud rate before selecting a serial port.")
            return
        
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        
        try:
            self.serial_port = serial.Serial(port_name, baudrate=self.baud_rate, timeout=TIMEOUT)
            self.start_button.setEnabled(True)
        except serial.SerialException as e:
            print(f"Error opening serial port: {e}")
    
    def start_acquisition(self):
        if self.serial_port is None or self.baud_rate is None:
            QMessageBox.warning(self, "Missing Serial Port or Baud Rate", "Please select both a serial port and a baud rate before starting acquisition.")
            return
        if not self.is_acquiring:
            try:
                if self.serial_port.isOpen():
                    self.serial_port.close()
                self.serial_port.open()
            except serial.SerialException as e:
                QMessageBox.critical(self, "Serial Port Error", f"Error opening serial port: {e}")
                self.stop_acquisition()
            else:
                self.timer.start()
                self.start_serial_read()
                self.is_acquiring = True
                
                self.pause_resume_button.setEnabled(True)
                self.clear_button.setEnabled(True)
        else:
            QMessageBox.warning(self, "Already Acquiring", "Data acquisition is already running.")
    
    def stop_acquisition(self):
        if self.serial_port:
            self.timer.stop()
            self.serial_port.close()
            self.is_acquiring = False
            self.pause_resume_button.setEnabled(False)
            self.clear_button.setEnabled(False)
            self.clear_plot()
    
    def apply_dark_mode(self):
        dark_palette = self.palette()
        dark_palette.setColor(dark_palette.Window, Qt.black)
        dark_palette.setColor(dark_palette.WindowText, Qt.white)
        dark_palette.setColor(dark_palette.Base, Qt.black)
        dark_palette.setColor(dark_palette.AlternateBase, Qt.black)
        dark_palette.setColor(dark_palette.ToolTipBase, Qt.white)
        dark_palette.setColor(dark_palette.ToolTipText, Qt.white)
        dark_palette.setColor(dark_palette.Text, Qt.white)
        dark_palette.setColor(dark_palette.Button, Qt.black)
        dark_palette.setColor(dark_palette.ButtonText, Qt.white)
        dark_palette.setColor(dark_palette.BrightText, Qt.red)
        dark_palette.setColor(dark_palette.Link, Qt.blue)
        dark_palette.setColor(dark_palette.Highlight, Qt.blue)
        dark_palette.setColor(dark_palette.HighlightedText, Qt.white)
        self.setPalette(dark_palette)
        QApplication.setStyle(QStyleFactory.create("Fusion"))
        QApplication.setPalette(dark_palette)
    
    def setup_ui(self):
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)

        # ? Layouts
        main_layout = QHBoxLayout(self.central_widget)
        left_layout = QVBoxLayout()
        main_layout.addLayout(left_layout)
        right_layout = QVBoxLayout()
        main_layout.addLayout(right_layout)

        # ? Plot widget
        self.graphics_view = pg.PlotWidget()
        left_layout.addWidget(self.graphics_view)

        # ? X-axis scaling controls
        self.scale_x_label = QLabel("Number of Points to Display:")
        left_layout.addWidget(self.scale_x_label)
        self.scale_x_slider = QSlider(Qt.Horizontal)
        self.scale_x_slider.setMinimum(10)
        self.scale_x_slider.setMaximum(MAX_PLOT_POINTS)
        self.scale_x_slider.setValue(MAX_PLOT_POINTS // 4)
        self.scale_x_slider.setTickPosition(QSlider.TicksBelow)
        self.scale_x_slider.setTickInterval(100)
        left_layout.addWidget(self.scale_x_slider)
        self.scale_x_value_label = QLabel(f"Current Scale: {MAX_PLOT_POINTS // 4}")
        left_layout.addWidget(self.scale_x_value_label)
        self.scale_x_slider.valueChanged.connect(self.update_scale_value)

        # ? Button layout
        buttons_layout = QVBoxLayout()
        self.auto_position_button = QPushButton("Auto Position")
        self.auto_position_button.clicked.connect(self.auto_position)
        buttons_layout.addWidget(self.auto_position_button)

        self.pause_resume_button = QPushButton("Pause")
        self.pause_resume_button.setCheckable(True)
        self.pause_resume_button.clicked.connect(self.pause_resume)
        buttons_layout.addWidget(self.pause_resume_button)

        self.clear_button = QPushButton("Clear")
        self.clear_button.clicked.connect(self.clear_plot)
        buttons_layout.addWidget(self.clear_button)

        # ? Create channel buttons with initial colors
        self.channel_buttons = []  # ? List to hold channel toggle buttons
        colors = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0)]  # ? Colors for each channel
        for i in range(CHANNELS):
            button = QPushButton(f"Channel {i + 1}")
            button.setCheckable(True)
            if i == 0:
                button.setStyleSheet("background-color: rgb(255, 0, 0); color: white;")  # ? First button is red
                button.setChecked(True)  # ? Start with the first channel checked
            else:
                button.setStyleSheet("background-color: rgb(0, 0, 0); color: white;")  # ? Others are black
                button.setChecked(False)
            button.clicked.connect(lambda checked, index=i: self.toggle_channel(index, checked))
            self.channel_buttons.append(button)
            buttons_layout.addWidget(button)

        right_layout.addLayout(buttons_layout)

        # ? Grid layout for baud rate, serial port, start, and stop buttons
        grid_layout = QGridLayout()
        right_layout.addLayout(grid_layout)

        # ? Baud rate ComboBox
        self.baud_rates = QComboBox()
        for baud in BAUD_RATES:
            self.baud_rates.addItem(str(baud))
        grid_layout.addWidget(self.baud_rates, 0, 0)
        self.baud_rates.currentIndexChanged.connect(self.select_baud_rate)

        # ? Serial port ComboBox
        self.serial_ports = QComboBox()
        ports = list(serial.tools.list_ports.comports())
        for port in ports:
            self.serial_ports.addItem(port.device)
        grid_layout.addWidget(self.serial_ports, 1, 0)
        self.serial_ports.currentIndexChanged.connect(self.select_serial_port)

        # ? Start button
        self.start_button = QPushButton("Start")
        self.start_button.clicked.connect(self.start_acquisition)
        self.start_button.setEnabled(False)  # ? Disable Start button initially
        grid_layout.addWidget(self.start_button, 2, 0)

        # ? Stop button
        self.stop_button = QPushButton("Stop")
        self.stop_button.clicked.connect(self.stop_acquisition)
        grid_layout.addWidget(self.stop_button, 3, 0)

        # ? Apply dark mode
        self.apply_dark_mode()

    def update_plot_data(self):
        current_plot_length = self.scale_x_slider.value()

        if np.any(self.plot_data):
            for i in range(CHANNELS):
                if self.channel_buttons[i].isChecked():
                    self.plot_data_items[i].setData(self.x_data[-current_plot_length:], self.plot_data[i, -current_plot_length:])
        else:
            for plot_item in self.plot_data_items:
                plot_item.clear()  # ? Clear plot data

        self.scale_x_value_label.setText(f"Current Scale: {current_plot_length}")
    
    def update_plot(self):
        if self.is_acquiring and not self.is_paused and self.serial_port:
            try:
                if self.serial_port.in_waiting > 0:
                    self.serial_data += self.serial_port.read_until(b'\n')
                    lines = self.serial_data.split(b'\n')
                    if len(lines) > 1:
                        for line in lines[:-1]:
                            if line.strip(): # ? Check if line is not empty
                                try:
                                    readings = list(map(int, line.decode().strip().split('\t')))
                                    if len(readings) == CHANNELS:
                                        self.plot_data[:, :-1] = self.plot_data[:, 1:]
                                        self.plot_data[:, -1] = readings
                                        self.update_plot_data()
                                except ValueError:
                                    print(f"Invalid data: {line}")
                    self.serial_data = lines[-1]
            except Exception as e:
                print(f"Error updating plot: {e}")

    def setup_serial(self):
        self.serial_port = None
        self.baud_rate = None
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_plot)
        self.serial_data = b''
    
    def setup_plots(self):
        self.plot_data_items = []
        self.plot_data = np.zeros((CHANNELS, MAX_PLOT_POINTS))
        self.x_data = np.arange(MAX_PLOT_POINTS)
        colors = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0)]
        for i in range(CHANNELS):
            color = colors[i % len(colors)]
            pen = pg.mkPen(color=color, width=2)
            plot_data_item = self.graphics_view.plot(pen=pen)
            plot_data_item.getViewBox().setAspectLocked(False) # ? Optionally, disable aspect ratio locking
            plot_data_item.getViewBox().setAutoVisible(y=True) # ? Optionally, enable auto-scaling vertically
            self.plot_data_items.append(plot_data_item)
        
        # ? Show grid
        self.graphics_view.showGrid(x=True, y=True)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    main_window = MainWindow()
    main_window.setGeometry(100, 100, 800, 600)
    main_window.show()
    sys.exit(app.exec_())
