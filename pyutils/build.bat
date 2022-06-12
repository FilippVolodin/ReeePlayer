python -m venv env
call env\Scripts\activate
pip install -r requirements.txt
pyinstaller -y vad.spec
