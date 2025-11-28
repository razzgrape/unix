#!/bin/sh

SHARED_DIR="/shared_volume"
LOCK_FILE="${SHARED_DIR}/lockfile.lock"
CID=$RANDOM
COUNTER=0

while true; do

    CREATED_FILENAME="" 
    
    {
        flock -x 666
        FILE_NAME=""
        for i in $(seq -w 1 999); do
            if [ ! -f "${SHARED_DIR}/${i}.txt" ]; then
                FILE_NAME="${i}.txt"
                break
            fi
        done

        if [ -n "$FILE_NAME" ]; then
            COUNTER=$((COUNTER + 1))
            CONTENT="${CID} ${COUNTER}"
            echo "$CONTENT" > "${SHARED_DIR}/${FILE_NAME}"
            CREATED_FILENAME="${SHARED_DIR}/${FILE_NAME}"
            echo "[+] Created: ${FILE_NAME} (${COUNTER}) by ${CID}"
        fi
     } 666>"${LOCK_FILE}"

    sleep 1

    if [ -n "$CREATED_FILENAME" ]; then
        rm -f "$CREATED_FILENAME"
        echo "[-] Deleted: $(basename "$CREATED_FILENAME") by ${CID}"
    fi

    sleep 1
done