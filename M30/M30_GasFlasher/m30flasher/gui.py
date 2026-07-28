"""Tkinter desktop UI for choosing an M30 gas and flashing it to an ESP32."""

from __future__ import annotations

import queue
import subprocess
import sys
import threading
import webbrowser
from pathlib import Path
from tkinter import BOTH, END, LEFT, RIGHT, X, Y, StringVar, Tk, filedialog, messagebox
from tkinter import ttk
import tkinter as tk

from .flashjob import FlashJob, JobCancelled, Paths, has_artifacts
from .profiles import GasProfile
from .settings import BOARD_PRESETS, Settings
from .sketch import Sketch, SketchError, discover_sketches
from .toolchain import CommandRunner, SerialPort, ToolchainError, find_arduino_cli, list_ports

APP_TITLE = "M30 Gas Flasher"
MAX_LOG_LINES = 5000


def default_search_roots() -> list[Path]:
    """Where to look for M30 sketches on first run."""
    here = Path(getattr(sys, "_MEIPASS", Path(__file__).resolve().parent.parent))
    app_dir = Path(sys.executable).parent if getattr(sys, "frozen", False) else here
    roots = [app_dir.parent, app_dir.parent.parent, app_dir, Path.cwd()]
    seen: list[Path] = []
    for root in roots:
        resolved = root.resolve()
        if resolved.is_dir() and resolved not in seen:
            seen.append(resolved)
    return seen


def _attach_scrollbars(parent: ttk.Frame, text: tk.Text) -> None:
    """Put ``text`` in ``parent`` with a vertical and a horizontal scrollbar."""
    vertical = ttk.Scrollbar(parent, orient=tk.VERTICAL, command=text.yview)
    horizontal = ttk.Scrollbar(parent, orient=tk.HORIZONTAL, command=text.xview)
    text.configure(yscrollcommand=vertical.set, xscrollcommand=horizontal.set)
    horizontal.pack(side=tk.BOTTOM, fill=X)
    vertical.pack(side=RIGHT, fill=Y)
    text.pack(side=LEFT, fill=BOTH, expand=True)


class FlasherApp(ttk.Frame):
    def __init__(self, master: Tk):
        super().__init__(master, padding=8)
        self.master: Tk = master
        self.settings = Settings.load()
        self.messages: queue.Queue[tuple[str, str]] = queue.Queue()
        self.worker: threading.Thread | None = None
        self.job: FlashJob | None = None
        self.sketches: list[Sketch] = []
        self.profiles: list[GasProfile] = []
        self.visible_profiles: list[GasProfile] = []
        self.ports: list[SerialPort] = []
        self.active_include: str | None = None

        self.sketch_var = StringVar()
        self.port_var = StringVar(value=self.settings.port)
        self.board_var = StringVar()
        self.fqbn_var = StringVar(value=self.settings.fqbn)
        self.unit_var = StringVar(value=self.settings.unit_id)
        self.filter_var = StringVar()
        self.status_var = StringVar(value="Ready")
        self.active_var = StringVar(value="")

        self._build_menu()
        self._build_widgets()
        self.pack(fill=BOTH, expand=True)

        self.master.protocol("WM_DELETE_WINDOW", self._on_close)
        self.after(100, self._pump)
        self.refresh_sketches()
        self.refresh_ports()
        self._report_cli()

    # -- construction -------------------------------------------------------

    def _build_menu(self) -> None:
        menubar = tk.Menu(self.master)

        file_menu = tk.Menu(menubar, tearoff=0)
        file_menu.add_command(label="Choose firmware folder...", command=self.browse_sketch)
        file_menu.add_command(label="Rescan for firmware versions", command=self.refresh_sketches)
        file_menu.add_separator()
        file_menu.add_command(label="Open work folder", command=self.open_work_folder)
        file_menu.add_command(label="Open flash log (CSV)", command=self.open_flash_log)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self._on_close)
        menubar.add_cascade(label="File", menu=file_menu)

        tools_menu = tk.Menu(menubar, tearoff=0)
        tools_menu.add_command(label="Install / update ESP32 toolchain", command=self.setup_toolchain)
        tools_menu.add_command(label="Locate arduino-cli...", command=self.locate_cli)
        tools_menu.add_separator()
        self.templates_var = tk.BooleanVar(value=self.settings.show_templates)
        tools_menu.add_checkbutton(
            label="Show template profiles",
            variable=self.templates_var,
            command=self.reload_profiles,
        )
        menubar.add_cascade(label="Tools", menu=tools_menu)

        help_menu = tk.Menu(menubar, tearoff=0)
        help_menu.add_command(label="How this works", command=self.show_help)
        help_menu.add_command(label="Get arduino-cli", command=lambda: webbrowser.open(
            "https://arduino.github.io/arduino-cli/latest/installation/"
        ))
        menubar.add_cascade(label="Help", menu=help_menu)

        self.master.config(menu=menubar)

    def _build_widgets(self) -> None:
        source = ttk.LabelFrame(self, text="1. Firmware source", padding=6)
        source.pack(fill=X)
        source.columnconfigure(1, weight=1)
        ttk.Label(source, text="Version:").grid(row=0, column=0, sticky="w", padx=(0, 6))
        self.sketch_combo = ttk.Combobox(source, textvariable=self.sketch_var, state="readonly")
        self.sketch_combo.grid(row=0, column=1, sticky="ew")
        self.sketch_combo.bind("<<ComboboxSelected>>", lambda _e: self.reload_profiles())
        ttk.Button(source, text="Browse...", command=self.browse_sketch, width=10).grid(
            row=0, column=2, padx=(6, 0)
        )
        ttk.Label(source, textvariable=self.active_var, foreground="#444").grid(
            row=1, column=0, columnspan=3, sticky="w", pady=(4, 0)
        )

        gases = ttk.LabelFrame(self, text="2. Gas to flash", padding=6)
        gases.pack(fill=BOTH, expand=True, pady=(8, 0))

        panes = ttk.PanedWindow(gases, orient=tk.HORIZONTAL)
        panes.pack(fill=BOTH, expand=True)

        left = ttk.Frame(panes)
        filter_row = ttk.Frame(left)
        filter_row.pack(fill=X, pady=(0, 4))
        ttk.Label(filter_row, text="Find:").pack(side=LEFT, padx=(0, 4))
        filter_entry = ttk.Entry(filter_row, textvariable=self.filter_var)
        filter_entry.pack(side=LEFT, fill=X, expand=True)
        self.filter_var.trace_add("write", lambda *_: self._populate_tree())

        tree_holder = ttk.Frame(left)
        tree_holder.pack(fill=BOTH, expand=True)
        columns = ("gas", "range", "type", "profile")
        self.tree = ttk.Treeview(tree_holder, columns=columns, show="headings", selectmode="browse")
        for column, heading, width in (
            ("gas", "Gas", 125),
            ("range", "Range", 100),
            ("type", "Type", 50),
            ("profile", "Profile file", 165),
        ):
            self.tree.heading(column, text=heading)
            self.tree.column(column, width=width, anchor="w", stretch=(column == "profile"))
        tree_scroll = ttk.Scrollbar(tree_holder, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=tree_scroll.set)
        tree_scroll.pack(side=RIGHT, fill=Y)
        self.tree.pack(side=LEFT, fill=BOTH, expand=True)
        self.tree.bind("<<TreeviewSelect>>", lambda _e: self._on_profile_selected())
        self.tree.tag_configure("active", background="#e8f4ff")
        panes.add(left, weight=3)

        right = ttk.Frame(panes)
        self.details = tk.Text(right, width=54, height=16, wrap="none", state="disabled")
        self.details.configure(font=("Consolas", 9))
        _attach_scrollbars(right, self.details)
        panes.add(right, weight=2)

        target = ttk.LabelFrame(self, text="3. Target unit", padding=6)
        target.pack(fill=X, pady=(8, 0))
        target.columnconfigure(1, weight=1)
        target.columnconfigure(4, weight=1)

        ttk.Label(target, text="COM port:").grid(row=0, column=0, sticky="w", padx=(0, 6))
        self.port_combo = ttk.Combobox(target, textvariable=self.port_var, state="readonly", width=34)
        self.port_combo.grid(row=0, column=1, sticky="ew")
        ttk.Button(target, text="Refresh", command=self.refresh_ports, width=10).grid(
            row=0, column=2, padx=6
        )
        ttk.Label(target, text="Unit ID:").grid(row=0, column=3, sticky="w", padx=(12, 6))
        ttk.Entry(target, textvariable=self.unit_var).grid(row=0, column=4, sticky="ew")

        ttk.Label(target, text="Board:").grid(row=1, column=0, sticky="w", padx=(0, 6), pady=(6, 0))
        self.board_combo = ttk.Combobox(
            target, textvariable=self.board_var, state="readonly", values=list(BOARD_PRESETS)
        )
        self.board_combo.grid(row=1, column=1, columnspan=2, sticky="ew", pady=(6, 0))
        self.board_combo.bind("<<ComboboxSelected>>", lambda _e: self._on_board_selected())
        ttk.Label(target, text="FQBN:").grid(row=1, column=3, sticky="w", padx=(12, 6), pady=(6, 0))
        ttk.Entry(target, textvariable=self.fqbn_var).grid(row=1, column=4, sticky="ew", pady=(6, 0))
        self._sync_board_combo()

        actions = ttk.Frame(self)
        actions.pack(fill=X, pady=(8, 0))
        self.build_upload_btn = ttk.Button(
            actions, text="Build & Upload", command=self.build_and_upload
        )
        self.build_upload_btn.pack(side=LEFT)
        self.upload_btn = ttk.Button(actions, text="Upload (no rebuild)", command=self.upload_only)
        self.upload_btn.pack(side=LEFT, padx=6)
        self.build_btn = ttk.Button(actions, text="Build only", command=self.build_only)
        self.build_btn.pack(side=LEFT)
        self.stop_btn = ttk.Button(actions, text="Stop", command=self.stop, state="disabled")
        self.stop_btn.pack(side=LEFT, padx=6)
        self.progress = ttk.Progressbar(actions, mode="indeterminate", length=180)
        self.progress.pack(side=RIGHT)
        ttk.Label(actions, textvariable=self.status_var).pack(side=RIGHT, padx=8)

        # Packed bottom-up so the buttons keep their space when the window shrinks.
        log_buttons = ttk.Frame(self)
        log_buttons.pack(side=tk.BOTTOM, fill=X, pady=(4, 0))
        ttk.Button(log_buttons, text="Clear", command=self.clear_log, width=10).pack(side=LEFT)
        ttk.Button(log_buttons, text="Save log...", command=self.save_log, width=12).pack(
            side=LEFT, padx=6
        )

        log_frame = ttk.LabelFrame(self, text="Output", padding=6)
        log_frame.pack(side=tk.BOTTOM, fill=BOTH, expand=True, pady=(8, 0))
        self.log_text = tk.Text(log_frame, height=10, wrap="none", state="disabled")
        self.log_text.configure(font=("Consolas", 9))
        _attach_scrollbars(log_frame, self.log_text)
        for tag, colour in (("error", "#b00020"), ("ok", "#0a7d28"), ("step", "#0b4f9e")):
            self.log_text.tag_configure(tag, foreground=colour)

    # -- data loading -------------------------------------------------------

    def refresh_sketches(self) -> None:
        roots = default_search_roots()
        configured = Path(self.settings.sketch_dir) if self.settings.sketch_dir else None
        if configured and configured.is_dir():
            roots.insert(0, configured.parent)
            roots.insert(0, configured)
        self.sketches = discover_sketches(roots)
        labels = [s.name for s in self.sketches]
        self.sketch_combo["values"] = labels
        if not self.sketches:
            self.sketch_var.set("")
            self.log_line(
                "No M30 firmware folder found. Use File > Choose firmware folder...", "error"
            )
            self.profiles = []
            self._populate_tree()
            return
        wanted = Path(self.settings.sketch_dir).name if self.settings.sketch_dir else ""
        self.sketch_var.set(wanted if wanted in labels else labels[0])
        self.reload_profiles()

    def current_sketch(self) -> Sketch | None:
        name = self.sketch_var.get()
        for sketch in self.sketches:
            if sketch.name == name:
                return sketch
        return None

    def reload_profiles(self) -> None:
        sketch = self.current_sketch()
        if not sketch:
            return
        self.settings.sketch_dir = str(sketch.root)
        self.settings.show_templates = bool(self.templates_var.get())
        try:
            self.profiles = sketch.profiles(include_templates=self.settings.show_templates)
        except (SketchError, OSError) as exc:
            self.profiles = []
            self.log_line(f"Could not read gas profiles: {exc}", "error")
        version = sketch.firmware_version()
        self.active_include = sketch.active_include()
        self.active_var.set(
            f"{sketch.root}    |    {version or 'unknown FW version'}    |    "
            f"currently selected in source: "
            f"{self.active_include or '(none / more than one selected)'}"
        )
        self._populate_tree()

    def _populate_tree(self) -> None:
        needle = self.filter_var.get().strip().lower()
        active = self.active_include
        previous = self.settings.last_profile

        self.tree.delete(*self.tree.get_children())
        self.visible_profiles = []
        for profile in self.profiles:
            haystack = f"{profile.gas} {profile.key} {profile.title} {profile.unit}".lower()
            if needle and needle not in haystack:
                continue
            self.visible_profiles.append(profile)
            tags = ("active",) if profile.include_path == active else ()
            self.tree.insert(
                "",
                END,
                iid=profile.key,
                values=(profile.display_gas, profile.range_label, profile.family, profile.key),
                tags=tags,
            )

        # Preselect: what the operator used last, else whatever the source tree
        # is currently set to, else the first row.
        rows = set(self.tree.get_children())
        active_key = next(
            (p.key for p in self.visible_profiles if p.include_path == active), None
        )
        target = next(
            (key for key in (previous, active_key) if key and key in rows),
            self.visible_profiles[0].key if self.visible_profiles else None,
        )
        if target:
            self.tree.selection_set(target)
            self.tree.see(target)
        else:
            self._set_details("")

    def selected_profile(self) -> GasProfile | None:
        selection = self.tree.selection()
        if not selection:
            return None
        for profile in self.visible_profiles:
            if profile.key == selection[0]:
                return profile
        return None

    def _on_profile_selected(self) -> None:
        profile = self.selected_profile()
        if not profile:
            return
        self.settings.last_profile = profile.key
        text = profile.details()
        cached = self._cached_artifacts(profile)
        if cached:
            text += f"\n\nBuilt firmware cached — 'Upload (no rebuild)' will flash\n{cached}"
        self._set_details(text)

    def _cached_artifacts(self, profile: GasProfile) -> Path | None:
        """Artifact dir for ``profile`` if a build of this exact source exists."""
        sketch = self.current_sketch()
        if not sketch:
            return None
        try:
            directory = Paths(self.settings.work_path).artifacts(
                sketch.name, profile.key, sketch.fingerprint(profile.include_path)
            )
        except (SketchError, OSError, ValueError):
            return None
        return directory if has_artifacts(directory) else None

    def _set_details(self, text: str) -> None:
        self.details.configure(state="normal")
        self.details.delete("1.0", END)
        self.details.insert(END, text)
        self.details.configure(state="disabled")

    def refresh_ports(self) -> None:
        self.ports = list_ports()
        labels = [p.label for p in self.ports]
        self.port_combo["values"] = labels
        current = SerialPort.device_of(self.port_var.get())
        match = next((l for l in labels if SerialPort.device_of(l) == current), None)
        self.port_var.set(match or (labels[0] if labels else ""))
        if not labels:
            self.log_line("No serial ports found. Check the USB cable and CP210x/CH340 driver.", "error")

    def _sync_board_combo(self) -> None:
        for label, fqbn in BOARD_PRESETS.items():
            if fqbn == self.fqbn_var.get():
                self.board_var.set(label)
                return
        self.board_var.set("")

    def _on_board_selected(self) -> None:
        fqbn = BOARD_PRESETS.get(self.board_var.get())
        if fqbn:
            self.fqbn_var.set(fqbn)

    # -- logging ------------------------------------------------------------

    def log_line(self, text: str, tag: str = "") -> None:
        self.log_text.configure(state="normal")
        self.log_text.insert(END, text + "\n", tag or ())
        overflow = int(self.log_text.index("end-1c").split(".")[0]) - MAX_LOG_LINES
        if overflow > 0:
            self.log_text.delete("1.0", f"{overflow}.0")
        self.log_text.see(END)
        self.log_text.configure(state="disabled")

    def clear_log(self) -> None:
        self.log_text.configure(state="normal")
        self.log_text.delete("1.0", END)
        self.log_text.configure(state="disabled")

    def save_log(self) -> None:
        path = filedialog.asksaveasfilename(
            defaultextension=".txt", filetypes=[("Text file", "*.txt"), ("All files", "*.*")]
        )
        if not path:
            return
        Path(path).write_text(self.log_text.get("1.0", END), encoding="utf-8")
        self.status_var.set(f"Log saved to {Path(path).name}")

    def _pump(self) -> None:
        """Drain worker messages onto the UI thread."""
        try:
            while True:
                kind, payload = self.messages.get_nowait()
                if kind == "log":
                    self.log_line(payload)
                elif kind == "step":
                    self.status_var.set(payload)
                    self.log_line(f"--- {payload} ---", "step")
                elif kind == "error":
                    self.log_line(payload, "error")
                    self.status_var.set("Failed")
                elif kind == "ok":
                    self.log_line(payload, "ok")
                    self.status_var.set(payload)
                elif kind == "done":
                    self._set_busy(False)
                    self.reload_profiles()
        except queue.Empty:
            pass
        self.after(100, self._pump)

    # -- actions ------------------------------------------------------------

    def _report_cli(self) -> None:
        cli = find_arduino_cli(self.settings.arduino_cli)
        if cli:
            self.log_line(f"arduino-cli: {cli}")
        else:
            self.log_line(
                "arduino-cli not found. Install it (Help > Get arduino-cli), drop "
                "arduino-cli.exe in the tools\\ folder next to this program, or point "
                "at it with Tools > Locate arduino-cli.",
                "error",
            )

    def locate_cli(self) -> None:
        path = filedialog.askopenfilename(
            title="Select arduino-cli",
            filetypes=[("arduino-cli", "arduino-cli.exe"), ("All files", "*.*")],
        )
        if not path:
            return
        self.settings.arduino_cli = path
        self.settings.save()
        self._report_cli()

    def browse_sketch(self) -> None:
        path = filedialog.askdirectory(title="Select the M30 firmware folder")
        if not path:
            return
        sketch = Sketch(Path(path))
        try:
            sketch.validate()
        except SketchError as exc:
            messagebox.showerror(APP_TITLE, str(exc))
            return
        self.settings.sketch_dir = str(sketch.root)
        self.settings.save()
        self.refresh_sketches()

    def _set_busy(self, busy: bool) -> None:
        state = "disabled" if busy else "normal"
        for widget in (self.build_upload_btn, self.upload_btn, self.build_btn):
            widget.configure(state=state)
        self.stop_btn.configure(state="normal" if busy else "disabled")
        if busy:
            self.progress.start(12)
        else:
            self.progress.stop()

    def _capture_settings(self) -> None:
        self.settings.fqbn = self.fqbn_var.get().strip()
        self.settings.port = SerialPort.device_of(self.port_var.get())
        self.settings.unit_id = self.unit_var.get().strip()
        self.settings.show_templates = bool(self.templates_var.get())
        profile = self.selected_profile()
        if profile:
            self.settings.last_profile = profile.key
        self.settings.save()

    def _start(self, action, needs_profile: bool = True, needs_port: bool = False) -> None:
        if self.worker and self.worker.is_alive():
            messagebox.showinfo(APP_TITLE, "A build or upload is already running.")
            return
        self._capture_settings()

        sketch = self.current_sketch()
        profile = self.selected_profile()
        if needs_profile and (not sketch or not profile):
            messagebox.showwarning(APP_TITLE, "Select a firmware version and a gas first.")
            return
        if needs_port and not self.settings.port:
            messagebox.showwarning(APP_TITLE, "Select the COM port of the unit first.")
            return
        cli = find_arduino_cli(self.settings.arduino_cli)
        if not cli:
            messagebox.showerror(
                APP_TITLE,
                "arduino-cli was not found.\n\nInstall it, or use Tools > Locate arduino-cli.",
            )
            return

        runner = CommandRunner(cli)
        self.job = FlashJob(
            self.settings,
            runner,
            lambda line: self.messages.put(("log", line)),
            lambda step: self.messages.put(("step", step)),
        )
        self._set_busy(True)
        self.worker = threading.Thread(
            target=self._run, args=(action, sketch, profile), daemon=True
        )
        self.worker.start()

    def _run(self, action, sketch: Sketch | None, profile: GasProfile | None) -> None:
        job = self.job
        assert job is not None
        result = "failed"
        try:
            summary = action(job, sketch, profile, self.settings.port)
            result = "ok"
            self.messages.put(("ok", summary))
        except JobCancelled:
            result = "cancelled"
            self.messages.put(("error", "Stopped by operator."))
        except (ToolchainError, SketchError) as exc:
            self.messages.put(("error", str(exc)))
        except Exception as exc:  # unexpected — still surface it in the log
            self.messages.put(("error", f"{type(exc).__name__}: {exc}"))
        finally:
            if sketch and profile and action is not _setup_action:
                job.record(sketch, profile, self.settings.port, result)
            self.messages.put(("done", ""))

    def build_and_upload(self) -> None:
        self._start(_build_upload_action, needs_port=True)

    def build_only(self) -> None:
        self._start(_build_action)

    def upload_only(self) -> None:
        self._start(_upload_action, needs_port=True)

    def setup_toolchain(self) -> None:
        self._start(_setup_action, needs_profile=False)

    def stop(self) -> None:
        if self.job:
            self.job.cancel()
        self.status_var.set("Stopping...")

    # -- misc ---------------------------------------------------------------

    def open_work_folder(self) -> None:
        self._open(self.settings.work_path)

    def open_flash_log(self) -> None:
        path = self.settings.work_path / "flash_log.csv"
        if not path.exists():
            messagebox.showinfo(APP_TITLE, "No units have been flashed yet.")
            return
        self._open(path)

    def _open(self, path: Path) -> None:
        directory = path.parent if path.suffix else path
        directory.mkdir(parents=True, exist_ok=True)
        try:
            if sys.platform.startswith("win"):
                subprocess.Popen(["explorer", str(path)])
            elif sys.platform == "darwin":
                subprocess.Popen(["open", str(path)])
            else:
                subprocess.Popen(["xdg-open", str(path)])
        except OSError as exc:
            messagebox.showerror(APP_TITLE, f"Could not open {path}:\n{exc}")

    def show_help(self) -> None:
        messagebox.showinfo(
            APP_TITLE,
            "1. Pick the firmware version (folder with the .ino).\n"
            "2. Pick the gas the unit is built for.\n"
            "3. Plug the unit in, pick its COM port, type the unit ID.\n"
            "4. Press Build && Upload.\n\n"
            "The firmware sources in your repository are never modified: the sketch is "
            "copied to a work folder, the chosen profile is enabled in that copy's "
            "inc/SensorConfig.h, and the copy is what gets compiled.\n\n"
            "Every build is kept, so flashing a second unit with the same gas skips "
            "straight to the upload. Each flash is appended to flash_log.csv "
            "(File > Open flash log).",
        )

    def _on_close(self) -> None:
        if self.worker and self.worker.is_alive():
            if not messagebox.askyesno(APP_TITLE, "A build is running. Stop it and quit?"):
                return
            if self.job:
                self.job.cancel()
        self._capture_settings()
        self.master.destroy()


# -- worker actions (module level so identity checks in _run stay simple) ----


def _build_upload_action(job: FlashJob, sketch, profile, port) -> str:
    job.build_and_upload(sketch, profile, port, do_upload=True)
    return f"{profile.gas} {profile.range_label} flashed to {port}"


def _build_action(job: FlashJob, sketch, profile, port) -> str:
    job.build_and_upload(sketch, profile, port, do_upload=False)
    return f"{profile.gas} {profile.range_label} built"


def _upload_action(job: FlashJob, sketch, profile, port) -> str:
    job.upload_only(sketch, profile, port)
    return f"{profile.gas} {profile.range_label} flashed to {port}"


def _setup_action(job: FlashJob, sketch, profile, port) -> str:
    job.setup_toolchain()
    return "Toolchain installed"


def main() -> int:
    root = Tk()
    root.title(APP_TITLE)
    root.geometry("1080x760")
    root.minsize(900, 640)
    try:
        ttk.Style().theme_use("vista" if sys.platform.startswith("win") else "clam")
    except tk.TclError:
        pass
    FlasherApp(root)
    root.mainloop()
    return 0
