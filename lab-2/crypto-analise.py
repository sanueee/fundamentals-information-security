import os
from collections import Counter
import string
abbr_list = set()  # список слов-аббревиатур

RU_FREQ_ORDER = "ОЕАИНТСРВЛКМДПУЯЫЬГЗБЧЙХЖШЮЦЩЭФЪ" # по убыванию вероятности
RU_ALPHABET = "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" # алфавит

def load_cryptogram():
    print("[1] ввести криптограмму вручную")
    print("[2] загрузить из файла")
    choice = input("выберете способ ввода (1 или 2): ").strip()

    if choice == "2":
        path = input("укажите путь к файлу: ").strip()
        try:
            with open(path, 'r', encoding='windows-1251') as f:
                text = f.read().upper()
                print("--криптограмма успешно загружена--")
                return text
        except (FileNotFoundError):
            print("--файл не найден--")
            return

    print("--введите криптограмму (завершение: "" + enter):")
    lines = []
    while True:
        line = input()
        if line == "":
            break
        lines.append(line.upper())
    return "\n".join(lines) # соединяем так же как в вводил пользователь

def get_letter_freq(text):
    letters = [c for c in text if c not in string.punctuation and not c.isdigit() and c != "\n" and not c.isspace()]
    total = len(letters)
    if total == 0:
        return {}
    letter_counts = Counter(letters)
    return {ch: letter_counts[ch] / total for ch in sorted(letter_counts, key=letter_counts.get, reverse=True)}

def show_frequency(text):
    freq = get_letter_freq(text)
    if not freq:
        print("пустая криптограмма")
        return

    print("--частотный анализ криптограммы--")
    print(f"{'буква':<8} {'частота':>8}  {'предполагаемая замена'}")

    for i, (ch, f) in enumerate(freq.items()):
        if i < len(RU_FREQ_ORDER):
            suggestion = RU_FREQ_ORDER[i]
        else: suggestion = "?"
        print(f" {ch:5} {f:8.2%} -> {suggestion}")

def apply_substitution(text, subs):
    result = []
    start_of_sentence = True
    i = 0
    while i < len(text):
        ch = text[i]
        if ch in RU_ALPHABET:
            current_word = []
            while i < len(text) and text[i] in RU_ALPHABET:
                current_word.append(text[i])
                i += 1
            full_word = ''.join(current_word)
            is_abbr = full_word in abbr_list
            for j, letter in enumerate(full_word):
                if is_abbr:
                    result.append(letter)
                elif j == 0 and start_of_sentence:
                    result.append(letter)
                    start_of_sentence = False
                elif letter in subs:
                    result.append(subs[letter].lower())
                else:
                    result.append(letter)
        else:
            result.append(ch)
            if ch in '.!?\n':
                start_of_sentence = True
            i += 1
    return "".join(result)

def show_cryptogram(text, subs):
    print("--криптограмма (ЗАГЛАВНЫЕ = нерасшифровано, строчные = расшифровано)")
    decoded = apply_substitution(text, subs)
    print(decoded)

def show_words_by_length(text, subs):
    decoded = apply_substitution(text, subs)
    words = decoded.split()
    by_len = {}
    for w in set(words):
        clean_w = w.strip(',.!:')
        l = len(clean_w)
        if l not in by_len:
            by_len[l] = []
        by_len[l].append(clean_w)

    print("--cлова по количеству букв--")
    for l in sorted(by_len.keys()):
        words_str = "  ".join(sorted(by_len[l]))
        print(f"[{l:2}]: {words_str}")

def show_words_by_unsolved(text, subs):
    decoded = apply_substitution(text, subs)
    sentence_start_positions = set()
    is_start = True
    for i, ch in enumerate(decoded):
        if ch.isalpha():
            if is_start:
                sentence_start_positions.add(i)
                is_start = False
        elif ch in '.!?\n':
            is_start = True
    by_unsolved = {}
    seen = set()

    i = 0
    while i < len(decoded):
        if not decoded[i].isalpha():
            i += 1
            continue

        word_start = i # запомнили начало слова
        word = []
        while i < len(decoded) and decoded[i].isalpha():
            word.append(decoded[i])
            i += 1

        clean_word = ''.join(word)
        upper_key = clean_word.upper()

        if upper_key in seen:
            continue
        seen.add(upper_key)

        if upper_key in abbr_list:
            n = 0
        else:
            n = 0
            for j, c in enumerate(clean_word):
                if j == 0 and word_start in sentence_start_positions:
                    continue
                if c.isupper():
                    n += 1

        if n not in by_unsolved:
            by_unsolved[n] = []
        by_unsolved[n].append(clean_word)

    print("--слова по числу нерасшифрованных букв:")
    for n in sorted(by_unsolved.keys()):
        label = "полностью расшифрованы" if n == 0 else f"{n} нерасшифрованных букв"
        words_str = "  ".join(sorted(by_unsolved[n]))
        print(f"{label}: {words_str}")

def show_substitution_table(subs):
    print("\nтекущая таблица замен")
    if not subs:
        print("замены ещё не заданы")
        return
    for coded, uncoded in sorted(subs.items()):
        print(f"{coded} -> {uncoded}")

def auto_replace(text, subs, history):
    freq = get_letter_freq(text)
    coded_order = list(freq.keys())

    new_subs = dict(subs)
    already_used = set(new_subs.values())

    count = 0
    for i, coded_ch in enumerate(coded_order):
        if coded_ch in new_subs:
            continue

        for uncoded_ch in RU_FREQ_ORDER:
            if uncoded_ch not in already_used:
                new_subs[coded_ch] = uncoded_ch
                already_used.add(uncoded_ch)
                count += 1
                break

    history.append(dict(subs))
    subs.clear()
    subs.update(new_subs)
    print(f"--автоматически добавлено {count} замен--")

def manual_replace(subs, history):
    print("--введите замену в формате: БУКВА_ШИФРА -> БУКВА_ТЕКСТА--")
    print("например: Р -> О   (буква Р в криптограмме означает О)")
    raw = input("замена: ").strip().upper().replace("->", " ")
    parts = raw.split()
    if len(parts) != 2:
        print("неверный формат")
        return

    coded_ch, uncoded_ch = parts[0], parts[1]
    if coded_ch not in RU_ALPHABET or uncoded_ch not in RU_ALPHABET:
        print("буквы должны быть из русского алфавита.")
        return

    if uncoded_ch in subs.values():
        existing = [k for k, v in subs.items() if v == uncoded_ch][0]
        print(f"ошибка: буква '{uncoded_ch}' уже используется в замене {existing} -> {uncoded_ch}")
        return

    history.append(dict(subs)) # сохранили предыдущее состояние
    subs[coded_ch] = uncoded_ch # обновили таблицу
    print(f"замена добавлена: {coded_ch} -> {uncoded_ch}")

def undo(subs, history):
    if not history:
        print("история пуста, откат невозможен")
        return
    subs.clear()
    subs.update(history.pop())
    print("откат выполнен")

def remove_replace(subs, history):
    ch = input("введите букву шифра, замену которой нужно удалить: ").strip().upper()
    if ch in subs:
        history.append(dict(subs))
        del subs[ch]
        print(f"замена для {ch} удалена")
    else:
        print(f"замены для {ch} не найдено")

def save_result(text, subs):
    path = input("имя файла для сохранения: ").strip()
    decoded = apply_substitution(text, subs)
    with open(path, 'w', encoding='windows-1251') as f:
        f.write(text + "\n")
        f.write(decoded + "\n")
        for c, p in sorted(subs.items()):
            f.write(f"{c} -> {p}\n")

def mark_as_abbreviation():
    word = input("введите аббревиатуру (например: ГТ, ЭВМ): ").strip().upper()

    if not word:
        print("пустой ввод, отменено")
        return

    if not all(c in RU_ALPHABET for c in word):
        print("аббревиатура должна содержать только русские буквы")
        return

    abbr_list.add(word)
    print(f"'{word}' помечена как аббревиатура")
    print(f"всего аббревиатур: {len(abbr_list)}")

def show_abbreviations():
    print("--помеченные аббревиатуры:")
    if not abbr_list:
        print("  (список пуст)")
    else:
        for abbr in sorted(abbr_list):
            print(f"  - {abbr}")

def remove_abbreviation():
    word = input("введите аббревиатуру для удаления: ").strip().upper()
    if word in abbr_list:
        abbr_list.remove(word)
        print(f"'{word}' удалена из списка аббревиатур")
    else:
        print(f"'{word}' не найдена в списке")

def main():
    text = load_cryptogram()
    if not text:
        print("файл пуст")
        return

    subs = {}    # словарь замен: буква_шифра -> буква_текста
    history = [] # история для отката

    while True:
        print("--панель команд:")
        print("  [1] показать криптограмму")
        print("  [2] частотный анализ")
        print("  [3] слова по длине")
        print("  [4] слова по числу нерасшифрованных букв")
        print("  [5] текущая таблица замен")
        print("  [6] добавить/изменить замену вручную")
        print("  [7] удалить замену")
        print("  [8] откатить последнее действие")
        print("  [9] автоматические замены по частотам")
        print("  [10] сохранить результат в файл")
        print("  [11] пометить слово как аббревиатуру")
        print("  [12] показать список аббревиатур")
        print("  [13] удалить аббревиатуру")
        print("  [0] выход")

        choice = input("--введите номер желаемой команды: ").strip()

        if choice == "1":
            show_cryptogram(text, subs)
        elif choice == "2":
            show_frequency(text)
        elif choice == "3":
            show_words_by_length(text, subs)
        elif choice == "4":
            show_words_by_unsolved(text, subs)
        elif choice == "5":
            show_substitution_table(subs)
        elif choice == "6":
            manual_replace(subs, history)
        elif choice == "7":
            remove_replace(subs, history)
        elif choice == "8":
            undo(subs, history)
        elif choice == "9":
            auto_replace(text, subs, history)
        elif choice == "10":
            save_result(text, subs)
        elif choice == "11":
            mark_as_abbreviation()
        elif choice == "12":
            show_abbreviations()
        elif choice == "13":
            remove_abbreviation()
        elif choice == "0":
            print("завершение работы...")
            break
        else:
            print("неверный ввод! попробуйте снова...")


if __name__ == "__main__":
    main()