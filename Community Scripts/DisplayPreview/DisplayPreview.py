#WINDOWS

import platform
import sys
import tkinter as tk
from tkinter import ttk, messagebox
import threading
import os
import subprocess
import time
import mss
import numpy as np
import cv2
from configparser import ConfigParser
from screeninfo import get_monitors
from PIL import Image, ImageTk

from audio_manager import AudioManager
from area_selector import AreaSelector
from logging_config import setup_logging

logger = setup_logging()

class ScreenRecorderApp:
    def __init__(self, root):
        self.root = root
        self.initialize_ffmpeg()
        logger.info("THE APP WAS OPEN")

        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.root.title("Display Preview")
        root.geometry('1280x720')
        self.config = ConfigParser()
        self.config_file = 'config.ini'
        self.load_config()

        self.audio_manager = AudioManager()
        self.audio_devices = self.audio_manager.audio_devices
        if len(self.audio_devices) == 0:
            messagebox.showerror("Error", "No audio devices.")
            return

        self.monitors = self.get_monitors()
        if len(self.monitors) == 0:
            messagebox.showerror("Error", "No monitors found.")
            return

        self.set_icon()

        self.init_ui()

        self.create_output_folder()
        self.recording_process = None
        self.running = False
        self.elapsed_time = 0
        self.record_area = None
        self.area_selector = AreaSelector(root)
        self.preview_running = False

        # Start preview by default
        self.preview_running = True
        threading.Thread(target=self._update_preview_thread, daemon=True).start()

        # Bind resizing event to dynamically adjust preview size
        self.root.bind("<Configure>", self.resize_preview)

    def set_icon(self):
        if platform.system() == 'Windows':
            self.root.iconbitmap('video.ico')
        elif platform.system() == 'Linux':
            icon = tk.PhotoImage(file='video.png')
            self.root.iconphoto(True, icon)

    def load_config(self):
        if os.path.exists(self.config_file):
            self.config.read(self.config_file)
        else:
            self.config['Settings'] = {'monitor': 0}
            with open(self.config_file, 'w') as configfile:
                self.config.write(configfile)

    def init_ui(self):
        self.monitor_label = ttk.Label(self.root, text="Monitor:")
        self.monitor_label.grid(row=0, column=0, padx=10, pady=5, sticky="e")
        self.monitor_combo = ttk.Combobox(self.root, values=[
            f"Monitor {i+1}: ({monitor.width}x{monitor.height})"
            for i, monitor in enumerate(self.monitors)
        ], width=25)
        self.monitor_combo.grid(row=0, column=1, padx=10, pady=5, sticky="w")
        self.monitor_combo.current(0)
        self.monitor_combo.config(state="readonly")

        self.preview_frame = ttk.Frame(self.root)
        self.preview_frame.grid(row=1, column=0, columnspan=2, pady=5, padx=10, sticky="nsew")
        self.preview_label = ttk.Label(self.preview_frame)
        self.preview_label.pack(fill=tk.BOTH, expand=True)

        # Configure resizing behavior
        self.root.grid_rowconfigure(1, weight=1)
        self.root.grid_columnconfigure(1, weight=1)

    def _update_preview_thread(self):
        with mss.mss() as sct:
            while self.preview_running:
                try:
                    monitor_index = self.monitor_combo.current()
                    if monitor_index < len(sct.monitors) - 1:
                        monitor = sct.monitors[monitor_index + 1]
                        screenshot = np.array(sct.grab(monitor))
                        screenshot = cv2.cvtColor(screenshot, cv2.COLOR_RGBA2RGB)
                        screenshot = cv2.cvtColor(screenshot, cv2.COLOR_BGR2RGB)

                        # Get current frame size
                        frame_width = self.preview_frame.winfo_width()
                        frame_height = self.preview_frame.winfo_height()

                        # Resize based on current window size
                        if frame_width > 0 and frame_height > 0:
                            screenshot = cv2.resize(screenshot, (frame_width, frame_height), interpolation=cv2.INTER_AREA)

                        tk_image = ImageTk.PhotoImage(image=Image.fromarray(screenshot))
                        self.root.after(0, self._update_preview_label, tk_image)
                    time.sleep(0.03)
                except tk.TclError:
                    break

    def _update_preview_label(self, tk_image):
        if self.preview_running:
            self.preview_label.config(image=tk_image)
            self.preview_label.image = tk_image

    def resize_preview(self, event):
        # Trigger a preview update on window resize
        if self.preview_running:
            self.preview_label.update_idletasks()

    def close_preview(self):
        self.preview_running = False
        self.preview_label.config(image='')
        self.preview_label.image = None

    def on_closing(self):
        self.close_preview()
        self.root.quit()
        self.root.destroy()

    def create_output_folder(self):
        self.output_folder = os.path.join(os.getcwd(), "OutputFiles")
        os.makedirs(self.output_folder, exist_ok=True)

    def get_monitors(self):
        return get_monitors()

    def initialize_ffmpeg(self):
        ffmpeg_path = self.get_ffmpeg_path()
        if not os.path.exists(ffmpeg_path):
            logger.error("FFmpeg not found.")
            messagebox.showerror("Error", "FFmpeg not found.")
            sys.exit(1)
        logger.info("FFmpeg was found.")

    def get_ffmpeg_path(self):
        base_path = os.path.dirname(os.path.abspath(__file__))
        if platform.system() == 'Windows':
            return os.path.join(base_path, 'ffmpeg_files', 'ffmpeg.exe')
        return os.path.join(base_path, 'ffmpeg_files', 'ffmpeg')


if __name__ == "__main__":
    try:
        root = tk.Tk()
        app = ScreenRecorderApp(root)
        root.mainloop()
    except Exception as e:
        logger.error(f"An error occurred: {e}")
        messagebox.showerror("Error", f"An unexpected error occurred: {e}")
