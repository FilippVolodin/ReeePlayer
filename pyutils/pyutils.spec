# -*- mode: python ; coding: utf-8 -*-


block_cipher = None


vad_a = Analysis(['vad.py'],
             pathex=[],
             binaries=[],
             datas=[('vad.onnx', '.'), ('env\\Lib\\site-packages\\onnxruntime\\capi\\onnxruntime_providers_shared.dll', 'onnxruntime\\capi')],
             hiddenimports=[],
             hookspath=[],
             hooksconfig={},
             runtime_hooks=[],
             excludes=[],
             win_no_prefer_redirects=False,
             win_private_assemblies=False,
             cipher=block_cipher,
             noarchive=False)

anki_a = Analysis(['anki.py'],
             pathex=[],
             binaries=[],
             datas=[],
             hiddenimports=[],
             hookspath=[],
             hooksconfig={},
             runtime_hooks=[],
             excludes=[],
             win_no_prefer_redirects=False,
             win_private_assemblies=False,
             cipher=block_cipher,
             noarchive=False)

vad_pyz = PYZ(vad_a.pure, vad_a.zipped_data,
             cipher=block_cipher)

anki_pyz = PYZ(anki_a.pure, anki_a.zipped_data,
             cipher=block_cipher)

vad_exe = EXE(vad_pyz,
          vad_a.scripts,
          [],
          exclude_binaries=True,
          name='vad',
          debug=False,
          bootloader_ignore_signals=False,
          strip=False,
          upx=True,
          console=True,
          disable_windowed_traceback=False,
          target_arch=None,
          codesign_identity=None,
          entitlements_file=None )

anki_exe = EXE(anki_pyz,
          anki_a.scripts,
          [],
          exclude_binaries=True,
          name='anki',
          debug=False,
          bootloader_ignore_signals=False,
          strip=False,
          upx=True,
          console=True,
          disable_windowed_traceback=False,
          target_arch=None,
          codesign_identity=None,
          entitlements_file=None )

coll = COLLECT(vad_exe,
               vad_a.binaries,
               vad_a.zipfiles,
               vad_a.datas,
               anki_exe,
               anki_a.binaries,
               anki_a.zipfiles,
               anki_a.datas,
               strip=False,
               upx=True,
               upx_exclude=[],
               name='pyutils')
