#!/bin/sh

SRC_FILE="$1"
OUTPUT_KEYWORD="Output:"
TEMP_DIR=""

ERR_MISSING_ARG=1
ERR_FILE_NOT_FOUND=2
ERR_OUTPUT_NOT_FOUND=3
ERR_COMPILATION_FAILED=4
ERR_FINAL_MOVE_FAILED=5

cleanup() {
    if [ -n "$TEMP_DIR" ] && [ -d "$TEMP_DIR" ]; then
        rm -rf "$TEMP_DIR"
        # echo "Временный каталог $TEMP_DIR удален." >&2
    fi  
}

get_compiler_and_ext() {
    case "$SRC_FILE" in
        *.c)
            COMPILER="cc"
            EXT_TO_COPY=""
            ;;
        *.cpp|*.cc|*.cxx)
            COMPILER="c++"
            EXT_TO_COPY=""
            ;;
        *.tex)
            COMPILER="pdflatex"
            EXT_TO_COPY=".pdf"
            ;;
        *)
            return 1
            ;;
    esac
    return 0
}

if [ -z "$SRC_FILE" ]; then
    echo "Ошибка: Не указан исходный файл." >&2
    echo "Использование: $0 <исходный_файл>" >&2
    exit $ERR_MISSING_ARG
fi

if [ ! -f "$SRC_FILE" ]; then
    echo "Ошибка: Исходный файл '$SRC_FILE' не найден." >&2
    exit $ERR_FILE_NOT_FOUND
fi

trap cleanup EXIT
trap 'cleanup; exit 130' INT
trap 'cleanup; exit 143' TERM

TEMP_DIR=$(mktemp -d -t build.XXXXXX)

if [ $? -ne 0 ]; then
    echo "Ошибка: Не удалось создать временный каталог с помощью mktemp." >&2
    exit 1
fi

OUTPUT_LINE=$(grep "^[[:space:]]*//.*$OUTPUT_KEYWORD\|^[[:space:]]*%.*$OUTPUT_KEYWORD" "$SRC_FILE" | head -n 1)

if [ -z "$OUTPUT_LINE" ]; then
    echo "Ошибка: Комментарий '$OUTPUT_KEYWORD' не найден в '$SRC_FILE'." >&2
    exit $ERR_OUTPUT_NOT_FOUND
fi

FINAL_FILENAME=$(echo "$OUTPUT_LINE" | sed -E "s/.*$OUTPUT_KEYWORD[[:space:]]*//; s/([[:space:]]|;).*//")

if [ -z "$FINAL_FILENAME" ]; then
    echo "Ошибка: Имя выходного файла не указано после '$OUTPUT_KEYWORD'." >&2
    exit $ERR_OUTPUT_NOT_FOUND
fi

get_compiler_and_ext
COMPILER_RESULT=$?

if [ $COMPILER_RESULT -ne 0 ]; then
    echo "Ошибка: Неизвестный тип файла '$SRC_FILE'." >&2
    exit $ERR_COMPILATION_FAILED
fi

BASE_SRC_NAME=$(basename "$SRC_FILE")

echo "Начало сборки '$SRC_FILE' в '$TEMP_DIR'..."

cp "$SRC_FILE" "$TEMP_DIR/"

(
    cd "$TEMP_DIR" || exit $ERR_COMPILATION_FAILED
    
    COMPILE_COMMAND=""
    
    case "$COMPILER" in
        cc|c++)
            COMPILE_COMMAND="$COMPILER $BASE_SRC_NAME -o $FINAL_FILENAME"
            $COMPILE_COMMAND
            ;;
        pdflatex)
            COMPILE_COMMAND="$COMPILER $BASE_SRC_NAME"
            $COMPILE_COMMAND > /dev/null 2>&1
            if [ $? -eq 0 ]; then
                $COMPILE_COMMAND > /dev/null 2>&1
            fi
            TEMP_OUTPUT_FILE="$TEMP_DIR/$(basename "$SRC_FILE" .tex)$EXT_TO_COPY"
            ;;
        *)
            echo "Ошибка: Неизвестный компилятор '$COMPILER'." >&2
            exit $ERR_COMPILATION_FAILED
            ;;
    esac

    if [ $? -ne 0 ]; then
        echo "Ошибка: Компиляция не удалась. Команда: '$COMPILE_COMMAND'" >&2
        exit $ERR_COMPILATION_FAILED
    fi

) 

if [ $? -ne 0 ]; then
    exit $? 
fi

if [ "$COMPILER" = "pdflatex" ]; then
    TEMP_OUTPUT_FILE="$TEMP_DIR/$(basename "$SRC_FILE" .tex)$EXT_TO_COPY"
else
    TEMP_OUTPUT_FILE="$TEMP_DIR/$FINAL_FILENAME"
fi

DEST_DIR=$(dirname "$SRC_FILE")

echo "Сборка завершена. Перемещение '$TEMP_OUTPUT_FILE' в '$DEST_DIR/$FINAL_FILENAME'."

mv -f "$TEMP_OUTPUT_FILE" "$DEST_DIR/$FINAL_FILENAME"

if [ $? -ne 0 ]; then
    echo "Ошибка: Не удалось переместить конечный файл '$FINAL_FILENAME'." >&2
    exit $ERR_FINAL_MOVE_FAILED
fi

echo "Успешно: '$DEST_DIR/$FINAL_FILENAME' создан."
exit 0
