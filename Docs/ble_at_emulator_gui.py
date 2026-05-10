#!/usr/bin/env python3

from __future__ import annotations

import queue
import random
import re
import sys
import threading
import tkinter as tk
from tkinter import messagebox, scrolledtext, ttk

try:
    import serial
except ImportError:
    serial = None  

ADV_DATA_RE = re.compile(r'^AT\+BLEADVDATA="([0-9A-Fa-f]+)"\s*$')

APP_STATES = {1: "NORMAL", 2: "WARNING", 3: "ALARM", 4: "ERROR"}
APP_ERRS = {
    0: "NONE",
    1: "BLE_AT",
    2: "BLE_TIMEOUT",
    3: "TMP117_I2C",
    4: "CO_ADC",
}


def decode_adv_hex(hex_str: str) -> str | None:
    raw = bytes.fromhex(hex_str)
    if len(raw) < 17:
        return None
    if raw[0:3] != bytes([0x02, 0x01, 0x06]):
        return None
    if raw[3] != 0x0D or raw[4] != 0xFF:
        return None
    app_state = raw[7]
    err = raw[8]
    dev = int.from_bytes(raw[9:13], "little")
    co = int.from_bytes(raw[13:15], "little")
    temp_x10 = int.from_bytes(raw[15:17], "little")
    if temp_x10 >= 0x8000:
        temp_x10 -= 0x10000
    temp_c = temp_x10 / 10.0
    st = APP_STATES.get(app_state, f"?({app_state})")
    er = APP_ERRS.get(err, f"?({err})")
    return f"adv: state={st} err={er} dev_id=0x{dev:08X} co_ppm={co} temp={temp_c:.1f}C"


def pick_reply(mode: str, custom_body: str) -> bytes | None:
    mode = mode.strip().lower()
    if mode == "ok":
        return b"OK\r\n"
    if mode == "error":
        return b"ERROR\r\n"
    if mode == "silent":
        return None
    if mode == "custom":
        t = custom_body.strip()
        if not t:
            return b"OK\r\n"
        if not t.endswith("\n"):
            t += "\r\n"
        elif not t.endswith("\r\n"):
            t = t.replace("\n", "\r\n")
        return t.encode("utf-8", errors="replace")
    if mode == "random":
        r = random.choice(["ok", "error", "garbage"])
        if r == "ok":
            return b"OK\r\n"
        if r == "error":
            return b"ERROR\r\n"
        return (random.choice(["NAK\r\n", "BUSY\r\n", "+ERR:99\r\n", "???\r\n"])).encode("ascii")
    return b"OK\r\n"


class BleAtEmulatorGui:
    def __init__(self) -> None:
        self.root = tk.Tk()
        self.root.title("BLE AT bench (fire_sensor)")
        self.root.minsize(640, 480)

        self.ser: serial.Serial | None = None
        self.reader_stop = threading.Event()
        self.reader_thread: threading.Thread | None = None
        self.line_q: queue.Queue[str] = queue.Queue()

        top = ttk.Frame(self.root, padding=8)
        top.pack(fill=tk.X)

        ttk.Label(top, text="COM port:").pack(side=tk.LEFT)
        self.port_var = tk.StringVar(value="COM3")
        ttk.Entry(top, textvariable=self.port_var, width=10).pack(side=tk.LEFT, padx=(4, 12))

        self.btn_connect = ttk.Button(top, text="Connect", command=self.toggle_connect)
        self.btn_connect.pack(side=tk.LEFT)

        ttk.Label(top, text="Reply:").pack(side=tk.LEFT, padx=(16, 4))
        self.mode_var = tk.StringVar(value="ok")
        modes = ttk.Combobox(
            top,
            textvariable=self.mode_var,
            values=("ok", "error", "silent", "custom", "random"),
            width=10,
            state="readonly",
        )
        modes.pack(side=tk.LEFT)

        ttk.Label(top, text="OK / ERROR / silent").pack(side=tk.LEFT, padx=(12, 0))

        custom_frame = ttk.LabelFrame(self.root, text="Custom (mode=custom)")
        custom_frame.pack(fill=tk.X, padx=8, pady=(0, 4))
        self.custom_text = scrolledtext.ScrolledText(custom_frame, height=3, wrap=tk.WORD)
        self.custom_text.pack(fill=tk.X, padx=4, pady=4)
        self.custom_text.insert(tk.END, "ERROR")

        log_frame = ttk.LabelFrame(self.root, text="Log")
        log_frame.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)
        self.log = scrolledtext.ScrolledText(log_frame, height=20, state=tk.DISABLED, font=("Consolas", 9))
        self.log.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        bottom = ttk.Frame(self.root, padding=8)
        bottom.pack(fill=tk.X)
        ttk.Button(bottom, text="Clear log", command=self.clear_log).pack(side=tk.LEFT)

        self.root.after(40, self.poll_queues)

    def log_line(self, direction: str, text: str) -> None:
        self.log.configure(state=tk.NORMAL)
        self.log.insert(tk.END, f"{direction} {text}\n")
        self.log.see(tk.END)
        self.log.configure(state=tk.DISABLED)

    def clear_log(self) -> None:
        self.log.configure(state=tk.NORMAL)
        self.log.delete("1.0", tk.END)
        self.log.configure(state=tk.DISABLED)

    def poll_queues(self) -> None:
        try:
            while True:
                line = self.line_q.get_nowait()
                self.handle_line_main_thread(line)
        except queue.Empty:
            pass

        self.root.after(40, self.poll_queues)

    def handle_line_main_thread(self, line: str) -> None:
        self.log_line("<<", line)
        m = ADV_DATA_RE.match(line)
        if m:
            decoded = decode_adv_hex(m.group(1))
            if decoded:
                self.log_line("##", decoded)

        custom_body = self.custom_text.get("1.0", tk.END)
        reply = pick_reply(self.mode_var.get(), custom_body)
        if self.ser is None:
            return
        if reply is None:
            self.log_line(">>", "(silent)")
            return
        try:
            self.ser.write(reply)
        except Exception as e:
            self.log_line(">>", f"(write failed: {e})")
            return
        shown = reply.decode("utf-8", errors="replace").strip()
        self.log_line(">>", shown)

    def toggle_connect(self) -> None:
        if serial is None:
            messagebox.showerror("pyserial", "Install: python -m pip install pyserial")
            return
        if self.ser is not None:
            self.disconnect()
            return
        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning("COM", "Enter COM port (e.g. COM5)")
            return
        try:
            self.ser = serial.Serial(port, baudrate=115200, timeout=0.05)
        except serial.SerialException as e:
            messagebox.showerror("Serial", str(e))
            return
        self.reader_stop.clear()
        self.reader_thread = threading.Thread(target=self.reader_loop, daemon=True)
        self.reader_thread.start()
        self.btn_connect.configure(text="Disconnect")
        self.log_line("**", f"Connected {port} 115200 mode={self.mode_var.get()}")

    def disconnect(self) -> None:
        self.reader_stop.set()
        if self.reader_thread is not None:
            self.reader_thread.join(timeout=1.0)
            self.reader_thread = None
        if self.ser is not None:
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None
        self.btn_connect.configure(text="Connect")
        self.log_line("**", "Disconnected")

    def reader_loop(self) -> None:
        assert self.ser is not None
        buf = bytearray()
        while not self.reader_stop.is_set():
            try:
                chunk = self.ser.read(256)
            except Exception:
                break
            if chunk:
                buf.extend(chunk)
            while True:
                nl = buf.find(b"\n")
                if nl < 0:
                    break
                line_bytes = bytes(buf[: nl + 1])
                del buf[: nl + 1]
                line = line_bytes.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                self.line_q.put(line)

    def run(self) -> None:
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)
        self.root.mainloop()

    def on_close(self) -> None:
        self.disconnect()
        self.root.destroy()


def main() -> int:
    if serial is None:
        print("Install pyserial:  python -m pip install pyserial", file=sys.stderr)
        return 1
    BleAtEmulatorGui().run()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
