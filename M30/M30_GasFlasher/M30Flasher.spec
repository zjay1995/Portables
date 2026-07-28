# PyInstaller build recipe for the M30 Gas Flasher.
# Build with:  python -m PyInstaller --noconfirm --clean M30Flasher.spec

block_cipher = None

analysis = Analysis(
    ["M30Flasher.py"],
    pathex=["."],
    binaries=[],
    datas=[],
    hiddenimports=["serial.tools.list_ports"],
    hookspath=[],
    runtime_hooks=[],
    excludes=["numpy", "matplotlib", "PIL", "pytest"],
    cipher=block_cipher,
)

pyz = PYZ(analysis.pure, analysis.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    analysis.scripts,
    analysis.binaries,
    analysis.zipfiles,
    analysis.datas,
    name="M30Flasher",
    console=False,          # windowed app — no black console box
    debug=False,
    strip=False,
    upx=False,
    icon=None,
)
