import sys
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

def parse_log_file(filepath: str) -> pd.DataFrame:
    """
    Парсит лог-файл C++ в DataFrame pandas.
    """
    parsed_data = []
    try:
        with open(filepath, 'r') as f:
            for line in f:
                if not line.strip():
                    continue
                
                iteration_data = {}
                parts = line.strip().split(' ')
                
                for part in parts:
                    try:
                        key, value = part.split(':', 1)
                        if '.' in value or 'e' in value: # Учитываем научную нотацию
                            iteration_data[key] = float(value)
                        else:
                            iteration_data[key] = int(value)
                    except ValueError:
                        pass
                
                if iteration_data:
                    parsed_data.append(iteration_data)
                    
    except FileNotFoundError:
        print(f"Ошибка: Файл '{filepath}' не найден.")
        return None
        
    if not parsed_data:
        print("Ошибка: Не удалось проанализировать данные из файла.")
        return None

    return pd.DataFrame(parsed_data).sort_values(by='ITER').reset_index(drop=True)

def preprocess_and_amortize(df: pd.DataFrame) -> pd.DataFrame:
    """
    Выполняет предварительную обработку данных для расчета амортизированного времени.
    """
    print("⏳ Выполняется предварительная обработка и амортизация времени...")
    
    # Заполняем пропуски в FR_IN_BUF, если они есть
    if 'FR_IN_BUF' not in df.columns:
        print("Внимание: столбец 'FR_IN_BUF' не найден. Амортизация не будет выполнена.")
        df['amortized_SDR'] = df['SDR']
        df['amortized_CONVERT'] = df['CONVERT']
        return df

    # Шаг 1: Определяем группы пакетов, относящиеся к одному буферу.
    df['buffer_group'] = (df['FR_IN_BUF'] == 1).cumsum()
    
    # Шаг 2: Находим, сколько всего пакетов было в каждом буфере.
    packets_in_buffer = df.groupby('buffer_group')['FR_IN_BUF'].transform('max')
    
    # Шаг 3: Рассчитываем амортизированное время.
    df['amortized_SDR'] = (df['SDR'].fillna(0) / packets_in_buffer)
    df['amortized_CONVERT'] = (df['CONVERT'].fillna(0) / packets_in_buffer)
    
    # Шаг 4: "Растягиваем" вычисленное значение на все пакеты в группе.
    df['amortized_SDR'] = df.groupby('buffer_group')['amortized_SDR'].transform('max')
    df['amortized_CONVERT'] = df.groupby('buffer_group')['amortized_CONVERT'].transform('max')
    
    print("✅ Амортизация завершена.")
    return df

def analyze_and_plot(df: pd.DataFrame):
    """
    Анализирует данные и строит графики.
    """
    # 1. Определяем столбцы с временем для анализа.
    # *** ИЗМЕНЕНИЕ: Убрали 'TIME' из списка исключений. ***
    time_columns_per_packet = [
        col for col in df.columns 
        if col not in ['ITER', 'SEQ', 'DET', 'FR_IN_BUF', 'GLOBAL', 'SDR', 'CONVERT', 'buffer_group'] 
        and (df[col].dtype == 'float64' or df[col].dtype == 'int64')
    ]
    # Добавляем новые амортизированные колонки
    if 'amortized_SDR' in df.columns:
        time_columns_per_packet.extend(['amortized_SDR', 'amortized_CONVERT'])
    
    # --- График 1: Динамика времени обработки по итерациям ---
    plt.figure(figsize=(15, 8))
    for col in time_columns_per_packet:
        plot_data = df[df[col] > 0]
        plt.plot(plot_data['ITER'], plot_data[col], marker='o', linestyle='-', markersize=3, alpha=0.7, label=col)
    
    plt.title('Динамика времени обработки пакетов (по итерациям)', fontsize=16)
    plt.xlabel('Номер итерации (ITER)', fontsize=12)
    plt.ylabel('Время (секунды)', fontsize=12)
    plt.legend()
    plt.grid(True, which='both', linestyle='--', linewidth=0.5)
    plt.yscale('log')
    plt.tight_layout()

    # --- График 2: Сглаженная динамика времени (скользящее среднее) ---
    plt.figure(figsize=(15, 8))
    rolling_window = 20
    for col in time_columns_per_packet:
        plot_data = df[df[col] > 0]
        if len(plot_data) > rolling_window:
             plt.plot(plot_data['ITER'], plot_data[col].rolling(window=rolling_window).mean(), linestyle='-', alpha=0.9, label=f'{col} (сглаженное)')

    plt.title(f'Сглаженная динамика времени обработки (окно = {rolling_window})', fontsize=16)
    plt.xlabel('Номер итерации (ITER)', fontsize=12)
    plt.ylabel('Время (секунды)', fontsize=12)
    plt.legend()
    plt.grid(True, which='both', linestyle='--', linewidth=0.5)
    plt.yscale('log')
    plt.tight_layout()

    # --- График 3: Номера пакетов и общее количество обработанных ---
    if 'SEQ' in df.columns and 'DET' in df.columns:
        plt.figure(figsize=(15, 8))
        plt.plot(df['ITER'], df['SEQ'], 'o-', label='Номер принятого пакета (SEQ)', alpha=0.7)
        plt.plot(df['ITER'], df['DET'], 's-', label='Всего обработано пакетов (DET)', alpha=0.7)
        
        plt.title('Динамика обработки пакетов', fontsize=16)
        plt.xlabel('Номер итерации (ITER)', fontsize=12)
        plt.ylabel('Номер пакета', fontsize=12)
        plt.legend()
        plt.grid(True)
        plt.tight_layout()

    # --- График 4: Итоговое сравнение средних времен ---
    average_times = df[time_columns_per_packet].mean().sort_values(ascending=False)
    
    print("\n--- Среднее время на один пакет (с учетом амортизации) ---")
    print(average_times.round(6))
    
    plt.figure(figsize=(12, 7))
    sns.set_theme(style="whitegrid")
    barplot = sns.barplot(x=average_times.index, y=average_times.values, palette="viridis")
    
    plt.title('Среднее время, затраченное на каждый этап обработки', fontsize=16)
    plt.xlabel('Этап обработки', fontsize=12)
    plt.ylabel('Среднее время (секунды)', fontsize=12)
    plt.xticks(rotation=45, ha='right')
    
    for p in barplot.patches:
        barplot.annotate(format(p.get_height(), '.2e'),
                       (p.get_x() + p.get_width() / 2., p.get_height()),
                       ha='center', va='center', xytext=(0, 9), textcoords='offset points')

    plt.tight_layout()
    
    print("\n📊 Отображение графиков...")
    plt.show()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        log_filepath = "LOG.txt"
        print(f"INFO: Файл лога не указан. Используется файл по умолчанию: '{log_filepath}'")
    else:
        log_filepath = sys.argv[1]

    raw_df = parse_log_file(log_filepath)
    
    if raw_df is not None:
        processed_df = preprocess_and_amortize(raw_df)
        analyze_and_plot(processed_df)