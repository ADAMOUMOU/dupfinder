#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <fstream>

namespace fs = std::filesystem;

// struct to store base info of a file
struct FileInfo {
  std::filesystem::path full_path;  // full path to the file
  std::uintmax_t size;              // file size (bytes)
};

using SizeGroupMap = std::unordered_map<std::uintmax_t, std::vector<FileInfo>>;

void collect_files(const std::filesystem::path& root_dir, std::vector<FileInfo>& files) 
{
  // browse root_dir and all the subfolders
  for (const fs::directory_entry& dir_entry : fs::recursive_directory_iterator(root_dir)) {
    // check if dir_entry is a regular file
    if (!dir_entry.is_regular_file()) 
      continue;

    FileInfo curr_file;
    curr_file.full_path = dir_entry.path();
    curr_file.size = std::filesystem::file_size(dir_entry.path());

    files.push_back(curr_file);
  }
}

SizeGroupMap group_by_size(const std::vector<FileInfo>& all_files) {
  SizeGroupMap sgm;

  for (const FileInfo& curr_file : all_files) {
    sgm[curr_file.size].push_back(curr_file);
  }

  return sgm;
}

std::string calculate_partial_hash(const std::filesystem::path& filepath, size_t bytes_to_read = 0) {
  if (bytes_to_read == 0)
    bytes_to_read = 8192; // 8ko
  
  // open file in bin mode
  std::ifstream file(filepath, std::ios::binary);
  if (!file.is_open())
    return "";

  // read chunk
  std::vector<char> buffer(bytes_to_read);
  file.read(buffer.data(), bytes_to_read);
  size_t bytes_read = file.gcount();

  // buffer to string
  std::string data(buffer.data(), bytes_read);

  // calculate the hash
  std::hash<std::string> hasher;
  size_t hash_value = hasher(data);

  return std::to_string(hash_value);
}

using HashGroupMap = std::unordered_map<std::string, std::vector<FileInfo>>;

HashGroupMap group_by_hash_partial(const SizeGroupMap& size_groups) {
  HashGroupMap hgm;

  for (auto& [_, files] : size_groups) {
    if (files.size() == 1)
      continue;
    
    for (const FileInfo& file : files) {
      size_t size_to_read = 8192;
      if (file.size < size_to_read)
        size_to_read = file.size;

      std::string hash = calculate_partial_hash(file.full_path, size_to_read);
      hgm[hash].push_back(file);
    }
  }

  return hgm;
}


int main(int argc, char *argv[]) {
    bool fast_mode = false;
    std::filesystem::path dir1;
    std::filesystem::path dir2;
    int dir_count = 0;

    if (argc < 2) {
        std::cerr << "At least 1 argument is expected: " << argv[0] << " <folder1> [folder2] [--fast]\n";
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--fast") {
            fast_mode = true;
        } else {
            if (dir_count == 0) {
                dir1 = arg;
                dir_count++;
            } else if (dir_count == 1) {
                dir2 = arg;
                dir_count++;
            } else {
                std::cerr << "Error: You provided more than 2 folders to check, this is not supported yet.\n";
                return 1;
            }
        }
    }
    
    if (!std::filesystem::exists(dir1) || !std::filesystem::is_directory(dir1)) {
        std::cerr << "Error: '" << dir1.string() << "' is not a valid path for a folder.\n";
        return 1;
    }

    if (!dir2.empty() && (!std::filesystem::exists(dir2) || !std::filesystem::is_directory(dir2))) {
        std::cerr << "Error: '" << dir2.string() << "' is not a valid path for a folder.\n";
        return 1;
    }

    if (fast_mode) {
        std::cout << "Starting fast search (partial hash comparison)...\n";
        
    } else {
        std::cout << "Starting full search (complete hash comparison)...\n";
        
    }

    return 0;
}




