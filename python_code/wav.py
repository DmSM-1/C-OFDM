from pydub import AudioSegment
import os

def mp3_to_wav_mono(input_file, output_file=None):
    """
    Конвертирует MP3 файл в WAV с одним каналом (моно)
    
    Args:
        input_file (str): путь к входному MP3 файлу
        output_file (str): путь к выходному WAV файлу (опционально)
    """
    try:
        # Если выходной файл не указан, создаем автоматически
        if output_file is None:
            output_file = os.path.splitext(input_file)[0] + "_mono.wav"
        
        # Загружаем аудио файл
        audio = AudioSegment.from_mp3(input_file)
        
        # Конвертируем в моно (1 канал)
        audio_mono = audio.set_channels(1)
        
        # Экспортируем в WAV
        audio_mono.export(output_file, format="wav")
        
        print(f"Конвертация завершена: {input_file} -> {output_file}")
        print(f"Частота дискретизации: {audio.frame_rate} Hz")
        print(f"Битность: {audio.sample_width * 8} bit")
        
    except Exception as e:
        print(f"Ошибка при конвертации: {e}")

# Пример использования
if __name__ == "__main__":
    input_mp3 = "FlyMeToTheMoon.mp3"  # Замените на путь к вашему файлу
    mp3_to_wav_mono(input_mp3)