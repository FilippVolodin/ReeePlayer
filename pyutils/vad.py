import onnxruntime as rt
import numpy as np
import soundfile as sf
import argparse
import socket
from pathlib import Path
from os.path import join
import time
import select

class OnnxWrapper():

    def __init__(self, path):
        self.session = rt.InferenceSession(path)
        self.session.intra_op_num_threads = 1
        self.session.inter_op_num_threads = 1
        self.reset_states()

    def reset_states(self):
        self._h = np.zeros((2, 1, 64)).astype('float32')
        self._c = np.zeros((2, 1, 64)).astype('float32')

    def __call__(self, x, sr: int):

        x = np.expand_dims(x, 0)

        if sr / x.shape[1] > 31.25:
            raise ValueError("Input audio chunk is too short")

        ort_inputs = {'input': x, 'h0': self._h, 'c0': self._c}
        ort_outs = self.session.run(None, ort_inputs)
        out, self._h, self._c = ort_outs
        return out[:,1,0]

parser = argparse.ArgumentParser(description='Generate VAD')
parser.add_argument('wav_file', type=str, help='Wav-file path to process')
parser.add_argument('server_port', type=int, help='Server port for sending data')
parser.add_argument('first_chunk', type=int, help='First chunk')
args = parser.parse_args()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    sock.connect(("127.0.0.1", args.server_port))
except socket.error as exc:
    exit(1)

onnx = OnnxWrapper(join(Path(__file__).parent, "vad.onnx"))

SAMPLING_RATE = 16000
window_size_samples = 1536

import soundfile as sf
data, samplerate = sf.read(args.wav_file, dtype='float32') 

num_chunks = len(data) // window_size_samples
sock.sendall(num_chunks.to_bytes(4, 'big'))

last_time = time.time()

speech_probs = []
begin = args.first_chunk * window_size_samples
end = len(data) // window_size_samples * window_size_samples
for i in range(begin, end, window_size_samples):
    speech_prob = onnx(data[i: i+ window_size_samples], SAMPLING_RATE).item()
    speech_probs.append(speech_prob)

    read_sockets, _, _ = select.select([sock], [], [], 0)
    if sock in read_sockets:
        cmd = sock.recv(1)
        if cmd == b'\x01':
            exit()

    read_sockets.clear()

    cur_time = time.time()
    if (cur_time - last_time > 1.0):
        if speech_probs:
            pb = [int(p * 256) for p in speech_probs]
            sock.sendall(bytes(pb))
            speech_probs.clear()
        last_time = cur_time

if speech_probs:
    pb = [int(p * 256) for p in speech_probs]
    sock.sendall(bytes(pb))
