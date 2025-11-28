#!/usr/bin/env python3

"""
Rate Convert音频处理验证脚本
功能：验证Rate Convert处理后的音频数据的THD和频率响应
支持单文件验证和批量HTTP验证两种模式
python rate_cvt_test.py --mode batch
"""

import numpy as np
import io
import soundfile as sf
import librosa
import scipy.signal
import scipy.fft
from scipy.signal import periodogram, welch, find_peaks

# 创建一个类似scipy.signal.snr的函数
def snr(signal, fs, target_freq=1000, signal_bandwidth=50):
    """
    计算信噪比（SNR）
    模拟scipy.signal.snr的功能
    Args:
        signal: 输入信号
        fs: 采样率
        target_freq: 目标频率（Hz）
        signal_bandwidth: 信号带宽（Hz）
    Returns:
        snr_db: 信噪比（dB）
    """
    # 计算功率谱密度
    freqs, psd = periodogram(signal, fs=fs, window='hann', nfft=None, return_onesided=True)
    df = freqs[1] - freqs[0]
    
    # 定义信号频率范围
    signal_freq_min = max(0, target_freq - signal_bandwidth/2)
    signal_freq_max = min(fs/2, target_freq + signal_bandwidth/2)
    
    # 找到信号和噪声频率范围
    signal_mask = (freqs >= signal_freq_min) & (freqs <= signal_freq_max)
    
    # 计算功率
    signal_power = np.sum(psd[signal_mask]) * df
    noise_power = np.sum(psd[~signal_mask]) * df
    
    # 计算SNR
    if noise_power > 0:
        snr_db = 10 * np.log10(signal_power / noise_power)
    else:
        snr_db = float('inf')
    
    return snr_db
from scipy.stats import pearsonr
import argparse
import os
import sys
import wave
import urllib.request
import json
from tqdm import tqdm
import math

class RateConvertVerifier:
    def __init__(self, sample_rate=48000, channels=1):
        self.sample_rate = sample_rate
        self.channels = channels
        
    def load_audio_file(self, filepath, duration=None):
        """
        加载音频文件
        Args:
            filepath: 音频文件路径
            duration: 加载时长（秒），None表示加载全部
        Returns:
            audio_data: 音频数据数组
        """
        if not os.path.exists(filepath):
            raise FileNotFoundError(f"音频文件不存在: {filepath}")
            
        # 使用librosa加载音频
        audio_data, sr = librosa.load(filepath, sr=self.sample_rate, mono=(self.channels==1), duration=duration)
        
        if self.channels == 1:
            return audio_data
        else:
            # 如果是立体声，重新整形为 (samples, channels)
            return audio_data.reshape(-1, self.channels)

    def load_audio_bytes(self, data_bytes, duration=None):
        """
        从内存字节加载音频数据
        """
        with sf.SoundFile(io.BytesIO(data_bytes)) as f:
            sr = f.samplerate
            ch = f.channels
            # 若未固定声道数，则遵从文件声道
            target_mono = (self.channels == 1)
            data, _ = sf.read(io.BytesIO(data_bytes), dtype='float32', always_2d=not target_mono)
        # 若验证器初始采样率与文件不同，更新为文件采样率
        self.sample_rate = sr
        self.channels = 1 if target_mono else ch
        if self.channels == 1 and data.ndim > 1:
            data = data[:, 0]
        return data
    
    def calculate_thd(self, x, fs, num_harmonics=6):
        """
        计算与 MATLAB thd(x, fs, num_harmonics) 等价的总谐波失真（THD）
        
        参数：
            x : ndarray
                输入信号（时域）
            fs : float
                采样率 (Hz)
            num_harmonics : int
                计算到第几次谐波（默认 6）
        返回：
            thd_db : float
                总谐波失真 (dBc)
            thd_percent : float
                总谐波失真 (%)
        """
        # ---- 步骤 1: 归一化 ----
        x = x / np.max(np.abs(x))
        
        # ---- 步骤 2: 使用周期图计算功率谱密度（与MATLAB一致）----
        freqs, psd = periodogram(x, fs, window='hann', nfft=None, return_onesided=True)
        
        # ---- 步骤 3: 找到基波频率 ----
        # 在功率谱密度中找到最大值对应的频率
        fundamental_idx = np.argmax(psd)
        fundamental_freq = freqs[fundamental_idx]
        fundamental_power = psd[fundamental_idx]

        # ---- 步骤 4: 计算谐波功率 ----
        harmonic_powers = []
        for k in range(2, num_harmonics + 1):
            target_freq = k * fundamental_freq
            # 找到最接近目标频率的索引
            idx = np.argmin(np.abs(freqs - target_freq))
            if idx < len(psd):
                harmonic_powers.append(psd[idx])

        harmonic_power_sum = np.sum(harmonic_powers)

        # ---- 步骤 5: 计算 THD（与MATLAB公式一致）----
        # MATLAB的THD公式：THD = sqrt(sum(harmonic_powers) / fundamental_power)
        thd_ratio = np.sqrt(harmonic_power_sum / fundamental_power)
        thd_percent = thd_ratio * 100
        thd_db = 20 * np.log10(thd_ratio)
        print(f"thd_db: {thd_db}, thd_percent: {thd_percent}, fundamental_freq: {fundamental_freq}, fundamental_power: {fundamental_power}, harmonic_power_sum: {harmonic_power_sum}")
        return thd_db, thd_percent, fundamental_freq, fundamental_power, harmonic_power_sum
    
    def calculate_snr(self, audio_data, target_freq=1000):
        """
        计算信噪比（SNR）
        Args:
            audio_data: 音频数据
            target_freq: 目标频率（Hz）
        Returns:
            snr_db: 信噪比（dB）
        """
        if self.channels > 1:
            audio_mono = np.mean(audio_data, axis=1)
        else:
            audio_mono = audio_data
            
        # 使用自定义的snr函数计算SNR
        snr_db = snr(audio_mono, self.sample_rate, target_freq, signal_bandwidth=50)
        print(f"snr_db: {snr_db}")
        return snr_db
    
    def calculate_frequency_response(self, input_audio, output_audio, input_fs, output_fs, target_freq=1000):
        """
        计算幅频特性
        对比输入文件和输出文件的频率响应，计算幅度差异
        处理不同采样率的情况
        Args:
            input_audio: 输入音频数据
            output_audio: 输出音频数据
            input_fs: 输入采样率
            output_fs: 输出采样率
            target_freq: 目标频率（Hz）
        Returns:
            magnitude_response_diff: 输入输出幅度响应差异（dB）
        """
        # 处理输入音频
        if self.channels > 1:
            input_mono = np.mean(input_audio, axis=1)
        else:
            input_mono = input_audio
            
        # 处理输出音频
        if self.channels > 1:
            output_mono = np.mean(output_audio, axis=1)
        else:
            output_mono = output_audio
            
        # 计算输入音频的功率谱密度
        freqs_input, psd_input = periodogram(input_mono, input_fs, window='hann', nfft=None, return_onesided=True)
        
        # 计算输出音频的功率谱密度
        freqs_output, psd_output = periodogram(output_mono, output_fs, window='hann', nfft=None, return_onesided=True)
        
        # 检查目标频率是否在有效范围内
        nyquist_input = input_fs / 2
        nyquist_output = output_fs / 2
        
        if target_freq > nyquist_input or target_freq > nyquist_output:
            # 目标频率超出奈奎斯特频率，无法比较
            return -np.inf
        
        # 找到目标频率处的响应
        target_idx_input = np.argmin(np.abs(freqs_input - target_freq))
        target_idx_output = np.argmin(np.abs(freqs_output - target_freq))
        
        input_response = psd_input[target_idx_input]
        output_response = psd_output[target_idx_output]
        
        # 计算幅频特性差异（输入输出幅度响应差异）
        if input_response > 0 and output_response > 0:
            input_magnitude = 10 * np.log10(input_response)
            output_magnitude = 10 * np.log10(output_response)
            magnitude_response_diff = output_magnitude - input_magnitude
        else:
            magnitude_response_diff = -np.inf
        
        print(f"magnitude_response_diff: {magnitude_response_diff}")
        return magnitude_response_diff
     
    def verify_rate_convert_processing(self, input_file, output_file, src_rate, dest_rate, 
                                       duration=None, tolerance=None):
        """
        验证Rate Convert处理结果
        Args:
            input_file: 输入音频文件路径
            output_file: 输出音频文件路径
            src_rate: 源采样率
            dest_rate: 目标采样率
            duration: 验证时长（秒）
            tolerance: 误差容忍度字典
        Returns:
            results: 验证结果字典
        """
        if tolerance is None:
            tolerance = {
                'thd_max_db': -50.0,      # THD最大允许值 (dBc)
                'thd_max_percent': 1.0,   # THD最大允许值 (%)
                'snr_min_db': 20.0,       # 最小信噪比 (dB)
                'magnitude_response_max': 3.0,  # 最大幅度响应差异 (dB)
            }
        
        print(f"开始验证Rate Convert处理结果...")
        print(f"输入文件: {input_file}")
        print(f"输出文件: {output_file}")
        print(f"采样率转换: {src_rate} -> {dest_rate}")
        print("-" * 50)
        
        # 加载音频文件
        try:
            input_audio = self.load_audio_file(input_file, duration)
            output_audio = self.load_audio_file(output_file, duration)
        except Exception as e:
            print(f"加载音频文件失败: {e}")
            return None
        
        results = {}
        
        # 1. 计算THD
        try:
            output_thd_db, output_thd_percent, _, _, _ = self.calculate_thd(output_audio, dest_rate)
            results['output_thd_db'] = output_thd_db
            results['output_thd_percent'] = output_thd_percent
            results['thd_pass'] = output_thd_db < tolerance['thd_max_db'] and output_thd_percent < tolerance['thd_max_percent']
        except Exception as e:
            results['thd_pass'] = False
        print("thd pass:", results['thd_pass'])
        # 2. 计算信噪比
        try:
            output_snr_db = self.calculate_snr(output_audio, 1000)
            results['output_snr_db'] = output_snr_db
            results['snr_pass'] = output_snr_db > tolerance['snr_min_db']
        except Exception as e:
            results['snr_pass'] = False
        print("snr pass:", results['snr_pass'])
        # 3. 计算幅频特性
        try:
            magnitude_response_diff = self.calculate_frequency_response(input_audio, output_audio, src_rate, dest_rate, 1000)
            results['magnitude_response_diff'] = magnitude_response_diff
            results['freq_response_pass'] = abs(magnitude_response_diff) < tolerance['magnitude_response_max']
        except Exception as e:
            results['freq_response_pass'] = False
        print("freq response pass:", results['freq_response_pass'])
        # 总结
        total_tests = 3
        passed_tests = sum([results.get('thd_pass', False), 
                          results.get('snr_pass', False), 
                          results.get('freq_response_pass', False)])
        
        results['overall_pass'] = passed_tests == total_tests
        
        return results

def get_wav_info(filepath):
    with wave.open(filepath, 'rb') as wf:
        sample_rate = wf.getframerate()
        channels = wf.getnchannels()
        bits_per_sample = wf.getsampwidth() * 8
    return sample_rate, channels, bits_per_sample

def verify_rate_convert_processing_from_bytes(input_bytes, output_bytes, src_rate, dest_rate,
                                            sample_rate_hint=None, channels_hint=None,
                                            duration=None, tolerance=None):
    """直接从字节数据进行验证的辅助函数（供批量脚本调用）"""
    verifier = RateConvertVerifier(sample_rate=sample_rate_hint or 48000,
                                 channels=channels_hint or 1)
    input_audio = verifier.load_audio_bytes(input_bytes, duration)
    output_audio = verifier.load_audio_bytes(output_bytes, duration)

    results = {}
    
    # THD计算
    try:
        output_thd_db, output_thd_percent, _, _, _ = verifier.calculate_thd(output_audio, dest_rate)
        results['output_thd_db'] = output_thd_db
        results['output_thd_percent'] = output_thd_percent
        results['thd_pass'] = (output_thd_db < (tolerance['thd_max_db'] if tolerance and 'thd_max_db' in tolerance else -40.0) and 
                             output_thd_percent < (tolerance['thd_max_percent'] if tolerance and 'thd_max_percent' in tolerance else 1.0))
    except Exception:
        results['thd_pass'] = False
    print("thd pass:", results['thd_pass'])
    # 信噪比计算
    try:
        output_snr_db = verifier.calculate_snr(output_audio, 1000)
        results['output_snr_db'] = output_snr_db
        results['snr_pass'] = output_snr_db > (tolerance['snr_min_db'] if tolerance and 'snr_min_db' in tolerance else 20.0)
    except Exception:
        results['snr_pass'] = False
    print("snr pass:", results['snr_pass'])
    # 幅频特性计算
    try:
        magnitude_response_diff = verifier.calculate_frequency_response(input_audio, output_audio, src_rate, dest_rate, 1000)
        results['magnitude_response_diff'] = magnitude_response_diff
        results['freq_response_pass'] = abs(magnitude_response_diff) < (tolerance['magnitude_response_max'] if tolerance and 'magnitude_response_max' in tolerance else 3.0)
    except Exception:
        results['freq_response_pass'] = False
    print("freq response pass:", results['freq_response_pass'])
    total_tests = 3
    passed_tests = sum([results.get('thd_pass', False), results.get('snr_pass', False), results.get('freq_response_pass', False)])
    results['overall_pass'] = (passed_tests == total_tests)
    return results

# HTTP批量验证相关函数
SOURCE_BASE_URL = 'http://10.18.20.184:8080/audio_files/audio_test_dataset/sine'
DEST_BASE_URL = 'http://10.18.20.184:8080/upload/ae_test/rate_cvt_test'

# 固定的源文件名
SOURCE_FILENAME = 'sine1kHz0dB_48000_1_{bits}_10'

# 待验证的目标采样率
RATES = [8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000]
BITS_PER_SAMPLE = [16, 24, 32]
def download_bytes(url):
    try:
        with urllib.request.urlopen(url) as resp:
            return resp.read()
    except Exception as e:
        print(f'下载失败: {url} -> {e}')
        return None

def download_input_file(src_rate, bits):
    # 从HTTP下载源文件
    fname = f"sine1kHz0dB_{src_rate}_1_{bits}_10.wav"
    remote = f"{SOURCE_BASE_URL}/{fname}"
    data = download_bytes(remote)
    if not data:
        print(f'未能获取源文件: {remote}')
    return data, fname

def download_output_file(src_rate, dest_rate, bits):
    # 目标文件命名规则：sine1kHz0dB_{src_rate}_to_{dest_rate}_1_{bits}_10.wav
    fname = f"sine1kHz0dB_{src_rate}_to_{dest_rate}_1_{bits}_10.wav"
    remote = f"{DEST_BASE_URL}/{bits}/{fname}"
    data = download_bytes(remote)
    if not data:
        print(f'未能获取目标文件: {remote}')
    return data, fname

def batch_verify():
    """批量HTTP验证模式"""
    # 从main函数传递的参数中获取--rate参数
    import sys
    rate_arg = None
    if '--rate' in sys.argv:
        rate_idx = sys.argv.index('--rate')
        if rate_idx + 1 < len(sys.argv):
            rate_arg = int(sys.argv[rate_idx + 1])
    for src_rate in RATES:
        for bits in BITS_PER_SAMPLE:
            # 下载源文件
            input_bytes, input_name = download_input_file(src_rate, bits)
            if not input_bytes:
                print(f'跳过：无法获取目标文件(HTTP) sine1kHz0dB_{src_rate}_1_{bits}_10.wav')
                return

            print(f'成功下载源文件: {input_name}')

            # 遍历目标采样率
            for dest_rate in RATES:
                if rate_arg and dest_rate != rate_arg:
                    continue
                    
                output_bytes, out_fname = download_output_file(src_rate, dest_rate, bits)
                if not output_bytes:
                    print(f'跳过：无法获取目标文件(HTTP) sine1kHz0dB_{src_rate}_to_{dest_rate}_1_{bits}_10.wav')
                    continue
                    
                print(f'\n==== 验证: {out_fname} ({src_rate} -> {dest_rate}, bits: {bits}) ====')
                # 直接在内存中验证
                try:
                    results = verify_rate_convert_processing_from_bytes(
                        input_bytes, output_bytes, src_rate, dest_rate,
                        sample_rate_hint=None, channels_hint=None,
                        duration=None, tolerance=None,
                    )
                    passed = results and results.get('overall_pass')
                    print('通过' if passed else '失败', ': ', out_fname)
                    if results:
                        print(f'  THD: {results.get("output_thd_db", 0):.2f} dBc')
                        print(f'  SNR: {results.get("output_snr_db", 0):.2f} dB')
                        print(f'  幅频特性差异: {results.get("magnitude_response_diff", 0):.2f} dB')
                except Exception as e:
                    print(f'运行失败: {e}')

def main():
    parser = argparse.ArgumentParser(description='Rate Convert音频处理验证脚本')
    parser.add_argument('--mode', choices=['single', 'batch'], default='single', 
                       help='验证模式: single=单文件验证, batch=批量HTTP验证')
    parser.add_argument('--input', '-i', help='输入音频文件路径 (single模式)')
    parser.add_argument('--output', '-o', help='输出音频文件路径 (single模式)')
    parser.add_argument('--src-rate', type=int, default=48000, help='源采样率 (默认: 48000)')
    parser.add_argument('--dest-rate', type=int, default=48000, help='目标采样率 (默认: 48000)')
    parser.add_argument('--sample-rate', type=int, default=None, help='采样率 (自动从wav获取)')
    parser.add_argument('--channels', type=int, default=None, help='声道数 (自动从wav获取)')
    parser.add_argument('--bits', type=int, default=None, help='位宽 (自动从wav获取)')
    parser.add_argument('--duration', type=float, help='验证时长（秒）')
    parser.add_argument('--tolerance', type=str, help='误差容忍度JSON字符串')
    parser.add_argument('--rate', type=int, help='批量模式：只处理指定采样率')
    
    args = parser.parse_args()

    if args.mode == 'batch':
        # 批量HTTP验证模式
        batch_verify()
        return

    # 单文件验证模式
    if not args.input or not args.output:
        print("单文件模式需要指定 --input 和 --output 参数")
        sys.exit(1)

    # 自动从wav头获取参数
    ext = os.path.splitext(args.input)[1].lower()
    if ext == '.wav':
        sample_rate, channels, bits_per_sample = get_wav_info(args.input)
    else:
        sample_rate = args.sample_rate if args.sample_rate else 16000
        channels = args.channels if args.channels else 1
        bits_per_sample = args.bits if args.bits else 16

    # 创建验证器
    verifier = RateConvertVerifier(sample_rate=sample_rate, channels=channels)
    
    # 解析容忍度设置
    tolerance = None
    if args.tolerance:
        try:
            tolerance = json.loads(args.tolerance)
        except json.JSONDecodeError:
            print("警告: 无法解析容忍度设置，使用默认值")
    
    # 执行验证
    results = verifier.verify_rate_convert_processing(
        input_file=args.input,
        output_file=args.output,
        src_rate=args.src_rate,
        dest_rate=args.dest_rate,
        duration=args.duration,
        tolerance=tolerance
    )
    
    if results is None:
        sys.exit(1)
    
    # 返回适当的退出码
    sys.exit(0 if results['overall_pass'] else 1)

if __name__ == '__main__':
    main()
