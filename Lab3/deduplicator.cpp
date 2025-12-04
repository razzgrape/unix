#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <cstdio>
#include <unistd.h>
#include <openssl/sha.h>
#include <filesystem>

namespace fs = std::filesystem;

const int HASH_BYTE_COUNT = SHA_DIGEST_LENGTH;

std::string calculate_file_fingerprint(const fs::path& target_file_path)
{
    unsigned char hash_result_data[HASH_BYTE_COUNT];
    SHA_CTX sha_processor;
    if (SHA1_Init(&sha_processor) != 1) return "";

    std::ifstream file_stream(target_file_path, std::ios::binary);
    if (!file_stream.is_open()) return "";

    char read_buffer[8192];
    
    while (file_stream.read(read_buffer, sizeof(read_buffer))){
        SHA1_Update(&sha_processor, (unsigned char*)read_buffer, file_stream.gcount());
    }

    if (file_stream.gcount() > 0) {
        SHA1_Update(&sha_processor, (unsigned char*)read_buffer, file_stream.gcount());
    }

    if (SHA1_Final(hash_result_data, &sha_processor) != 1) return "";
    
    char hex_string_buffer[2 * HASH_BYTE_COUNT + 1];
    for (int i = 0; i < HASH_BYTE_COUNT; i++) {
        std::sprintf(hex_string_buffer + i * 2, "%02x", hash_result_data[i]);
    }

    return std::string(hex_string_buffer, 40);
}

bool replace_file_with_hardlink(const fs::path& original_source, const fs::path& duplicate_target)
{
    if (!fs::remove(duplicate_target)) {
        std::cerr << "Не удалось удалить файл-дубликат: " << duplicate_target.string() << "\n";
        return false;
    }
    
    if (::link(original_source.c_str(), duplicate_target.c_str()) == 0) {
        std::cout << "Заменено жесткой ссылкой: " << duplicate_target.string() << "\n";
        return true;
    }

    std::cerr << "Не удалось создать жесткую ссылку: " << duplicate_target.string() << " -> "<< original_source.string() << "\n";
    return false;
}


int main(int argc, char* argv[]) {
    
    if (argc != 2){
        std::cerr << "Ошибка: Не указан путь к каталогу." << "\n";
        std::cerr << "Использование: " << argv[0] << " <путь_к_каталогу>" << "\n";
        return 1; 
    }

    std::string working_directory_path = argv[1];

    if (!fs::is_directory(working_directory_path)) {
        std::cerr << "Ошибка: Путь '" << working_directory_path << "' не является каталогом или не существует." << "\n";
        return 1;
    }

    std::unordered_map<std::string, fs::path> hash_map_storage;

    std::cout << "Начало поиска дубликатов в: " << working_directory_path << "\n";

    try {
        for (const auto& directory_entry : fs::recursive_directory_iterator(working_directory_path)) {

            if (!fs::is_regular_file(directory_entry)) continue;

            fs::path current_file_path = directory_entry.path();

            std::string file_hash = calculate_file_fingerprint(current_file_path);
            if (file_hash.empty()) continue;

            if (hash_map_storage.count(file_hash)) {
                const fs::path& original_path = hash_map_storage.at(file_hash);
                
                std::cout << "Дубликат найден: " << current_file_path << " совпадает с " << original_path << "\n";
                
                replace_file_with_hardlink(original_path, current_file_path);
            }
            else {
                hash_map_storage[file_hash] = current_file_path;
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Произошла ошибка файловой системы при обходе: " << e.what() << "\n";
        return 2;
    }
    std::cout << "--- Программа завершена. ---" << "\n";
    return 0;
}   