# PyInstaller build recipe for the M30 Gas Flasher (PyInstaller 6.x).
# Build with:  python -m PyInstaller --noconfirm --clean M30Flasher.spec
# Output:      dist\M30Flasher.exe  — single file, no console window.

a = Analysis(
    ["M30Flasher.py"],
    pathex=["."],
    binaries=[],
    datas=[],
    hiddenimports=["serial.tools.list_ports"],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=["numpy", "matplotlib", "PIL", "pytest", "pandas", "scipy"],
    noarchive=False,
    optimize=0,
)

pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name="M30Flasher",
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=False,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,          # windowed app — no black console box
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=None,
)
