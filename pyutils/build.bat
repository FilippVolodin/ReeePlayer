python -m venv env
call env\Scripts\activate
REM python -m pip install numpy onnxruntime soundfile pyinstaller genanki
pip install -r requirements.txt
pyinstaller -y pyutils.spec
call env\Scripts\deactivate
