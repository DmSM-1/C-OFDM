import os
import re
from pathlib import Path

def compare_files(original_path, received_path):
    """Сравнивает два файла и возвращает процент совпадения байт и бит"""
    try:
        with open(original_path, 'rb') as f_orig, open(received_path, 'rb') as f_rec:
            orig_data = f_orig.read()
            rec_data = f_rec.read()
    except FileNotFoundError:
        return None, None
    
    # Сравнение байт
    min_len = min(len(orig_data), len(rec_data))
    if min_len == 0:
        return 0.0, 0.0
    
    byte_matches = sum(1 for i in range(min_len) if orig_data[i] == rec_data[i])
    byte_accuracy = byte_matches / min_len
    
    # Сравнение бит
    bit_matches = 0
    total_bits = min_len * 8
    
    for i in range(min_len):
        orig_byte = orig_data[i]
        rec_byte = rec_data[i]
        for bit in range(8):
            if ((orig_byte >> bit) & 1) == ((rec_byte >> bit) & 1):
                bit_matches += 1
    
    bit_accuracy = bit_matches / total_bits if total_bits > 0 else 0.0
    
    return byte_accuracy, bit_accuracy

def analyze_log(log_file_path):
    """Анализирует log.txt и выводит статистику"""
    log_pattern = re.compile(
        r'FRAME FROM\s+(\d+)\s+TO\s+(\d+)\s+SEQ_NUM\s+(\d+)\s+ITER\s+(\d+)'
    )
    
    total_frames = 0
    compared_frames = 0
    total_byte_accuracy = 0.0
    total_bit_accuracy = 0.0
    
    with open(log_file_path, 'r') as log_file:
        for line in log_file:
            match = log_pattern.match(line.strip())
            if not match:
                continue
                
            tx_id, rx_id, seq_num, iter_num = match.groups()
            total_frames += 1
            
            # Формируем пути к файлам
            original_filename = f"frames/frame_{seq_num}.txt"
            received_filename = f"frames/rx_frame_{seq_num}.txt"
            
            # Проверяем существование исходного пакета
            if not os.path.exists(original_filename):
                print(f"Original frame {seq_num} not found, skipping...")
                continue
            
            # Сравниваем файлы
            byte_acc, bit_acc = compare_files(original_filename, received_filename)
            
            if byte_acc is not None:
                compared_frames += 1
                total_byte_accuracy += byte_acc
                total_bit_accuracy += bit_acc
                
                print(f"FRAME FROM {tx_id:>3} TO {rx_id:>3} "
                      f"SEQ_NUM {seq_num:>5} ACCURACY {byte_acc:.5f} {bit_acc:.5f} "
                      f"ITER {iter_num:>6}")
    
    # Выводим итоговую статистику
    if compared_frames > 0:
        avg_byte_acc = total_byte_accuracy / compared_frames
        avg_bit_acc = total_bit_accuracy / compared_frames
        
        print("\n=== FINAL STATISTICS ===")
        print(f"Total frames processed: {total_frames}")
        print(f"Frames compared: {compared_frames}")
        print(f"Average byte accuracy: {avg_byte_acc:.5f}")
        print(f"Average bit accuracy: {avg_bit_acc:.5f}")
        print(f"Frame success rate: {compared_frames/total_frames:.3f}")
    else:
        print("No frames were successfully compared")

if __name__ == "__main__":
    log_path = "data/log.txt"
    
    # Проверяем существование log-файла
    if not os.path.exists(log_path):
        print(f"Log file {log_path} not found!")
        exit(1)
    
    # Создаем папку frames если её нет
    Path("frames").mkdir(exist_ok=True)
    
    analyze_log(log_path)