import sys
from PyQt6.QtWidgets import QApplication
from gui.main_window import MainWindow

def main():
    # High DPI scaling
    if hasattr(sys, 'set_int_max_str_digits'):
        sys.set_int_max_str_digits(0) 
        
    app = QApplication(sys.argv)
    
    window = MainWindow()
    window.show()
    
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
