#!/bin/sh

SHARED_DIR="/shared_volume"
LOCK_FILE="${SHARED_DIR}/lockfile.lock"
CID=$(cat /proc/sys/kernel/random/uuid | cut -c 1-8)
COUNTER=0

while true; do

    (
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
            echo "[+] Created: ${FILE_NAME} (${COUNTER})"
        fi

    ) 200>"${LOCK_FILE}"

    sleep 1

    TO_REMOVE=$(ls -1 "${SHARED_DIR}" | grep '^[0-9]' | head -1)

    if [ -n "$TO_REMOVE" ] && [ "$TO_REMOVE" != "lockfile.lock" ]; then
        rm "${SHARED_DIR}/${TO_REMOVE}"
        echo "[-] Deleted: ${TO_REMOVE}"
    fi

    sleep 1

done