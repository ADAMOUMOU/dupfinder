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

HashGroupMap group_by_hash_partial(const SizeGroupMap& size_groups, size_t size_to_read) {
  HashGroupMap hgm;

  for (auto& [_, files] : size_groups) {
    if (files.size() == 1)
      continue;
    
    for (const FileInfo& file : files) {
      if (file.size < size_to_read)
        size_to_read = file.size;

      std::string hash = calculate_partial_hash(file.full_path, size_to_read);
      hgm[hash].push_back(file);
    }
  }

  return hgm;
}


HashGroupMap find_final_duplicates(const HashGroupMap& partial_groups) {
  HashGroupMap hgm;
  
  for (auto& [_, files] : partial_groups) {
    if (files.size() == 1)
      continue;
    
    for (const FileInfo& file : files) {
      size_t size_to_read = file.size;

      std::string hash = calculate_partial_hash(file.full_path, size_to_read);
      hgm[hash].push_back(file);
    }
  }
  
  return hgm;
}

void display_duplicates(const HashGroupMap& groups) {
  for (auto& [hash, files] : groups) {
    if (files.size() == 1) 
      continue;

    std::cout << std::endl << hash << ":" << std::endl;

    for (auto& [full_path, size] : files) {
      std::cout << "  -" << full_path.string() << "(" << size << " bytes)" << std::endl;
    }
  }
}

int main(int argc, char *argv[]) {
  bool fast_mode = false;
  std::filesystem::path dir1;
  std::filesystem::path dir2;
  int dir_count = 0;
  size_t size_to_read;

  if (argc < 2) {
    std::cerr << "At least 1 argument is expected: " << argv[0] << " <folder1> [folder2] [--fast bits_to_read]\n";
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "--fast") {
      fast_mode = true;
      
      i++;
      if (i >= argc) {
        std::cerr << "Error: the argument for the option '--fast' is missing." << std::endl;
        return 1;
      }
      
      char* end;
      long val = std::strtol(argv[i], &end, 10);

      if (*end != '\0' || val < 0) {
        std::cerr << "Error: '" << end << "' is not a valid number." << std::endl;
        return 1;
      }

      size_to_read = static_cast<size_t>(val);

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

  std::vector<FileInfo> fi_list;
  collect_files(dir1, fi_list);

  if (dir_count == 2) {
    collect_files(dir2, fi_list);
  }

  HashGroupMap hgm = group_by_hash_partial(group_by_size(fi_list), size_to_read);
  
  if (!fast_mode)
    hgm = find_final_duplicates(hgm);

  // shows the result
  display_duplicates(hgm);

  return 0;
}
