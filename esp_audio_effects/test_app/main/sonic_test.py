#!/usr/bin/env python3
"""
Sonic音频处理验证脚本
功能：验证Sonic处理后的音频数据是否符合设置的speed和pitch参数
支持单文件验证和批量HTTP验证两种模式
python sonic_test.py --mode batch
"""

import numpy as np
import io
import soundfile as sf
import librosa
import scipy.signal
import scipy.fft
from scipy.stats import pearsonr
import argparse
import os
import sys
import wave
import urllib.request
import json

class SonicVerifier:
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
        从内存字节加载音频数据。
        优先保持原采样率，若需要可在外部保证输入输出采样率一致。
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
    
    def calculate_tempo_error(self, input_samples, output_samples, speed_ratio):
        """
        计算Tempo Error
        Args:
            input_samples: 输入样本数
            output_samples: 输出样本数
            speed_ratio: 速度比例
        Returns:
            tempo_error: Tempo误差百分比
        """
        expected_samples = input_samples / speed_ratio
        tempo_error = abs(output_samples - expected_samples) / expected_samples * 100.0
        return tempo_error
    
    def extract_f0(self, audio_data):
        """
        提取基频F0
        Args:
            audio_data: 音频数据
        Returns:
            f0_mean: 平均基频
        """
        if self.channels > 1:
            audio_mono = np.mean(audio_data, axis=1)
        else:
            audio_mono = audio_data
        # 使用librosa提取基频
        f0, voiced_flag, voiced_probs = librosa.pyin(audio_mono, 
                                                   fmin=librosa.note_to_hz('C2'), 
                                                   fmax=librosa.note_to_hz('C7'),
                                                   sr=self.sample_rate)
        # 计算有效F0的平均值
        valid_f0 = f0[~np.isnan(f0)]
        if len(valid_f0) > 0:
            return np.mean(valid_f0)
        else:
            return 0.0
    
    def calculate_spectral_similarity(self, input_audio, output_audio):
        """
        计算频谱相似度
        Args:
            input_audio: 输入音频
            output_audio: 输出音频
        Returns:
            similarity: 频谱相似度 (0-1)
        """
        if self.channels > 1:
            input_mono = input_audio[:, 0]
            output_mono = output_audio[:, 0]
        else:
            input_mono = input_audio
            output_mono = output_audio
            
        # 计算STFT
        hop_length = 512
        n_fft = 2048
        
        input_stft = librosa.stft(input_mono, n_fft=n_fft, hop_length=hop_length)
        output_stft = librosa.stft(output_mono, n_fft=n_fft, hop_length=hop_length)
        
        # 计算幅度谱
        input_mag = np.abs(input_stft)
        output_mag = np.abs(output_stft)
        
        # 计算对数幅度谱
        input_log_mag = librosa.amplitude_to_db(input_mag, ref=np.max)
        output_log_mag = librosa.amplitude_to_db(output_mag, ref=np.max)
        
        # 计算频谱相似度（使用相关系数）
        # 将2D频谱展平为1D进行比较
        input_flat = input_log_mag.flatten()
        output_flat = output_log_mag.flatten()
        
        # 确保长度一致
        min_len = min(len(input_flat), len(output_flat))
        input_flat = input_flat[:min_len]
        output_flat = output_flat[:min_len]
        
        # 计算皮尔逊相关系数
        correlation, _ = pearsonr(input_flat, output_flat)
        
        # 将相关系数转换为0-1范围的相似度
        similarity = (correlation + 1) / 2
        return max(0, similarity)  # 确保非负
    
    def detect_transient_preservation(self, input_audio, output_audio):
        """
        检测瞬态保持性
        Args:
            input_audio: 输入音频
            output_audio: 输出音频
        Returns:
            preservation_score: 瞬态保持性分数 (0-1)
        """
        if self.channels > 1:
            input_mono = input_audio[:, 0]
            output_mono = output_audio[:, 0]
        else:
            input_mono = input_audio
            output_mono = output_audio
            
        # 计算能量包络
        input_energy = np.abs(input_mono) ** 2
        output_energy = np.abs(output_mono) ** 2
        
        # 检测峰值
        input_peaks, _ = scipy.signal.find_peaks(input_energy, height=np.max(input_energy) * 0.1)
        output_peaks, _ = scipy.signal.find_peaks(output_energy, height=np.max(output_energy) * 0.1)
        
        # 计算保持性分数
        if len(input_peaks) > 0:
            preservation_score = min(len(output_peaks) / len(input_peaks), 1.0)
        else:
            preservation_score = 0.0
            
        return preservation_score
    
    def verify_sonic_processing(self, input_file, output_file, speed_ratio, pitch_ratio, 
                              duration=None, tolerance=None):
        """
        验证Sonic处理结果
        Args:
            input_file: 输入音频文件路径
            output_file: 输出音频文件路径
            speed_ratio: 速度比例
            pitch_ratio: 音调比例
            duration: 验证时长（秒）
            tolerance: 误差容忍度字典
        Returns:
            results: 验证结果字典
        """
        if tolerance is None:
            tolerance = {
                'tempo_error': 15.0,    # Tempo误差百分比
                'bpm_error': 30.0,      # BPM误差百分比
                'f0_error': 25.0,       # F0误差百分比
                'spectral_similarity': 0.5,  # 频谱相似度阈值
                'transient_preservation': 0.3  # 瞬态保持性阈值
            }
        
        print(f"开始验证Sonic处理结果...")
        print(f"输入文件: {input_file}")
        print(f"输出文件: {output_file}")
        print(f"速度比例: {speed_ratio}")
        print(f"音调比例: {pitch_ratio}")
        print("-" * 50)
        
        # 加载音频文件
        try:
            input_audio = self.load_audio_file(input_file, duration)
            output_audio = self.load_audio_file(output_file, duration)
        except Exception as e:
            print(f"加载音频文件失败: {e}")
            return None
        
        results = {}
        
        # 1. 检测时长：计算Tempo Error
        input_samples = len(input_audio)
        output_samples = len(output_audio)
        tempo_error = self.calculate_tempo_error(input_samples, output_samples, speed_ratio)
        results['tempo_error'] = tempo_error
        results['tempo_pass'] = tempo_error < tolerance['tempo_error']
        
        print(f"1. Tempo Error: {float(tempo_error):.2f}% (阈值: {float(tolerance['tempo_error']):.1f}%)")
        print(f"   输入样本数: {int(input_samples)}, 输出样本数: {int(output_samples)}")
        print(f"   预期样本数: {int(input_samples/speed_ratio):d}")
        print(f"   结果: {'通过' if results['tempo_pass'] else '失败'}")
        
        # 2. 检测基频：提取F0
        try:
            input_f0 = self.extract_f0(input_audio)
            output_f0 = self.extract_f0(output_audio)
            expected_f0 = input_f0 * pitch_ratio
            f0_error = abs(output_f0 - expected_f0) / expected_f0 * 100.0 if expected_f0 > 0 else 0
            results['f0_error'] = f0_error
            results['f0_pass'] = f0_error < tolerance['f0_error']
            
            print(f"2. F0检测: 输入={float(input_f0):.1f}Hz, 输出={float(output_f0):.1f}Hz, 预期={float(expected_f0):.1f}Hz")
            print(f"   F0误差: {float(f0_error):.2f}% (阈值: {float(tolerance['f0_error']):.1f}%)")
            print(f"   结果: {'通过' if results['f0_pass'] else '失败'}")
        except Exception as e:
            print(f"2. F0检测失败: {e}")
            results['f0_error'] = float('inf')
            results['f0_pass'] = False
        
        # 3. 检测频谱：计算频谱相似度
        try:
            spectral_similarity = self.calculate_spectral_similarity(input_audio, output_audio)
            results['spectral_similarity'] = spectral_similarity
            results['spectral_pass'] = spectral_similarity > tolerance['spectral_similarity']
            
            print(f"3. 频谱相似度: {float(spectral_similarity):.3f} (阈值: {float(tolerance['spectral_similarity']):.1f})")
            print(f"   结果: {'通过' if results['spectral_pass'] else '失败'}")
        except Exception as e:
            print(f"3. 频谱相似度计算失败: {e}")
            results['spectral_similarity'] = 0.0
            results['spectral_pass'] = False
        
        # 4. 检测瞬态：脉冲保持性
        try:
            transient_preservation = self.detect_transient_preservation(input_audio, output_audio)
            results['transient_preservation'] = transient_preservation
            results['transient_pass'] = transient_preservation > tolerance['transient_preservation']
            
            print(f"4. 瞬态保持性: {float(transient_preservation):.3f} (阈值: {float(tolerance['transient_preservation']):.1f})")
            print(f"   结果: {'通过' if results['transient_pass'] else '失败'}")
        except Exception as e:
            print(f"4. 瞬态保持性检测失败: {e}")
            results['transient_preservation'] = 0.0
            results['transient_pass'] = False
        
        # 总结
        print("-" * 50)
        total_tests = 4
        passed_tests = sum([results['tempo_pass'], results['f0_pass'], 
                          results['spectral_pass'], results['transient_pass']])
        
        print(f"验证总结: {passed_tests}/{total_tests} 项测试通过")
        results['overall_pass'] = passed_tests == total_tests
        print(f"总体结果: {'全部通过' if results['overall_pass'] else '部分失败'}")
        
        return results

def get_wav_info(filepath):
    with wave.open(filepath, 'rb') as wf:
        sample_rate = wf.getframerate()
        channels = wf.getnchannels()
        bits_per_sample = wf.getsampwidth() * 8
    return sample_rate, channels, bits_per_sample

def verify_sonic_processing_from_bytes(input_bytes, output_bytes, speed_ratio, pitch_ratio,
                                       sample_rate_hint=None, channels_hint=None,
                                       duration=None, tolerance=None):
    """直接从字节数据进行验证的辅助函数（供批量脚本调用）"""
    verifier = SonicVerifier(sample_rate=sample_rate_hint or 48000,
                             channels=channels_hint or 1)
    input_audio = verifier.load_audio_bytes(input_bytes, duration)
    output_audio = verifier.load_audio_bytes(output_bytes, duration)

    results = {}
    input_samples = len(input_audio)
    output_samples = len(output_audio)
    results['tempo_error'] = verifier.calculate_tempo_error(input_samples, output_samples, speed_ratio)
    results['tempo_pass'] = results['tempo_error'] < (tolerance['tempo_error'] if tolerance and 'tempo_error' in tolerance else 15.0)

    try:
        input_f0 = verifier.extract_f0(input_audio)
        output_f0 = verifier.extract_f0(output_audio)
        expected_f0 = input_f0 * pitch_ratio
        f0_error = abs(output_f0 - expected_f0) / expected_f0 * 100.0 if expected_f0 > 0 else 0
        results['f0_error'] = f0_error
        results['f0_pass'] = f0_error < (tolerance['f0_error'] if tolerance and 'f0_error' in tolerance else 25.0)
    except Exception:
        results['f0_error'] = float('inf')
        results['f0_pass'] = False

    try:
        spectral_similarity = verifier.calculate_spectral_similarity(input_audio, output_audio)
        results['spectral_similarity'] = spectral_similarity
        results['spectral_pass'] = spectral_similarity > (tolerance['spectral_similarity'] if tolerance and 'spectral_similarity' in tolerance else 0.5)
    except Exception:
        results['spectral_similarity'] = 0.0
        results['spectral_pass'] = False

    try:
        transient_preservation = verifier.detect_transient_preservation(input_audio, output_audio)
        results['transient_preservation'] = transient_preservation
        results['transient_pass'] = transient_preservation > (tolerance['transient_preservation'] if tolerance and 'transient_preservation' in tolerance else 0.3)
    except Exception:
        results['transient_preservation'] = 0.0
        results['transient_pass'] = False

    total_tests = 4
    passed_tests = sum([results['tempo_pass'], results['f0_pass'], results['spectral_pass'], results['transient_pass']])
    results['overall_pass'] = (passed_tests == total_tests)
    return results

# HTTP批量验证相关函数
SOURCE_BASE_URL = 'http://10.18.20.184:8080/audio_files/audio_test_dataset/voice'
DEST_BASE_URL = 'http://10.18.20.184:8080/upload/ae_test/sonic_test'

# 固定的测试基准文件名（源音频）
BASE_NAMES = [
    'manch_48000_1_16_10',
    'manen_48000_1_16_10',
    'manloud_48000_1_16_10',
    'womanch_16000_1_16_6',
    'womanen_48000_1_16_10',
    'womanloud_48000_1_16_10',
]

# 待验证的(speed, pitch)组合（可按需调整）
COMBINATIONS = [
    (0.50, 1.00),
    (1.00, 0.50),
    (1.00, 2.00),
    (2.00, 1.00),
    (0.75, 1.25),
    (1.25, 0.75),
]

BITS_PER_SAMPLE = [16, 24, 32]

def download_bytes(url):
    try:
        with urllib.request.urlopen(url) as resp:
            return resp.read()
    except Exception as e:
        print(f'下载失败: {url} -> {e}')
        return None

def download_input_file(base_name):
    # 从HTTP下载源文件：如果有扩展名，直接用；否则尝试 .wav/.pcm
    stem, ext = os.path.splitext(base_name)
    if ext:
        remote = f"{SOURCE_BASE_URL}/{base_name}"
        data = download_bytes(remote)
        if data:
            return data, f"{stem}{ext}"
    for e in ['.wav', '.pcm']:
        remote = f"{SOURCE_BASE_URL}/{stem}{e}"
        data = download_bytes(remote)
        if data:
            return data, f"{stem}{e}"
    return None, None

def download_output_file(base_name, speed, pitch, bits):
    # 目标文件命名规则：{baseStem}_speed_{speed:.2f}_pitch_{pitch:.2f}.wav
    stem, _ = os.path.splitext(base_name)
    fname = f"{stem}_speed_{speed:.2f}_pitch_{pitch:.2f}_bits_{bits}.wav"
    remote = f"{DEST_BASE_URL}/{fname}"
    data = download_bytes(remote)
    if not data:
        print(f'未能获取目标文件: {remote}')
    return data, fname

def batch_verify():
    """批量HTTP验证模式"""
    # 从main函数传递的参数中获取--base参数
    import sys
    base_arg = None
    if '--base' in sys.argv:
        base_idx = sys.argv.index('--base')
        if base_idx + 1 < len(sys.argv):
            base_arg = sys.argv[base_idx + 1]

    # 遍历固定的基准文件名与组合，从HTTP下载源与目标文件到内存并验证
    for base_name in BASE_NAMES:
        if base_arg and not base_name.startswith(base_arg):
            continue
        for (speed, pitch) in COMBINATIONS:
            for bits in BITS_PER_SAMPLE:
                input_bytes, input_name = download_input_file(base_name)
                if not input_bytes:
                    print(f'未找到输入文件(HTTP): {base_name}.wav/.pcm')
                    continue
                output_bytes, out_fname = download_output_file(base_name, speed, pitch, bits)
                if not output_bytes:
                    print(f'跳过：无法获取目标文件(HTTP) {base_name}_speed_{speed:.2f}_pitch_{pitch:.2f}_bits_{bits}.wav')
                    continue
                print(f'\n==== 验证: {out_fname} (input={input_name}, speed={speed}, pitch={pitch}, bits={bits}) ====')
                # 直接在内存中验证
                try:
                    results = verify_sonic_processing_from_bytes(
                        input_bytes, output_bytes, speed, pitch,
                        sample_rate_hint=None, channels_hint=None,
                        duration=None, tolerance=None,
                    )
                    passed = results and results.get('overall_pass')
                    print('通过' if passed else '失败', ': ', out_fname)
                except Exception as e:
                    print(f'运行失败: {e}')

def main():
    parser = argparse.ArgumentParser(description='Sonic音频处理验证脚本')
    parser.add_argument('--mode', choices=['single', 'batch'], default='single', 
                       help='验证模式: single=单文件验证, batch=批量HTTP验证')
    parser.add_argument('--input', '-i', help='输入音频文件路径 (single模式)')
    parser.add_argument('--output', '-o', help='输出音频文件路径 (single模式)')
    parser.add_argument('--speed', '-s', type=float, default=1.0, help='速度比例 (默认: 1.0)')
    parser.add_argument('--pitch', '-p', type=float, default=1.0, help='音调比例 (默认: 1.0)')
    parser.add_argument('--sample-rate', type=int, default=None, help='采样率 (自动从wav获取)')
    parser.add_argument('--channels', type=int, default=None, help='声道数 (自动从wav获取)')
    parser.add_argument('--bits', type=int, default=None, help='位宽 (自动从wav获取)')
    parser.add_argument('--duration', type=float, help='验证时长（秒）')
    parser.add_argument('--tolerance', type=str, help='误差容忍度JSON字符串')
    parser.add_argument('--base', type=str, help='批量模式：只处理指定base名的前缀（如manch）')
    
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
    verifier = SonicVerifier(sample_rate=sample_rate, channels=channels)
    
    # 解析容忍度设置
    tolerance = None
    if args.tolerance:
        try:
            tolerance = json.loads(args.tolerance)
        except json.JSONDecodeError:
            print("警告: 无法解析容忍度设置，使用默认值")
    
    # 执行验证
    results = verifier.verify_sonic_processing(
        input_file=args.input,
        output_file=args.output,
        speed_ratio=args.speed,
        pitch_ratio=args.pitch,
        duration=args.duration,
        tolerance=tolerance
    )
    
    if results is None:
        sys.exit(1)
    
    # 返回适当的退出码
    sys.exit(0 if results['overall_pass'] else 1)

if __name__ == '__main__':
    main()
